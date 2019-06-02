// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __PSTATE_H__
#define __PSTATE_H__

#include <helion/tokenizer.h>
#include <helion/util.h>
#include <memory>
#include <unordered_map>
#include <helion/core.h>

namespace helion {


  namespace ast {
    class node;
  };

  class scope {

   public:
    inline scope() { m_parent = nullptr; }

    inline scope &spawn() {
      auto ns = std::make_unique<scope>();
      ns->m_parent = this;
      scope *ptr = ns.get();
      children.push_back(std::move(ns));
      return *ptr;
    }

    inline std::shared_ptr<ast::node>& find(std::string &name) {
      // do a tree walking search, as variables can only be found in the current
      // scope and any scopes above it.
      if (m_vars.count(name) != 0) {
        return m_vars.at(name);
      }
      if (m_parent != nullptr) {
        return m_parent->find(name);
      }
      throw std::logic_error(strfmt("variable %s not bound", name.c_str()).c_str());
    }

    inline void bind(std::string &name, std::shared_ptr<ast::node>& node) {
      // very simple...
      m_vars[name] = node;
    }


   protected:
    scope *m_parent = nullptr;
    std::vector<std::unique_ptr<scope>> children;
    std::unordered_map<std::string, std::shared_ptr<ast::node>> m_vars;
  };

  class pstate {
    int ind = 0;
    std::shared_ptr<tokenizer> tokn;

   public:
    inline pstate() {
      ind = 0;
      tokn = nullptr;
    }
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
    inline pstate next(void) { return pstate(tokn, ind + 1); }
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
  };




}  // namespace helion


#endif
