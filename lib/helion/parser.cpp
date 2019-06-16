// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/ast.h>
#include <helion/core.h>
#include <helion/parser.h>
#include <helion/pstate.h>
#include <atomic>
#include <map>



using namespace helion;


static std::atomic<int> next_type_num;


static std::shared_ptr<ast::type_node> get_next_param_type(scope *s) {
  auto t = std::make_shared<ast::type_node>(s);
  t->name = get_next_param_name();
  t->parameter = true;
  return t;
}

ast::module::module() { m_scope = std::make_unique<scope>(); }

scope *ast::module::get_scope(void) { return m_scope.get(); }




static std::shared_ptr<ast::type_node> make_function_type(
    std::vector<std::shared_ptr<ast::type_node>> &a, scope *sc) {
  assert(a.size() >= 2);

  auto t = std::make_shared<ast::type_node>(sc);
  t->name = "->";
  t->params = {nullptr, a.back()};

  for (int i = a.size() - 2; i > 0; i--) {
    t->params[0] = a[i];
    auto nt = std::make_shared<ast::type_node>(sc);
    nt->name = "->";
    nt->params = {nullptr, t};
    t = nt;
  }

  t->params[0] = a[0];

  return t;
}



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

static presult parse_type(pstate s, scope *sc, bool follow_arrows = true);




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
        } else { /*
           if (false && auto tn = std::dynamic_pointer_cast<ast::var_decl>(v);
           tn) {
             // adding lets to the top level needs to do some special things...
             // First, we need to peel the value out of the decl, and put it in
             // an explicit assignment later on. this is so you can logically
             // use a variable before it is actually defined in the top level

             auto val = tn->value;
             tn->value = nullptr;

             mod->globals.push_back(tn);

             auto var = std::make_shared<ast::var>(tn->scp);
             var->decl = tn;
             auto assignment = std::make_shared<ast::binary_op>(tn->scp);
             assignment->left = var;
             assignment->right = val;
             assignment->op = "=";


             mod->stmts.push_back(assignment);
           } else {
           */
          mod->stmts.push_back(v);
          // }
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
      throw syntax_error(s, "unexpected token in module");
    }
  }


  /*
  auto entry = std::make_shared<ast::def>(mod->get_scope());

  entry->fn = std::make_shared<ast::func>(mod->get_scope());
  mod->get_scope()->fn = entry->fn;
  entry->fn->proto = std::make_shared<ast::prototype>(mod->get_scope());
  entry->fn->stmts = stmts;
  entry->fn->anonymous = true;

  mod->entry = entry;
  */

  return mod;
}


/**
 * wrapper that creates a state around text
 */
std::unique_ptr<ast::module> helion::parse_module(text s, text pth) {
  auto t = std::make_shared<tokenizer>(s, pth);
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
        auto v = std::make_shared<ast::dot>(sc);
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

      auto c = std::make_shared<ast::call>(sc);
      c->set_bounds(start_token, t);
      c->func = expr;
      c->args = res.vals;
      s++;
      r = presult(c, s);
      can_assign = false;
      continue;
    }


    if (t.type == tok_left_square) {
      auto sub = std::make_shared<ast::subscript>(sc);
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

  auto call = std::make_shared<ast::call>(sc);
  call->func = expr;
  call->args = args;

  return presult(call, s);
}


static presult parse_var(pstate s, scope *sc) {
  auto v = std::make_shared<ast::var>(sc);

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
  auto node = std::make_shared<ast::number>(sc);
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
  auto n = std::make_shared<ast::string>(sc);
  n->val = s.first().val;
  s++;
  return presult(n, s);
}


static presult parse_keyword(pstate s, scope *sc) {
  auto n = std::make_shared<ast::keyword>(sc);
  n->val = s.first().val;
  s++;
  return presult(n, s);
}



