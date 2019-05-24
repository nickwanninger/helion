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
ADD_PARSER(expr);
ADD_PARSER(num);
ADD_PARSER(def);
ADD_PARSER(paren);

static presult parse_binary_rhs(pstate s, int expr_prec, ast::node *lhs);



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

      if (s.first().type == tok_eof) {
        break;
      }
    } else
      break;
  }

  return module;
}


ast::module *helion::parse_module(text &s) {
  pstate state = s;
  return parse_module(state);
}



static presult parse_expr(pstate s) {
  token begin = s;

  auto res = pfail();


#define HANDLE(TYPE) \
  if (!res && begin.type == tok_##TYPE) res = parse_##TYPE(s);

#define TRY(expr)                \
  do {                           \
    res = expr;                  \
    if (res) goto parse_success; \
  } while (0);

  if (!res && begin.type == tok_num) TRY(parse_num(s));
  if (!res && begin.type == tok_left_paren) TRY(parse_paren(s));

parse_success:
  if (!res) {
    return pfail();
  }

  return parse_binary_rhs(res, -100, res);
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
      puts(tok, "not binary op", lhs->str());
      return presult(lhs, s);
    }
    auto token_prec = parser_op_prec.at(tok.val);

    if (token_prec < expr_prec) {
      puts(tok, " LESS THAN ");
      return presult(lhs, s);
    }
    // move to the next state
    s++;

    // right hand sides will never have a declaration, so pass false
    auto rhs = parse_expr(s);
    if (!rhs) {
      puts("fail");
      return pfail();
    }

    s = rhs;

    token t2 = s;

    if (parser_op_prec.count(t2.val) != 0) {
      auto next_prec = parser_op_prec.at(t2.val);
      if (token_prec < next_prec) {
        rhs = parse_binary_rhs(s, token_prec + 1, rhs);
        if (rhs) {
          return pfail();
        }
      }
    }
    s = rhs;

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

