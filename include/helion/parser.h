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
    using node_ptr = std::shared_ptr<ast::node>;

    bool failed = false;
    std::vector<node_ptr> vals;
    pstate state;

    inline presult() {}
    inline presult(node_ptr v, pstate s) : state{s} { vals.push_back(v); }
    inline presult(node_ptr v) { vals.push_back(v); }

    inline operator node_ptr(void) {
      return vals.size() > 0 ? vals[0] : nullptr;
    }


    template<typename T>
    inline rc<T> as(void) {
      return std::dynamic_pointer_cast<T>(vals[0]);
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

  inline presult pfail(pstate s) {
    presult p;
    p.state = s;
    p.failed = true;
    return p;
  }

  inline parse_func options(std::vector<parse_func> funcs) {
    return [funcs](pstate s) -> presult {
      for (auto& fn : funcs) {
        auto r = fn(s);
        if (r) return r;
      }
      return pfail(s);
    };
  }
  inline parse_func sequence(std::vector<parse_func> funcs) {
    return [funcs](pstate s) -> presult {
      std::vector<std::shared_ptr<ast::node>> emitted;
      for (auto& fn : funcs) {
        auto r = fn(s);
        if (r) {
          s = r.state;
          for (auto& v : r.vals) {
            emitted.push_back(v);
          }
        } else {
          return pfail(s);
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
  std::unique_ptr<ast::module> parse_module(pstate);
  std::unique_ptr<ast::module> parse_module(text, text);



  class syntax_error : public std::exception {
    std::string _msg;

   public:

    long line;
    long col;

    inline syntax_error(pstate s, text msg) {
      token tok = s;
      _msg += s.path();
      _msg += " (";
      _msg += std::to_string(tok.line+1);
      _msg += ":";
      _msg += std::to_string(tok.col+1);
      _msg += ") ";
      _msg += "error: ";
      _msg += msg; 
      _msg += "\n";


      text indent = "  | ";

      _msg += indent;
      _msg += s.line(tok.line);
      _msg += "\n";

      _msg += indent;
      for (int i = 0; i < tok.col-1; i++) {
        _msg += " ";
      }
      _msg += "^\n\n";

    }
    inline const char* what() const throw() {
      // simply pull the value out of the msg
      return _msg.c_str();
    }
  };


}  // namespace helion


#endif
