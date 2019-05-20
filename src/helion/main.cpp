// [License]
// MIT - See LICENSE.md file in the package.

#include <helion.h>
#include <stdio.h>
#include <iostream>
#include <memory>
#include <gc/gc.h>
#include <unordered_map>
#include <vector>

using namespace helion;

// pgas

class array_literal : public node {
 public:
  std::vector<node *> vals;

  inline text str(void) {
    text t;
    t += "[";
    for (size_t i = 0; i < vals.size(); i++) {
      auto &v = vals[i];
      if (v != nullptr) {
        t += v->str();
      } else {
        t += "UNKNOWN!";
      }
      if (i < vals.size() - 1) t += ", ";
    }
    t += "]";
    return t;
  }
};

class number_literal : public node {
 public:
  text num;
  inline text str(void) { return num; }
};

class var_literal : public node {
 public:
  text var;
  inline text str(void) { return var; }
};



class string_literal : public node {
 public:
  text val;
  inline text str(void) {
    text s;
    s += "'";
    s += val;
    s += "'";
    return s;
  }
};

presult parse_num(pstate);
presult parse_var(pstate);
presult parse_string(pstate);
presult parse_array(pstate);

auto parse_expr = options({parse_num, parse_var, parse_string, parse_array});

parse_func accept(enum tok_type t) {
  return [t](pstate s) -> presult {
    if (s.first().type == t) {
      presult p;
      p.state = s.next();
      return p;
    }
    return pfail();
  };
}

auto epsilon = [](pstate s) {
  presult r;
  r.state = s;
  return r;
};


presult parse_num(pstate s) {
  token t = s;
  if (t.type == tok_num) {
    auto *n = new number_literal();
    n->num = t.val;
    return presult(n, s.next());
  } else {
    return pfail();
  }
}

presult parse_var(pstate s) {
  token t = s;
  if (t.type == tok_var) {
    auto *n = new var_literal();
    n->var = t.val;
    return presult(n, s.next());
  } else {
    return pfail();
  }
}


presult parse_string(pstate s) {
  token t = s;
  if (t.type == tok_str) {
    auto *n = new string_literal();
    n->val = t.val;
    return presult(n, s.next());
  } else {
    return pfail();
  }
}




/**
 * list grammar is as follows:
 * list -> [ list_body ]
 * list_body -> expr list_tail | _
 * list_tail -> , list_body | _
 */
// decl
presult p_array_tail(pstate);
presult p_array_body(pstate);
// impl
presult p_array_body(pstate s) {
  return ((parse_expr & p_array_tail) | (parse_func)epsilon)(s);
}
presult p_array_tail(pstate s) {
  return ((accept(tok_comma) & p_array_body) | (parse_func)epsilon)(s);
}

presult parse_array(pstate s) {
  auto p_list =
      accept(tok_left_square) & p_array_body & accept(tok_right_square);
  auto res = p_list(s);
  array_literal *ln = new array_literal();
  ln->vals = res.vals;
  return presult(ln, res.state);
}







int main(int argc, char **argv) {
  // print every token from the file
  text src = read_file(argv[1]);
  tokenizer s(src);
  for (pstate s = src; s; s++) {
    puts(s.first());
  }
  puts("-------------------------------------");
  auto res = parse_expr(src);
  puts("res:", res.str());
  return 0;
}
