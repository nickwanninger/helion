// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/ast.h>
#include <helion/parser.h>
#include <helion/pstate.h>

#include <map>


using namespace helion;


/**
 * This file is the implementation of <helion/parser.h> and is where the bulk
 * of the parsing logic lives. It's a hand-written recursive decent parser that
 * uses the `pstate` struct to allow `efficient` backtracking
 * TODO(optim): actually determine the efficiency of the pstate struct
 */

#define ADD_PARSER(name) static presult parse_##name(pstate);
ADD_PARSER(num);
ADD_PARSER(var);
ADD_PARSER(paren);
ADD_PARSER(str);
ADD_PARSER(do);
static presult parse_expr(pstate, bool do_binary = true);
static presult parse_binary_rhs(pstate s, int expr_prec, ast::node *lhs);
static presult expand_expression(presult);
static presult parse_function_args(pstate);




auto glob_term(pstate s) {
  while (s.first().type == tok_term) {
    s = s.next();
  }
  return s;
}




/**
 * primary parse function, ideally called per-file
 */
ast::module *helion::parse_module(pstate s) {
  auto *module = new ast::module();

  while (true) {
    // while we can, parse a statement
    if (auto r = parse_expr(s); r) {
      // inherit the state from the parser. This allows us to pick up right
      // after the end of the last parse_expr
      s = r.state;
      for (auto v : r.vals) {
        module->stmts.push_back(v);
      }

      bool found = false;
      while (true) {
        if (token end = s; end.type == tok_term) {
          s++;
          found = true;
        } else
          break;
      }

      if (s.first().type == tok_eof) {
        break;
      }
    } else {
      throw syntax_error(s, "unexpected token");
    }
  }

  return module;
}


/**
 * wrapper that creates a state around text
 */
ast::module *helion::parse_module(text s, text pth) {
  auto t = std::make_shared<tokenizer>(s, pth);
  pstate state(t, 0);
  return parse_module(state);
}



/**
 * primary expression parser
 */
static presult parse_expr(pstate s, bool do_binary) {
  token begin = s;
  auto res = pfail();

#define TRY(expr)                \
  {                              \
    res = expr;                  \
    if (res) goto parse_success; \
  }

  if (!res && begin.type == tok_num) TRY(parse_num(s));
  if (!res && begin.type == tok_left_paren) TRY(parse_paren(s));
  if (!res && begin.type == tok_var) TRY(parse_var(s));
  if (!res && begin.type == tok_str) TRY(parse_str(s));
  if (!res && begin.type == tok_do) TRY(parse_do(s));

parse_success:
  if (!res) {
    return pfail();
  }

  res = expand_expression(res);
  if (do_binary && res) {
    return parse_binary_rhs(res, -100, res);
  } else {
    return res;
  }
}


static presult expand_expression(presult r) {
  pstate initial_state = r;
  while (true) {
    ast::node *expr = r;
    pstate s = r;
    token t = s;


    token start_token = t;

    if (t.type == tok_dot) {
      s++;
      t = s;
      if (t.type == tok_var) {
        s++;
        auto *v = new ast::dot();
        v->set_bounds(start_token, t);
        v->expr = expr;
        v->sub = t.val;
        r = presult(v, s);
        continue;
      }
    }



    if (t.type == tok_left_paren) {
      auto res = parse_function_args(s);
      s = res;
      t = s;
      if (t.type != tok_right_paren) {
        throw syntax_error(initial_state, "malformed function call");
      }

      auto *c = new ast::call();
      c->set_bounds(start_token, t);
      c->func = expr;
      c->args = res.vals;
      s++;
      r = presult(c, s);
      continue;
    }


    if (t.type == tok_left_square) {
      auto sub = new ast::subscript();
      sub->expr = expr;
      s++;
      t = s;
      while (true) {
        t = s;
        if (t.type == tok_right_square) {
          break;
        }
        auto res = parse_expr(s, true);
        if (!res) return pfail();

        sub->subs.push_back(res);
        s = res;
        t = s;

        if (t.type == tok_comma) {
          s++;
          continue;
        } else {
          break;
        }
      }
      s++;

      sub->set_bounds(initial_state, t);
      r = presult(sub, s);
      continue;
    }




    break;
  }
  return r;
}


static presult parse_var(pstate s) {
  auto *v = new ast::var();
  v->value = s.first().val;
  return presult(v, s.next());
}



