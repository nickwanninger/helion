// [License]
// MIT - See LICENSE.md file in the package.
#pragma once

#ifndef __HELION_IIR_H__
#define __HELION_IIR_H__

#include <stdint.h>
#include <unistd.h>
#include "slice.h"
#include <set>

namespace helion {

  // forward declare for quicker compiles (I guess)
  struct datatype;


  /**
   * iir: intermediate intermediate representation :)
   *
   * Basically a stage between the AST and llvm. Most type checking and
   * method expansion happens here.
   */
  namespace iir {




    // an iir module is what is produced out of a
    class module {};


    class value {
     protected:
      datatype *ty;

     public:
      datatype *get_type(void);
      void set_type(datatype *);

      virtual void print(std::ostream &, bool just_name = false, int = 0){};
    };


    class const_int : public value {
     public:
      size_t val;

      inline void print(std::ostream &s, bool = false, int = 0) {
        s << std::to_string(val);
      }
    };

    class const_flt : public value {
     public:
      double val;
      inline void print(std::ostream &s, bool = false, int = 0) {
        s << std::to_string(val);
      }
    };


    value *new_int(size_t);
    value *new_float(double);



    enum class inst_type : char {
      unknown = 0,
      ret,
      branch,
      call,
      add,
      sub,
      mul,
      div,
      invert,
      dot,
      alloc,
      load,
      store,
      cast
    };

    const char *inst_type_to_str(inst_type);



    class func;
    class block;

    class builder;

    /*
     * an instruction is what is contained inside a block
     */
    class instruction : public value {
     protected:
      inst_type itype;
      block &bb;
      int uid;

     public:
      slice<value *> args;
      instruction(block &, inst_type, datatype *, slice<value *>);
      instruction(block &, inst_type, datatype *);

      inline inst_type get_inst_type(void) { return itype; }

      void print(std::ostream &, bool = false, int = 0);
    };



    class block : public value {
     protected:
      friend builder;
      slice<instruction *> insts;
      friend instruction;
      friend func;
      int id = 0;
      func &fn;

     public:
      block(func &);
      inline int get_id(void) { return id; }
      void add_inst(instruction *);
      void print(std::ostream &, bool = false, int = 0);
    };




    class func : public value {
     private:
      module &mod;
      // instructions have to have unique ids, so it comes from here
      int uid = 0;

     protected:
      friend builder;
      slice<block *> blocks;

      // return types are determined as the code is compiled,
      // so this set is here to keep track of those
      std::set<datatype*> return_types;

     public:
      func(module &);

      int next_uid(void);
      block *new_block(void);
      void print(std::ostream &, bool = false, int = 0);
    };

    // A builder is used to add instructions to a block
    //
    // implemented in irbuilder.cpp
    //
    class builder {
     protected:
      friend instruction;
      func &current_func;
      block *target = nullptr;



      instruction *add_inst(instruction *);
      instruction *create_inst(inst_type, datatype *);
      instruction *create_inst(inst_type, datatype *, slice<value *>);

     public:
      builder(func &);
      void set_target(block *);

      value *create_alloc(datatype *);
      void create_ret(value *);
    };


  }  // namespace iir
}  // namespace helion

#endif
