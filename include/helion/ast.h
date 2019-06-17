// [License]
// MIT - See LICENSE.md file in the package.
#pragma once

#ifndef __HELION_AST_H__
#define __HELION_AST_H__

#include <vector>
#include "core.h"
#include "iir.h"
#include "slice.h"
#include "text.h"
#include "tokenizer.h"
#include "util.h"


namespace llvm {
  class Value;
};

namespace helion {


  class scope;


  // forward declaration
  class cg_ctx;



  /**
   * the ast represents the code at a more abstract level, and it
   * is produced by <helion/parser.h>.
   */
  namespace ast {


#define NODE_FOOTER                                 \
 public:                                            \
  using node::node;                                 \
  iir::value *to_iir(iir::builder &, iir::scope *); \
  text str(int depth = 0);

    // @abstract, all ast::nodes extend from this publically
    class node {
     protected:
      token start;
      token end;


     public:
      scope *scp;

      node(scope *s) { scp = s; }
      virtual ~node() {}
      inline void set_bounds(token s, token e) {
        start = s;
        end = e;
      }

      /**
       * syntax error will generate a string error message that
       * can be thrown later on.
       */
      inline std::string syntax_error(std::string &msg) {
        std::string error;
        return error;
      }



      template <typename T>
      inline T as(void) {
        return dynamic_cast<T>(this);
      }

      virtual text str(int depth = 0) { return ""; };

      virtual inline iir::value *to_iir(iir::builder &, iir::scope *) {
        return nullptr;
      }
    };


    using node_ptr = rc<node>;


    class number : public node {
     public:
      enum num_type {
        integer,
        floating,
      };
      num_type type;
      union {
        int64_t integer;
        double floating;
      } as;
      NODE_FOOTER;
    };

    class binary_op : public node {
     public:
      node_ptr left;
      node_ptr right;
      text op;
      NODE_FOOTER;
    };

    class dot : public node {
     public:
      node_ptr expr;
      text sub;
      NODE_FOOTER;
    };

    class subscript : public node {
     public:
      node_ptr expr;
      std::vector<node_ptr> subs;
      NODE_FOOTER;
    };



    class call : public node {
     public:
      node_ptr func;
      std::vector<node_ptr> args;
      NODE_FOOTER;
    };

    class tuple : public node {
     public:
      std::vector<node_ptr> vals;
      NODE_FOOTER;
    };


    class string : public node {
     public:
      text val;
      NODE_FOOTER;
    };


    class keyword : public node {
     public:
      text val;
      NODE_FOOTER;
    };

    class nil : public node {
     public:
      NODE_FOOTER;
    };



    class do_block : public node {
     public:
      std::vector<node_ptr> exprs;
      NODE_FOOTER;
    };



    class return_node : public node {
     public:
      node_ptr val;
      NODE_FOOTER;
    };


    /**
     * a type_node represents the textual representation of a type
     */
    class type_node : public node {
     public:
      bool constant = false;
      bool parameter = false;

      text name;

      type_style style = type_style::OBJECT;
      // type parameters, like Vector{Int} where Int would live in here.
      std::vector<rc<type_node>> params;

      NODE_FOOTER;
    };




    class var_decl : public node {
     public:
      var_decl(scope *s);

      bool global = false;
      int ind = 0;
      bool is_arg = false;
      std::shared_ptr<type_node> type;
      text name;
      std::shared_ptr<ast::node> value;
      text str(int = 0);
      iir::value *to_iir(iir::builder &, iir::scope *);
    };


    class var : public node {
     public:
      bool global = false;
      text global_name;
      std::shared_ptr<var_decl> decl;
      NODE_FOOTER;
    };




    // represents the prototype of a function. Types can be derived from this
    class prototype : public node {
     public:
      std::vector<std::shared_ptr<var_decl>> args;
      std::shared_ptr<type_node> type;
      // rc<type_node> return_type;
      NODE_FOOTER;
    };


    // represents function info. Basically, a common representation
    // of escaping variables, closed variables, prototypes, etc..
    class func : public node {
     public:
      // a vector of the variables which this function captures
      std::vector<std::shared_ptr<ast::var_decl>> captures;
      std::shared_ptr<prototype> proto = nullptr;
      std::vector<std::shared_ptr<ast::node>> stmts;
      std::vector<std::shared_ptr<ast::return_node>> returns;
      std::string name = "";
      bool anonymous = false;
      NODE_FOOTER;
    };

    class def : public node {
     public:
      std::shared_ptr<func> fn;
      NODE_FOOTER;
    };


    class if_node : public node {
     public:
      std::shared_ptr<ast::node> cond;
      std::shared_ptr<ast::node> true_expr;
      std::shared_ptr<ast::node> false_expr = nullptr;
      NODE_FOOTER;
    };



    class typedef_node : public node {
     public:
      struct field_t {
        std::shared_ptr<ast::type_node> type;
        text name;
      };
      std::shared_ptr<type_node> type;
      std::shared_ptr<type_node> extends;
      std::vector<field_t> fields;
      std::vector<std::shared_ptr<ast::def>> defs;

      NODE_FOOTER;
    };



    class typeassert : public node {
     public:
      std::shared_ptr<ast::node> val;
      std::shared_ptr<ast::type_node> type;
      NODE_FOOTER;
    };


    /**
     * a module AST node is what comes from parsing any top level expression,
     * string, or other representation. Technically, we parse a module per file
     * in a module directory, then merge them together.
     *
     * Implemented in parser.cpp
     */
    class module {
     private:
      std::unique_ptr<scope> m_scope;

     public:
      module();

      std::vector<std::shared_ptr<var_decl>> globals;
      std::vector<std::shared_ptr<ast::typedef_node>> typedefs;
      // stmts are top level expressions that will eventually be ran before main
      std::vector<std::shared_ptr<ast::node>> stmts;


      scope *get_scope(void);
      text str(int = 0);
    };




    std::shared_ptr<ast::type_node> parse_type(text);


  }  // namespace ast

}  // namespace helion

#endif
