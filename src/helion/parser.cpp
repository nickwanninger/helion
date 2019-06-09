// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/ast.h>
#include <helion/parser.h>
#include <helion/pstate.h>
#include <atomic>
#include <map>




using namespace helion;


static std::atomic<int> next_type_num;


static text get_next_param_name(void) {
  text name = "Inferred_";
  name += std::to_string(next_type_num++);
  return name;
}


static std::shared_ptr<ast::type_node> get_next_param_type(scope *s) {
  auto t = std::make_shared<ast::type_node>(s);
  t->name = get_next_param_name();
  t->parameter = true;
  return t;
}



ast::module::module() { m_scope = std::make_unique<scope>(); }

scope *ast::module::get_scope(void) { return m_scope.get(); }


/**
 * This file is the implementation of <helion/parser.h> and is where the bulk
 * of the parsing logic lives. It's a hand-written recursive decent parser that
 * uses the `pstate` struct to allow `efficient` backtracking
 * TODO(optim): actually determine the efficiency of the pstate struct
 */

#define ADD_PARSER(name) static presult parse_##name(pstate, scope *);
ADD_PARSER(num);
ADD_PARSER(var);
ADD_PARSER(paren);
ADD_PARSER(str);
ADD_PARSER(keyword);
ADD_PARSER(do);
ADD_PARSER(nil);
ADD_PARSER(def);

static presult parse_expr(pstate, scope *, bool do_binary = true);
static presult parse_binary_rhs(pstate s, scope *, int expr_prec,
                                rc<ast::node> lhs);
static presult expand_expression(presult, scope *);
static presult parse_function_args(pstate, scope *);
static presult parse_function_literal(pstate, scope *);
static presult parse_return(pstate, scope *);
static presult parse_if(pstate, scope *);
static presult parse_typedef(pstate, scope *);
static presult parse_let(pstate, scope *);

static presult parse_type(pstate s, scope *sc);




static auto glob_term(pstate s) {
  while (s.first().type == tok_term) {
    s = s.next();
  }
  return s;
}




/**
 * primary parse function, ideally called per-file, but can be
 * called per-string
 */
