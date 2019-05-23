// [License]
// MIT - See LICENSE.md file in the package.
#pragma once

#ifndef __HELION_AST_H__
#define __HELION_AST_H__

#include <helion/text.h>
#include <helion/tokenizer.h>
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
  // void codegen(void);

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

      virtual text str(int depth = 0) {
        throw std::logic_error("call .str() on base node type");
      };

      // virtual void codegen(void);
    };


    /**
     * a module AST node is what comes from parsing any top level expression,
     * string, or other representation. Technically, we parse a module per file
     * in a module directory, then merge them together
     */
    class module : public node {
     public:
      std::vector<node *> stmts;
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




    class type : public node {
     public:
      text name;
    };

    class lambda;
    /**
     * an argument is a dumb representation of `Type name` in a function
     * signature. If there is no type, then it's implicity `Any` which means
     * the function will be
     */
    class argument : public node {
     public:
      lambda *owner;
      bool has_type = false;
      text type;
      text name;
      NODE_FOOTER;
    };


    class lambda : public node {
     public:
      // if this value is true, then the 'lambda' node is inside a def block.
      bool of_def = false;
      NODE_FOOTER;
    };


    class def : public node {
     public:
      node *dst;
      lambda *func;
      std::vector<argument *> args;
      type *return_type;


      NODE_FOOTER;
    };



  }  // namespace ast

}  // namespace helion

#endif