static presult parse_nil(pstate s, scope *sc) {
  auto n = std::make_shared<ast::nil>(sc);
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
    auto tup = std::make_shared<ast::tuple>(sc);
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
    auto n = std::make_shared<ast::binary_op>();
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

    auto n = std::make_shared<ast::binary_op>(sc);

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
  auto block = std::make_shared<ast::do_block>(sc);

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
static presult parse_type(pstate s, scope *sc, bool absorb_params) {
  std::shared_ptr<ast::type_node> type;

  static auto is_type_token = [](token t) -> bool {
    return t.type == tok_var || t.type == tok_left_paren ||
           t.type == tok_left_square;
  };


  std::string name;
  bool param = false;
  std::vector<std::shared_ptr<ast::type_node>> params;


  if (!is_type_token(s.first())) {
    return pfail(s);
  }


  if (s.first().type == tok_left_square) {
    s++;
    auto t = parse_type(s, sc);
    if (!t) {
      throw syntax_error(s, "failed to parse type");
    }
    s = t;
    if (s.first().type != tok_right_square)
      throw syntax_error(s, "missing closing right square bracket");

    s++;
    name = "List";
    params.push_back(t.as<ast::type_node>());
  } else if (s.first().type == tok_left_paren) {
    s++;
    // special tuple ident
    name = "()";

    while (s.first().type != tok_right_paren) {
      auto p = parse_type(s, sc);

      if (!p) throw syntax_error(s, "failed to parse type in parenthesis");
      s = p;
      params.push_back(p.as<ast::type_node>());
      if (s.first().type == tok_comma) s++;
    }
    s++;


    if (params.size() == 0)
      throw syntax_error(s, "empty parens in type invalid");

    if (params.size() == 1) {
      return presult(params[0], s);
    }

  } else if (s.first().type == tok_var) {
    name = s.first().val;


    // check for parameter-ness
    param = true;
    if (name[0] >= 'A' && name[0] <= 'Z') {
      param = false;
    }

    s++;


    // absorb any parameters
    while (absorb_params && is_type_token(s.first())) {
      if (param)
        throw syntax_error(s, "cannot parameterize on parameter types");

      auto t = parse_type(s, sc, false);
      if (!t) {
        throw syntax_error(s, "failed to parse type");
      }
      s = t;
      params.push_back(t.as<ast::type_node>());
    }
  }


  type = std::make_shared<ast::type_node>(sc);
  type->name = name;
  type->style = type_style::OBJECT;
  type->parameter = param;
  type->params = params;


  if (s.first().type == tok_arrow) {
    s++;
    auto ret = parse_type(s, sc);
    if (!ret) throw syntax_error(s, "failed to parse function type");
    s = ret;
    auto fn = std::make_shared<ast::type_node>(sc);
    fn->name = "->";
    fn->style = type_style::OBJECT;
    fn->params = {type, ret.as<ast::type_node>()};
    return presult(fn, s);
  }

  return presult(type, s);
}


std::shared_ptr<ast::type_node> ast::parse_type(text src) {
  auto t = std::make_shared<tokenizer>(src, "");
  pstate state(t, 0);
  scope s;
  auto res = ::parse_type(state, &s);
  if (!res) throw syntax_error(state, "failed to parse");
  auto tn = res.as<ast::type_node>();
  return tn;
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

  if (!expect_closing_paren) return pfail(s);

  // skip over the possible left paren
  if (expect_closing_paren) s++;


  auto proto = std::make_shared<ast::prototype>(sc);

  std::shared_ptr<ast::type_node> return_type = nullptr;

  std::vector<std::shared_ptr<ast::type_node>> argument_types;

  while (true) {
    if (s.first().type == tok_term) {
      return pfail(s);
    }

    if ((expect_closing_paren && s.first().type == tok_right_paren) ||
        s.first().type == tok_colon || s.first().type == tok_term) {
      break;
    }


    if (s.first().type != tok_var) {
      throw syntax_error(s, "unexpected argument name");
    }


    text name = s.first().val;
    s++;

    rc<ast::type_node> atype = nullptr;


    if (s.first().type == tok_colon) {
      s++;
      auto tres = parse_type(s, sc);
      if (tres) {
        atype = tres.as<ast::type_node>();
        s = tres;
      }
    }

    if (atype == nullptr) {
      atype = get_next_param_type(sc);
    }



    argument_types.push_back(atype);

    auto a = std::make_shared<ast::var_decl>(sc);

    a->name = name;
    a->type = atype;
    a->is_arg = true;

    proto->args.push_back(a);

    if (s.first().type != tok_comma && s.first().type != tok_right_paren &&
        s.first().type != tok_term && s.first().type != tok_colon) {
      return pfail(s);
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
  } else {
    return_type = get_next_param_type(sc);
  }

  for (auto a : proto->args) {
    sc->bind(a->name, a);
  }
  argument_types.push_back(return_type);
  proto->type = make_function_type(argument_types, sc);
  return presult(proto, s);
}



static presult parse_function_literal(pstate s, scope *sc) {
  pstate initial_state = s;


  auto fn = std::make_shared<ast::func>(sc);

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

  if (s.first().type == tok_fat_arrow) {
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

  if (sc->fn == nullptr) throw syntax_error(s, "unexpected return");

  auto ret = std::make_shared<ast::return_node>(sc);
  if (s.first().type == tok_term) {
    return presult(ret, s);
  }
  auto es = parse_expr(s, sc);
  if (!es) {
    throw syntax_error(s, "");
  }
  s = es;
  ret->val = es;
  sc->fn->returns.push_back(ret);
  return presult(ret, s);
}


static presult parse_if(pstate s, scope *sc) {
  // conds is an array of conditions, basically each of the
  // mutually exclusive blocks in an if, elif, else chain
  // though the `else` block is handled differently, and has
  // no cond
  std::vector<ast::if_node::condition> conds;
  auto n = std::make_shared<ast::if_node>(sc);
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
  auto n = std::make_shared<ast::var_decl>(sc);

  auto start_token = s.first();


  // enter a new scope and construct a function object to represent this def's
  // information
  sc = sc->spawn();
  auto fn = std::make_shared<ast::func>(sc);
  sc->fn = fn;

  s++;
  if (s.first().type == tok_var) {
    fn->name = s.first().val;
  } else {
    throw syntax_error(s, "invalid name for function");
  }
  s++;

  auto protor = parse_prototype(s, sc);



  if (!protor) {
    throw syntax_error(s, "failed to parse def prototype");
  }

  fn->proto = protor.as<ast::prototype>();
  s = protor;
  s = glob_term(s);



  while (s.first().type != tok_end) {
    rc<ast::node> expr;

    s = glob_term(s);

    auto ntok = s.first().type;
    while (ntok != tok_end) {
      auto expr_res = parse_expr(s, sc);

      if (!expr_res) throw syntax_error(s, "expected expression");

      s = expr_res;
      fn->stmts.push_back(expr_res);

      s = glob_term(s);

      ntok = s.first().type;
    }
    // n->expr.push_back(cond);
  }

  n->set_bounds(start_token, s);
  fn->anonymous = false;
  n->set_bounds(start_token, s);

  n->value = fn;
  n->name = fn->name;
  n->type = fn->proto->type;
  s++;
  return presult(n, s);
}


static presult parse_typedef(pstate s, scope *sc) {
  auto n = std::make_shared<ast::typedef_node>(sc);
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
    if (s.first().type == tok_type || s.first().type == tok_left_square) {
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
      std::string e = "unexpected token in type definition: ";
      e += s.first().val;
      // if you get here, there's an invalid token in the type def
      throw syntax_error(s, e);
    }
    s = glob_term(s);
  }

  n->set_bounds(start_state, s);
  s++;  // skip the 'end' token
  return presult(n, s);
}

static presult parse_let(pstate s, scope *sc) {
  // skip tok_let
  //
  auto start = s;
  s++;

  auto decl = std::make_shared<ast::var_decl>(sc);

  if (s.first().type == tok_global) {
    s++;
  }

  // if (sc->global) decl->global = true;

  if (s.first().type != tok_var) {
    throw syntax_error(s, "unexpected token");
  }

  decl->name = s.first().val;
  s++;

  bool has_type = false;
  if (s.first().type == tok_colon) {
    s++;

    // attempt to parse a type
    auto tp = parse_type(s, sc);


    if (tp) {
      has_type = true;
      decl->type = tp.as<ast::type_node>();
      s = tp;
    }
  }

  if (!has_type) {
    decl->type = get_next_param_type(sc);
  }




  if (s.first().type == tok_assign) {
    s++;

    auto es = parse_expr(s, sc);

    if (!es) {
      throw syntax_error(
          s, "failed to parse expression on right hand side of `let`");
    }
    s = es;
    decl->value = es;
  } else {
    throw syntax_error(start, "`let` must have an initial value");
  }

  decl->set_bounds(start, s);


  sc->bind(decl->name, decl);
  return presult(decl, s);
}