std::unique_ptr<ast::module> helion::parse_module(pstate s) {
  auto mod = std::make_unique<ast::module>();

  mod->get_scope()->global = true;

  std::vector<std::shared_ptr<ast::node>> stmts;

  while (true) {
    // every time a top level expr is parsed, the scope
    // is reset to the top level scope
    s = glob_term(s);
    if (s.first().type == tok_eof) break;
    // while we can, parse a statement
    if (auto r = parse_expr(s, mod->get_scope()); r) {
      // inherit the state from the parser. This allows us to pick up right
      // after the end of the last parse_expr
      s = r.state;
      for (auto v : r.vals) {
        if (auto tn = std::dynamic_pointer_cast<ast::typedef_node>(v); tn) {
          mod->typedefs.push_back(tn);
        } else if (auto tn = std::dynamic_pointer_cast<ast::def>(v); tn) {
          mod->defs.push_back(tn);
        } else {
          stmts.push_back(v);
        }
      }

      // bool found = false;
      while (true) {
        if (token end = s; end.type == tok_term) {
          s++;
          // found = true;
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


  auto entry = std::make_shared<ast::def>(mod->get_scope());

  entry->fn = std::make_shared<ast::func>(mod->get_scope());
  mod->get_scope()->fn = entry->fn;

  // impossible function name
  entry->name = "#entry";

  entry->fn->stmts = stmts;

  mod->entry = entry;

  return mod;
}


/**
 * wrapper that creates a state around text
 */
std::unique_ptr<ast::module> helion::parse_module(text s, text pth) {
  auto t = make<tokenizer>(s, pth);
  pstate state(t, 0);
  return parse_module(state);
}



/**
 * primary expression parser
 */
static presult parse_expr(pstate s, scope *sc, bool do_binary) {
  token begin = s;
  auto res = pfail(s);

#define TRY(expr)                \
  {                              \
    res = expr;                  \
    if (res) goto parse_success; \
  }

  if (!res && begin.type == tok_num) TRY(parse_num(s, sc));
  // try to parse a function literal
  if (!res && begin.type == tok_left_paren) TRY(parse_function_literal(s, sc));
  if (!res && begin.type == tok_left_paren) TRY(parse_paren(s, sc));
  if (!res && begin.type == tok_var) TRY(parse_var(s, sc));
  if (!res && begin.type == tok_str) TRY(parse_str(s, sc));
  if (!res && begin.type == tok_keyword) TRY(parse_keyword(s, sc));
  if (!res && begin.type == tok_do) TRY(parse_do(s, sc));
  if (!res && begin.type == tok_return) TRY(parse_return(s, sc));
  if (!res && begin.type == tok_nil) TRY(parse_nil(s, sc));
  if (!res && begin.type == tok_if) TRY(parse_if(s, sc));
  if (!res && begin.type == tok_def) TRY(parse_def(s, sc));
  if (!res && begin.type == tok_typedef) TRY(parse_typedef(s, sc));
  if (!res && begin.type == tok_let) TRY(parse_let(s, sc));

parse_success:
  if (!res) {
    return pfail(s);
  }

  res = expand_expression(res, sc);
  if (do_binary && res) {
    return parse_binary_rhs(res, sc, -100, res);
  } else {
    return res;
  }
}

static presult expand_expression(presult r, scope *sc) {
  pstate initial_state = r;

  bool can_assign = false;
  while (true) {
    rc<ast::node> expr = r;
    pstate s = r;
    token t = s;


    token start_token = t;



    if (start_token.type == tok_is_type) {
      s++;
      auto type_res = parse_type(s, sc);
      if (!type_res) {
        throw syntax_error(s, "type assert expects a type");
      }

      s = type_res;
      auto n = std::make_shared<ast::typeassert>(sc);
      n->val = expr;
      n->type = type_res.as<ast::type_node>();

      r = presult(n, s);
      continue;
    }




    if (start_token.space_before) {
      break;
    }

    if (t.type == tok_dot) {
      s++;
      t = s;
      if (t.type == tok_var) {
        s++;
        auto v = make<ast::dot>(sc);
        v->set_bounds(start_token, t);
        v->expr = expr;
        v->sub = t.val;
        can_assign = true;
        r = presult(v, s);
        continue;
      }
    }

    if (t.type == tok_left_paren) {
      auto res = parse_function_args(s, sc);
      s = res;
      t = s;
      if (t.type != tok_right_paren) {
        throw syntax_error(initial_state, "malformed function call");
      }

      auto c = make<ast::call>(sc);
      c->set_bounds(start_token, t);
      c->func = expr;
      c->args = res.vals;
      s++;
      r = presult(c, s);
      can_assign = false;
      continue;
    }


    if (t.type == tok_left_square) {
      auto sub = make<ast::subscript>(sc);
      sub->expr = expr;
      s++;
      t = s;
      while (true) {
        t = s;
        if (t.type == tok_right_square) {
          break;
        }
        auto res = parse_expr(s, sc, true);
        if (!res) return pfail(s);

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
      can_assign = true;
      r = presult(sub, s);
      continue;
    }

    break;
  }

  pstate s = r;

  auto first = parse_expr(s, sc);

  if (!first) {
    return r;
  }

  // attempt to parse a paren-less function call
  // ie: expr expr [, expr]*
  rc<ast::node> expr = r;
  std::vector<rc<ast::node>> args;
  args.push_back(first);
  s = first;

  while (true) {
    if (s.first().type != tok_comma) {
      break;
    }
    // skip that comma
    s++;
    auto er = parse_expr(s, sc);
    if (er) {
      s = er;
      args.push_back(er);
    } else {
      throw syntax_error(s, "failed to parse function call");
    }
  }

  auto call = make<ast::call>(sc);
  call->func = expr;
  call->args = args;

  return presult(call, s);
}


static presult parse_var(pstate s, scope *sc) {
  auto v = make<ast::var>(sc);

  std::string name = s.first().val;
  auto found = sc->find(name);

  if (found == nullptr) {
    v->global = true;
    v->global_name = name;
  } else {
    v->decl = found;
  }

  return presult(v, s.next());
}



static presult parse_num(pstate s, scope *sc) {
  token t = s;
  std::string src = t.val;
  auto node = make<ast::number>(sc);
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


static presult parse_str(pstate s, scope *sc) {
  auto n = make<ast::string>(sc);
  n->val = s.first().val;
  s++;
  return presult(n, s);
}


static presult parse_keyword(pstate s, scope *sc) {
  auto n = make<ast::keyword>(sc);
  n->val = s.first().val;
  s++;
  return presult(n, s);
}



static presult parse_nil(pstate s, scope *sc) {
  auto n = make<ast::nil>(sc);
  s++;
  return presult(n, s);
}




/**
 * the paren parser will handle parsing (x) and (x, y) where
 * the first is simply x, and the second is the tuple (x,y)
 */
static presult parse_paren(pstate s, scope *sc) {
  auto init_state = s;
  s++;
  std::vector<rc<ast::node>> exprs;

  bool tuple = false;
  token t = s;

  while (true) {
    t = s;
    if (t.type == tok_right_paren) {
      break;
    }
    auto res = parse_expr(s, sc, true);
    if (!res) return pfail(s);

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

  rc<ast::node> n = nullptr;
  if (tuple) {
    auto tup = make<ast::tuple>(sc);
    tup->vals = exprs;
    n = tup;
  } else {
    n = exprs[0];
  }
  s++;

  return presult(n, s);
}




static presult parse_function_args(pstate s, scope *sc) {
  std::vector<rc<ast::node>> args;

  token t = s;
  if (t.type == tok_left_paren) s++;

  while (true) {
    t = s;
    /*t.type == tok_term || t.type == tok_eof || */
    if (t.type == tok_right_paren) {
      break;
    }

    auto res = parse_expr(s, sc, true);
    if (!res) {
      return pfail(s);
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
 *
 * It also detects assignment and if the lhs is a var, looks up the variable in
 * the scope. If it is not found, it must be a declaration so the function
 * returns a declaration
 */
static presult parse_binary_rhs(pstate s, scope *sc, int expr_prec,
                                rc<ast::node> lhs) {
  static const auto parser_op_prec = std::map<std::string, int>({
      {"=", 0},  {"+=", 0},  {"-=", 0},  {"*=", 0},  {"/=", 0}, {"||", 1},
      {"&&", 1}, {"^", 1},   {"==", 2},  {"!=", 2},  {"<", 10}, {"<=", 10},
      {">", 10}, {">=", 10}, {">>", 15}, {"<<", 15}, {"+", 20}, {"-", 20},
      {"*", 40}, {"/", 40},  {"%", 40},
  });

  /*
  static const auto assignment_ops = std::map<std::string, int>(
      {{"=", 0}, {"+=", 0}, {"-=", 0}, {"*=", 0}, {"/=", 0}});


  if (assignment_ops.count(s.first().val) != 0) {
    auto op = s.first().val;
    s++;
    auto rhs = parse_expr(s);

    if (!rhs) throw syntax_error(s, "failed to parse rhs of assignment");
    auto n = make<ast::binary_op>();
    n->op = op;
    n->left = lhs;
    n->right = rhs;
    return presult(n, rhs);
  }
  */

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
    auto rhs = parse_expr(s, sc, false);
    if (!rhs) {
      throw syntax_error(s, "binary expression missing right hand side");
      return pfail(s);
    }

    s = rhs;

    token t2 = s;

    if (parser_op_prec.count(t2.val) != 0) {
      auto next_prec = parser_op_prec.at(t2.val);
      if (token_prec < next_prec) {
        rhs = parse_binary_rhs(s, sc, token_prec, rhs);
        if (!rhs) {
          throw syntax_error(s, "malformed binary expression");
          return pfail(s);
        }
        s = rhs;
      }
    }

    token end = s;

    auto n = make<ast::binary_op>(sc);

    n->set_bounds(tok, end);

    n->op = bin_op;
    n->left = lhs;
    n->right = rhs;


    auto b = std::dynamic_pointer_cast<ast::binary_op>(lhs);
    if (false && b && bin_op == "=") {
      auto a = n;
      a->left = b->right;
      b->right = a;
      n = b;
    }


    lhs = n;
  }
  return pfail(s);
}




/**
 * `do` is the main block node.
 */
static presult parse_do(pstate s, scope *sc) {
  // get a new scope for the do block
  sc = sc->spawn();

  // skip over the tok_do...
  s++;
  auto block = make<ast::do_block>(sc);

  while (true) {
    s = glob_term(s);

    if (s.first().type == tok_end) {
      break;
    }
    auto res = parse_expr(s, sc);
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




/**
 * parse a type out into the special type representation.
 * This function absorbs the generic syntax as well
 */
static presult parse_type(pstate s, scope *sc) {
  bool constant = false;
  bool param = false;


  if (s.first().type == tok_some) {
    param = true;
    s++;
  }


  if (s.first().type == tok_const) {
    constant = true;
    s++;
  }

  std::shared_ptr<ast::type_node> type;



  auto open = tok_left_curly;
  auto close = tok_right_curly;

  // base case, actual type parsing
  if (s.first().type == tok_type) {
    if (s.first().val == "Fn") {
      // parse method type
      type = make<ast::type_node>(sc);
      type->constant = constant;
      type->name = s.first().val;
      type->style = type_style::METHOD;
      type->parameter = param;
      if (param)
        throw syntax_error(s, "cannot parameterize for some method type");
      if (constant)
        throw syntax_error(
            s, "methods are implicitly constant. No need for `const` keyword");

      s++;
      std::shared_ptr<ast::type_node> return_type = nullptr;
      std::vector<std::shared_ptr<ast::type_node>> types;
      if (s.first().type != open)
        throw syntax_error(s, "method type requires parameter information");
      // skip the open token
      s++;
      // normal comma separated list parsing...
      while (true) {
        if (s.first().type == close) {
          break;
        }
        if (s.first().type == tok_colon) {
          s++;
          auto ret_res = parse_type(s, sc);
          if (!ret_res)
            throw syntax_error(s,
                               "invalid return type parameter in method type");
          s = ret_res;
          return_type = ret_res.as<ast::type_node>();
          if (s.first().type != close)
            throw syntax_error(s, "unexpected token");
          break;
        }
        auto param_res = parse_type(s, sc);
        if (!param_res) throw syntax_error(s, "invalid type parameter");
        s = param_res;
        auto T = param_res.as<ast::type_node>();
        types.push_back(T);
        if (s.first().type == tok_comma) {
          s++;
          continue;
        }
      }
      s++;
      type->params.push_back(return_type);
      for (auto &t : types) type->params.push_back(t);
      goto FINALIZE;
    } else {
      type = make<ast::type_node>(sc);
      type->constant = constant;
      type->name = s.first().val;
      type->parameter = param;

      s++;


      if (s.first().type == open) {
        // skip the open token
        s++;
        // normal comma separated list parsing...
        while (true) {
          if (s.first().type == close) {
            break;
          }

          auto param_res = parse_type(s, sc);
          if (!param_res) throw syntax_error(s, "invalid type parameter");

          s = param_res;

          auto T = param_res.as<ast::type_node>();
          type->params.push_back(T);
          if (s.first().type == tok_comma) {
            s++;
            continue;
          }
        }
        s++;
      }

      goto FINALIZE;
    }
  }

  if (s.first().type == tok_left_square) {
    s++;

    auto tr = parse_type(s, sc);
    if (!tr) {
      throw syntax_error(s,
                         "Slice type needs a type within the square brackets");
    }

    s = tr;

    if (s.first().type != tok_right_square) {
      throw syntax_error(s, "unclosed square brackets");
    }

    s++;

    type = make<ast::type_node>(sc);
    type->name = "Slice";
    type->constant = constant;
    type->style = type_style::SLICE;
    type->params.push_back(tr.as<ast::type_node>());

    goto FINALIZE;
  }

  if (constant) {
    type = make<ast::type_node>(sc);
    type->constant = constant;
    goto FINALIZE;
  }

  return pfail(s);
FINALIZE:


  // absorb optional question marks
  if (s.first().type == tok_question) {

    throw syntax_error(s, "optional types not supported currently");
    auto opt = std::make_shared<ast::type_node>(sc);
    opt->style = type_style::OPTIONAL;
    opt->params.push_back(type);
    type = opt;
    s++;
  }

  return presult(type, s);
}


std::shared_ptr<ast::type_node> ast::parse_type(text src) {
  auto t = make<tokenizer>(src, "");
  pstate state(t, 0);
  scope s;
  auto res = ::parse_type(state, &s);
  return res.as<ast::type_node>();
}


/*
 * a function prototype is of the form...
 * 1) (args...) : return_type?
 * 2) args... : return_type?
 *
 * this means if we enter this function with a tok_left_paren,
 * we need to watch out for that as a closing symbol. If not, we need to not
 * look for it as it won't occur. If we find one, however, it needs to syntax
 * error...
 */
static presult parse_prototype(pstate s, scope *sc) {
  bool expect_closing_paren = s.first().type == tok_left_paren;

  // skip over the possible left paren
  if (expect_closing_paren) s++;


  auto proto = make<ast::prototype>(sc);

  std::shared_ptr<ast::type_node> return_type = nullptr;
  std::vector<std::shared_ptr<ast::type_node>> argument_types;

  while (true) {
    if (s.first().type == tok_term) {
      break;
    }

    if ((expect_closing_paren && s.first().type == tok_right_paren) ||
        s.first().type == tok_colon || s.first().type == tok_term) {
      break;
    }


    rc<ast::type_node> atype = nullptr;
    auto tres = parse_type(s, sc);


    if (tres) {
      atype = tres.as<ast::type_node>();
      s = tres;
    } else {
      atype = get_next_param_type(sc);
    }



    if (s.first().type != tok_var) {
      return pfail(s);
    }


    text name = s.first().val;
    s++;

    argument_types.push_back(atype);

    auto a = make<ast::var_decl>(sc);

    a->name = name;
    a->type = atype;
    a->is_arg = true;

    proto->args.push_back(a);

    if (s.first().type != tok_comma && s.first().type != tok_right_paren &&
        s.first().type != tok_term && s.first().type != tok_colon) {
      return pfail(s);
      throw syntax_error(s, "unexpected token");
    }
    if (s.first().type == tok_comma) s++;
  }

  if (expect_closing_paren) {
    if (s.first().type != tok_right_paren) {
      return pfail(s);
    }
    s++;
  }


  if (s.first().type == tok_colon) {
    s++;
    auto ret = parse_type(s, sc);
    if (ret) {
      s = ret;
      return_type = ret.as<ast::type_node>();
    }
  }

  for (auto a : proto->args) {
    sc->bind(a->name, a);
  }


  auto ftype = std::make_shared<ast::type_node>(sc);

  ftype->params.push_back(return_type);
  for (auto &p : argument_types) {
    ftype->params.push_back(p);
  }
  ftype->style = type_style::METHOD;

  proto->type = ftype;

  return presult(proto, s);
}



static presult parse_function_literal(pstate s, scope *sc) {
  pstate initial_state = s;


  auto fn = make<ast::func>(sc);

  // enter a new scope
  sc = sc->spawn();
  sc->fn = fn;

  // parse the prototype starting at a tok_left_paren
  auto protor = parse_prototype(s, sc);
  if (!protor) {
    return pfail(s);
  }

  auto proto = protor.as<ast::prototype>();

  s = protor;

  bool valid = false;

  if (s.first().type == tok_arrow) {
    s++;
    valid = true;
  }

  if (!valid) {
    return pfail(s);
  }


  s = glob_term(s);
  auto expr = parse_expr(s, sc);

  if (!expr) {
    throw syntax_error(s, "invalid lambda syntax");
  }

  s = expr;



  // fill in the function fields.
  fn->proto = proto;
  fn->stmts.push_back(expr);
  fn->anonymous = true;

  return presult(fn, s);
}


static presult parse_return(pstate s, scope *sc) {
  s++;

  auto ret = make<ast::return_node>(sc);

  if (s.first().type == tok_term) {
    return presult(ret, s);
  }

  auto es = parse_expr(s, sc);

  if (!es) {
    throw syntax_error(s, "");
  }
  s = es;
  ret->val = es;
  return presult(ret, s);
}


static presult parse_if(pstate s, scope *sc) {
  // conds is an array of conditions, basically each of the
  // mutually exclusive blocks in an if, elif, else chain
  // though the `else` block is handled differently, and has
  // no cond
  std::vector<ast::if_node::condition> conds;
  auto n = make<ast::if_node>(sc);
  auto start_token = s.first();

  while (s.first().type != tok_end) {
    // new scope for this condition
    auto ns = sc->spawn();

    ast::if_node::condition cond;
    if (s.first().type == tok_if || s.first().type == tok_elif) {
      s++;
      auto condr = parse_expr(s, ns);
      if (!condr) throw syntax_error(s, "invalid condition in if block");
      s = condr;
      cond.cond = condr.as<ast::node>();
    } else if (s.first().type == tok_else) {
      n->has_default = true;
      s++;
    }

    s = glob_term(s);
    if (s.first().type == tok_then) s++;
    s = glob_term(s);

    auto ntok = s.first().type;
    while (ntok != tok_elif && ntok != tok_else && ntok != tok_end) {
      auto expr_res = parse_expr(s, ns);

      if (!expr_res) throw syntax_error(s, "expected expression");

      s = expr_res;
      cond.body.push_back(expr_res);

      s = glob_term(s);

      ntok = s.first().type;
    }
    n->conds.push_back(cond);
  }

  n->set_bounds(start_token, s.first());
  s++;


  return presult(n, s);
}




static presult parse_def(pstate s, scope *sc) {
  auto n = make<ast::def>(sc);


  auto start_token = s.first();


  // enter a new scope and construct a function object to represent this def's
  // information
  sc = sc->spawn();
  n->fn = std::make_shared<ast::func>(sc);
  sc->fn = n->fn;

  s++;
  if (s.first().type == tok_var) {
    n->name = s.first().val;
  } else {
    throw syntax_error(s, "invalid name for function");
  }
  s++;

  auto protor = parse_prototype(s, sc);


  if (!protor) {
    throw syntax_error(s, "failed to parse def prototype");
  }

  n->fn->proto = protor.as<ast::prototype>();
  s = protor;
  s = glob_term(s);


  while (s.first().type != tok_end) {
    rc<ast::node> expr;

    s = glob_term(s);
    if (s.first().type == tok_then) s++;
    s = glob_term(s);

    auto ntok = s.first().type;
    while (ntok != tok_end) {
      auto expr_res = parse_expr(s, sc);

      if (!expr_res) throw syntax_error(s, "expected expression");

      s = expr_res;
      n->fn->stmts.push_back(expr_res);

      s = glob_term(s);

      ntok = s.first().type;
    }
    // n->expr.push_back(cond);
  }

  n->fn->set_bounds(start_token, s);
  n->fn->anonymous = false;
  n->set_bounds(start_token, s);
  s++;
  return presult(n, s);
}


static presult parse_typedef(pstate s, scope *sc) {
  auto n = make<ast::typedef_node>(sc);
  auto start_state = s;


  s++;

  auto typer = parse_type(s, sc);
  if (!typer)
    throw syntax_error(s, "Failed to parse type name in type definition");

  s = typer;
  n->type = typer.as<ast::type_node>();

  if (s.first().type == tok_extends) {
    s++;
    auto extendsr = parse_type(s, sc);
    if (!extendsr)
      throw syntax_error(
          s, "Failed to parse type name in type definition's extend statement");
    s = extendsr;
    n->extends = extendsr.as<ast::type_node>();
  }
  s = glob_term(s);

  while (s.first().type != tok_end) {
    if (s.first().type == tok_type) {
      auto typer = parse_type(s, sc);
      if (!typer)
        throw syntax_error(s, "failed to parse field type in type definition");
      s = typer;

      if (s.first().type != tok_var)
        throw syntax_error(s, "field name must be a variable name");
      auto name = s.first().val;
      s++;
      n->fields.push_back({.type = typer.as<ast::type_node>(), .name = name});
    } else if (s.first().type == tok_def) {
      auto defr = parse_def(s, sc);
      if (!defr)
        throw syntax_error(
            s, "failed to parse method definition in type definition");
      n->defs.push_back(defr.as<ast::def>());
      s = defr;
    } else {
      // if you get here, there's an invalid token in the type def
      throw syntax_error(s, "unexpected token in type definition");
    }
    s = glob_term(s);
  }

  n->set_bounds(start_state, s);
  s++;  // skip the 'end' token
  return presult(n, s);
}

static presult parse_let(pstate s, scope *sc) {
  // skip tok_let
  s++;

  auto decl = make<ast::var_decl>(sc);



  if (s.first().type == tok_global) {
    decl->global = true;
    s++;
  }

  if (sc->global) decl->global = true;

  // attempt to parse a type
  auto tp = parse_type(s, sc);




  bool has_type = false;
  if (tp) {
    has_type = true;
    decl->type = tp.as<ast::type_node>();
    s = tp;
  } else {
    decl->type = get_next_param_type(sc);
  }

  if (s.first().type != tok_var) {
    throw syntax_error(s, "unexpected token");
  }

  decl->name = s.first().val;
  s++;


  if (s.first().type == tok_assign) {
    s++;

    auto es = parse_expr(s, sc);

    if (!es) {
      throw syntax_error(
          s, "failed to parse expression on right hand side of `let`");
    }
    s = es;
    decl->value = es;
  } else if (!has_type) {
    throw syntax_error(
        s,
        "`let` syntax requires a type if you don't specify an initial value");
  }


  sc->bind(decl->name, decl);
  return presult(decl, s);
}
