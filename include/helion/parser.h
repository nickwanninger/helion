// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __PARSER_H__
#define __PARSER_H__

#include <helion/pstate.h>
#include <functional>

#include <helion/ast.h>

namespace helion {




  struct presult {
    bool failed = false;
    std::vector<ast::node*> vals;
    pstate state;

    inline presult() {}
    inline presult(ast::node* v, pstate s) : state{s} { vals.push_back(v); }
    inline presult(ast::node* v) { vals.push_back(v); }

    inline operator ast::node*(void) {
      return vals.size() > 0 ? vals[0] : nullptr;
    }
    inline operator pstate(void) { return state; }
    inline operator bool(void) { return !failed; }

    inline text str(void) {
      if (operator bool()) {
        text t;
        for (size_t i = 0; i < vals.size(); i++) {
          auto& v = vals[i];
          if (v != nullptr) {
            t += v->str();
          } else {
            t += "NULL";
          }
          if (i < vals.size() - 1) t += ", ";
        }
        return t;
      } else {
        return "";
      }
    }
  };


  using parse_func = std::function<presult(pstate)>;

  inline presult pfail(void) {
    presult p;
    p.failed = true;
    return p;
  }

  inline parse_func options(std::vector<parse_func> funcs) {
    return [funcs](pstate s) -> presult {
      for (auto& fn : funcs) {
        auto r = fn(s);
        if (r) return r;
      }
      return pfail();
    };
  }
  inline parse_func sequence(std::vector<parse_func> funcs) {
    return [funcs](pstate s) -> presult {
      std::vector<ast::node*> emitted;
      for (auto& fn : funcs) {
        auto r = fn(s);
        if (r) {
          s = r.state;
          for (auto& v : r.vals) {
            emitted.push_back(v);
          }
        } else {
          return pfail();
        }
      }
      presult p;
      p.vals = emitted;
      p.state = s;
      return p;
    };
  }


  inline parse_func operator|(parse_func l, parse_func r) {
    return options({l, r});
  }
  inline parse_func operator&(parse_func l, parse_func r) {
    return sequence({l, r});
  }


  /**
   * parse the top level of a module, which is the finest grain parsing
   * publically exposed by the ast:: API. All parser functions are implemented
   * inside `src/helion/parser.cpp` as static functions
   */
  ast::module* parse_module(pstate);
  ast::module* parse_module(text &);




}  // namespace helion


#endif