static presult parse_num(pstate s) {
  token t = s;
  std::string src = t.val;

  ast::number *node = new ast::number();
  node->set_bounds(t, t);

  // determine if the token is a float or not
  bool floating = false;
  for (auto r : src) {
    if (r == '.') {
      floating = true;
      break;
    }
  }

  if (floating) {
    // parse a float
    double i = std::stod(src);
    node->type = ast::number::floating;
    node->as.floating = i;
  } else {
    // parse an int
    int64_t i = std::stoll(src);
    node->type = ast::number::integer;
    node->as.integer = i;
  }

  return presult(node, s.next());
}


static presult parse_str(pstate s) {
  auto *n = new ast::string();
  n->val = s.first().val;
  s++;
  return presult(n, s);
}




/**
 * the paren parser will handle parsing (x) and (x, y) where
 * the first is simply x, and the second is the tuple (x,y)
 */
static presult parse_paren(pstate s) {
  auto init_state = s;
  s++;
  std::vector<ast::node *> exprs;

  bool tuple = false;
  token t = s;

  while (true) {
    t = s;
    if (t.type == tok_right_paren) {
      break;
    }
    auto res = parse_expr(s, true);
    if (!res) return pfail();

    exprs.push_back(res);
    s = res;
    t = s;

    if (t.type == tok_comma) {
      tuple = true;
      s++;
      continue;
    } else {
      break;
    }
  }


  if (exprs.size() == 0) {
    throw syntax_error(init_state, "Expected expression inside parenthesis");
  }

  ast::node *n = nullptr;
  if (tuple) {
    auto *tup = new ast::tuple();
    tup->vals = exprs;
    n = tup;
  } else {
    n = exprs[0];
  }
  s++;

  return presult(n, s);
}




static presult parse_function_args(pstate s) {
  std::vector<ast::node *> args;

  token t = s;
  if (t.type == tok_left_paren) s++;

  while (true) {
    t = s;
    /*t.type == tok_term || t.type == tok_eof || */
    if (t.type == tok_right_paren) {
      break;
    }

    auto res = parse_expr(s, true);
    if (!res) {
      return pfail();
    }

    args.push_back(res);
    s = res;
    t = s;

    if (t.type == tok_comma) {
      s++;
      continue;
    } else {
      break;
    }
  }

  presult r;
  r.failed = false;
  r.vals = args;
  r.state = s;
  return r;
}



/**
 * One of the more complicated parsing functions. Basically turns the flat
 * representation of tokens into a tree based on the `parser_op_prec` map
 * as a precedence table
 */
static presult parse_binary_rhs(pstate s, int expr_prec, ast::node *lhs) {
  static const auto parser_op_prec = std::map<std::string, int>({
      {"=", 0},  {"+=", 0},  {"-=", 0},  {"*=", 0},  {"/=", 0}, {"||", 1},
      {"&&", 1}, {"^", 1},   {"==", 2},  {"!=", 2},  {"<", 10}, {"<=", 10},
      {">", 10}, {">=", 10}, {">>", 15}, {"<<", 15}, {"+", 20}, {"-", 20},
      {"*", 40}, {"/", 40},  {"%", 40},
  });
  while (true) {
    token tok = s;
    auto bin_op = tok.val;
    // check if the token is a binary operator or not
    bool is_binary_op = parser_op_prec.count(tok.val) != 0;
    if (!is_binary_op || tok.type == tok_eof) {
      return presult(lhs, s);
    }
    auto token_prec = parser_op_prec.at(tok.val);

    if (token_prec < expr_prec) {
      return presult(lhs, s);
    }
    // move to the next state
    s++;

    // right hand sides will never have a declaration, so pass false
    auto rhs = parse_expr(s, false);
    if (!rhs) {
      throw syntax_error(s, "binary expression missing right hand side");
      return pfail();
    }

    s = rhs;

    token t2 = s;

    if (parser_op_prec.count(t2.val) != 0) {
      auto next_prec = parser_op_prec.at(t2.val);
      if (token_prec < next_prec) {
        rhs = parse_binary_rhs(s, token_prec + 1, rhs);
        if (!rhs) {
          throw syntax_error(s, "malformed binary expression");
          return pfail();
        }
        s = rhs;
      }
    }

    token end = s;

    auto *n = new ast::binary_op();

    n->set_bounds(tok, end);

    n->op = bin_op;
    n->left = lhs;
    n->right = rhs;
    lhs = n;
  }
  return pfail();
}




/**
 * `do` is the main block node.
 */
static presult parse_do(pstate s) {
  // skip over the tok_do...
  s++;
  auto block = new ast::do_block();

  while (true) {
    s = glob_term(s);

    if (s.first().type == tok_end) {
      break;
    }
    auto res = parse_expr(s);
    if (!res) {
      throw syntax_error(s, "error in do-block");
    }
    s = res;
    for (auto e : res.vals) {
      block->exprs.push_back(e);
    }
  }
  s++;

  return presult(block, s);
}
