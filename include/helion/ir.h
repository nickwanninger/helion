// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __HELION_IR__
#define __HELION_IR__

#include <helion/text.h>
#include <memory>
#include <vector>


// the helion IR is what is targeted by the ast and it sits between the AST and
// LLVM It represents the type checked and scope checked representation of the
// program

namespace helion {
  namespace ir {
    // much like LLVM, the base class in the IR is the value. All helion
    // expressions evaluate to an ir::value.
    class value {
     public:
      // virtual destructor
      virtual ~value() {}

      // pure virtual function that stringifies the value.
      // must be implemented on all child classes
      virtual text str(void) = 0;
    };

    // instructions live in basic blocks
    class instruction {
     public:
      virtual text str(void) = 0;
    };




    // basic blocks are not values, and they are where expressions compile into.
    // For example, an if statement will generate 2 basic blocks, the body of
    // the if statement and one for the join, which is jumped to if false.
    class basic_block {
     public:
      text name;
      // the previous block in this function, if it is null, this block is the
      // first
      std::shared_ptr<basic_block> previous = nullptr;
      std::shared_ptr<basic_block> next = nullptr;

      //
      std::vector<std::shared_ptr<ir::instruction>> insts;

      inline text str(void) {
        text s;
        s += name;
        s += ":\n";
        for (auto& in : insts) {
          s += "  ";
          s += in->str();
          s += "\n";
        }
        return s;
      }
    };

    class function_builder {
     public:
      // functions have a list of basic blocks. The order of this list is not
      // illustrative of the ordering of basic blocks within the function, it
      // is simply a list of the blocks within the function so they can be freed
      // later on upon dtor
      std::vector<std::shared_ptr<basic_block>> blocks;

    };


    class branch_inst : public instruction {
      public:
        std::shared_ptr<basic_block> to;
        inline text str(void) {
          text s;
          s += "branch ";
          s += to->name;
          return s;
        }
    };


  };  // namespace ir
};    // namespace helion

#endif
