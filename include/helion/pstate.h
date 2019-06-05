// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __PSTATE_H__
#define __PSTATE_H__

#include <helion/core.h>
#include <helion/tokenizer.h>
#include <helion/util.h>
#include <helion/ast.h>
#include <memory>
#include <unordered_map>

namespace helion {


  namespace ast {
    class node;
    class func;
  };  // namespace ast

  class scope {
   public:
    bool global = false;
    // a scope should know about the function it is based around
    std::shared_ptr<ast::func> fn;

    inline scope() { m_parent = nullptr; }
    inline scope *spawn() {
      auto ns = std::make_unique<scope>();
      ns->m_parent = this;
      ns->fn = fn;
      scope *ptr = ns.get();
      children.push_back(std::move(ns));
      return ptr;
    }

    inline std::shared_ptr<ast::var_decl> find(std::string &name) {
      // do a tree walking search, as variables can only be found in the current
      // scope and any scopes above it.
      if (m_vars.count(name) != 0) {
        return m_vars.at(name);
      }
      if (m_parent != nullptr) {
        return m_parent->find(name);
      }

      // not found
      return nullptr;
    }

    inline void bind(std::string name, std::shared_ptr<ast::var_decl> &node) {
      // very simple...
      m_vars[name] = node;
    }


    inline text str(int depth = 0) {
      text indent = "";
      text ic = "  ";
      for (int i = 0; i < depth; i++) indent += ic;

      text s;

      s += "{";
      s += "\"vars\": [";
      {
        size_t i = 0;
        for (auto &v : m_vars) {
          i++;
          s += "\"";
          s += v.first;
          s += "\"";
          if (i < m_vars.size()) s += ", ";
        }
      }
      s += "]";


      if (children.size() > 0) {
        s += ", \"children\": [";
        size_t i = 0;
        for (auto &c : children) {
          i++;
          s += c->str(depth + 1);
          if (i < children.size()) {
            s += ", ";
          };
        }
        s += "]";
      }
      s += "}";
      return s;
    }


   protected:
    scope *m_parent = nullptr;
    std::vector<std::unique_ptr<scope>> children;
    std::unordered_map<std::string, std::shared_ptr<ast::var_decl>> m_vars;
  };



  class pstate {
    int ind = 0;
    std::shared_ptr<tokenizer> tokn = nullptr;

   public:
    inline pstate() {}
    inline pstate(std::shared_ptr<tokenizer> t, int i = 0) {
      ind = i;
      tokn = t;
    }

    inline pstate(text src, text p, int i = 0) {
      ind = i;
      tokn = std::make_shared<tokenizer>(src, p);
    }

    inline text line(long ln) { return tokn->get_line(ln); }

    inline text path(void) { return tokn->get_path(); }

    inline token first(void) {
      static auto eof = token(tok_eof, "", nullptr, 0, 0);
      if (tokn != nullptr) {
        return tokn->get(ind);
      } else {
        return eof;
      }
    }
    inline pstate next(void) {
      auto p = pstate(tokn, ind + 1);
      return p;
    }
    inline bool done(void) { return first().type == tok_eof; }

    inline operator bool(void) { return !done(); }
    inline operator token(void) { return first(); }
    inline pstate operator++(int) {
      pstate self = *this;
      *this = next();
      return self;
    }



    inline pstate operator--(int) {
      pstate self = *this;
      ind--;
      return self;
    }


    inline pstate &operator=(pstate o) {
      tokn = o.tokn;
      ind = o.ind;
      return *this;
    }
  };




}  // namespace helion


#endif
