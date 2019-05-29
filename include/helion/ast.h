// [License]
// MIT - See LICENSE.md file in the package.
#pragma once

#ifndef __HELION_AST_H__
#define __HELION_AST_H__

#include <helion/text.h>
#include <helion/tokenizer.h>
#include <helion/util.h>
#include <vector>

namespace helion {
  /**
   * the ast represents the code at a more abstract level, and it
   * is produced by <helion/parser.h>.
   */
  namespace ast {



#define NODE_FOOTER        \
 public:                   \
  text str(int depth = 0); \

    // @abstract, all ast::nodes extend from this publically
    class node {
     protected:
      token start;
      token end;

     public:
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

      virtual text str(int depth = 0) { return ""; };

      // virtual void codegen(void);
    };


    using node_ptr = rc<node>;

    /**
     * a module AST node is what comes from parsing any top level expression,
     * string, or other representation. Technically, we parse a module per file
     * in a module directory, then merge them together
     */
    class module : public node {
     public:
      std::vector<node_ptr> stmts;
      NODE_FOOTER;
    };


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

    class var : public node {
     public:
      text value;
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
      // what kind of type node is this
      enum type_node_type {
        NORMAL_TYPE,    // normal representation, so Int, Float, etc..
        FUNCTION_TYPE,  // function type, so Fn(Int) -> Int for example
        SLICE_TYPE,     // slice types, so [T] where T is another type
      };

      text name;

      type_node_type type = NORMAL_TYPE;
      // type parameters, like Vector{Int} where Int would live in here.
      std::vector<rc<type_node>> params;

      NODE_FOOTER;
    };

    // represents the prototype of a function. Types can be derived from this
    class prototype : public node {
     public:
      // represents a typed argument
      struct argument {
        // a pointer to the type of the argument. null here means it's unknown
        // and must be inferred
        rc<type_node> type = nullptr;
        // the name of the argument is simply a string
        text name;
      };

      std::vector<argument> args;

      rc<type_node> return_type;

      NODE_FOOTER;
    };

    class func : public node {
     public:
      rc<prototype> proto = nullptr;
      node_ptr body;
      NODE_FOOTER;
    };


    class if_node : public node {
      public:
        struct condition {
          std::shared_ptr<ast::node> cond;
          std::vector<std::shared_ptr<ast::node>> body;
        };

        bool has_default = false;

        std::vector<condition> conds;
        NODE_FOOTER;
    };


  }  // namespace ast

}  // namespace helion

#endif
