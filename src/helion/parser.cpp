// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/ast.h>
#include <helion/parser.h>
#include <helion/pstate.h>


using namespace helion;


/**
 * This file is the implementation of <helion/parser.h> and is where the bulk
 * of the parsing logic lives. It's a hand-written recursive decent parser that
 * uses the `pstate` struct to allow `efficient` backtracking
 * TODO(optim): actually determine the efficiency of the pstate struct
 */

#define ADD_PARSER(name) static presult parse_##name(pstate);
ADD_PARSER(stmt);
ADD_PARSER(num);
ADD_PARSER(def);


ast::module *helion::parse_module(pstate s) {
  auto *module = new ast::module();

  while (true) {
    // while we can, parse a statement
    if (auto r = parse_stmt(s); r) {
      // inherit the state from the parser. This allows us to pick up right
      // after the end of the last parse_stmt
      s = r.state;
      for (auto v : r.vals) {
        module->stmts.push_back(v);
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



static presult parse_stmt(pstate s) {
  token begin = s;

  auto res = pfail();


#define HANDLE(TYPE) if (!res && begin.type == tok_##TYPE) res = parse_##TYPE(s);

  HANDLE(num);
  HANDLE(def);

  // if there was a successful parsing, we then check for tok_term
  // after.
  if (res) {
    bool found = false;
    while (true) {
      if (token end = res.state; end.type == tok_term) {
        res.state++;
        found = true;
      } else
        break;
    }

    if (!found) return pfail();
  }

  return res;
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


static presult parse_def(pstate s) {
  
  //
  token start = s;
  if (start.type == tok_def) {
  } else return pfail();

  return pfail();
}
