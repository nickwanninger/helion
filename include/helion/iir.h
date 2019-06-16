// [License]
// MIT - See LICENSE.md file in the package.
#pragma once

#ifndef __HELION_IIR_H__
#define __HELION_IIR_H__

#include <stdint.h>
#include <unistd.h>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include "gc.h"
#include "slice.h"


namespace helion {


  namespace ast {
    class type_node;
    class func;
  };  // namespace ast


  /**
   * iir: intermediate intermediate representation :)
   *
   * Basically a stage between the AST and llvm. Most type checking and
   * method expansion happens here.
   */
  namespace iir {


    // forward declarations
    class named_type;
    class var_type;
    class value;
    class scope;
    class func;

    enum class type_type : char { invalid, named, method, var };

    class type {
      type_type t = type_type::invalid;

     public:
      inline bool is_named(void) { return t == type_type::named; };
      inline bool is_var(void) { return t == type_type::var; };

      named_type *as_named(void);
      var_type *as_var(void);
      inline virtual std::string str(void) = 0;
    };

    class named_type : public type {
      type_type t = type_type::named;

     public:
      std::string name;
      std::vector<type *> params;

      inline named_type(std::string name) : name(name) {}
      inline named_type(std::string name, std::vector<type *> params)
          : name(name), params(params) {}
      std::string str(void);
    };


    class var_type : public type {
      type_type t = type_type::var;

     public:
      std::string name;
      type &points_to = *this;
      inline var_type(std::string name) : name(name) {}
      std::string str(void);
    };

    var_type &new_variable_type(void);


    bool operator==(type &, type &);
    inline bool operator!=(type &a, type &b) { return !(a == b); }




    // defined in typesystem.cpp
    type &convert_type(std::shared_ptr<ast::type_node>);




    class module;

    class scope {
     protected:
      std::unordered_map<std::string, value *> m_bindings;
      scope *m_parent;
      std::vector<std::unique_ptr<scope>> children;

     public:
      module *mod;

      inline scope *spawn() {
        auto p = gc::make_collected<scope>();
        p->m_parent = this;
        p->mod = mod;
        return p;
      }
      inline value *find_binding(std::string &name) {
        auto *sc = this;
        while (sc != nullptr) {
          if (sc->m_bindings.count(name) != 0) {
            return sc->m_bindings[name];
          }
          sc = sc->m_parent;
        }
        return nullptr;
      }
      inline void bind(std::string name, value *v) { m_bindings[name] = v; }
    };



    class value {
     protected:
      type *ty = nullptr;
      std::string name;

     public:
      type &get_type(void);
      void set_type(type &);

      virtual void print(std::ostream &, bool just_name = false, int = 0){};


      inline void set_name(std::string name) { this->name = name; }
      inline std::string get_name(void) { return name; }
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
      br,
      jmp,
      global,
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
      cast,
      poparg,
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
      instruction(block &, inst_type, type &, slice<value *>);
      instruction(block &, inst_type, type &);

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

     public:
      std::string name = "";
      bool intrinsic = false;

      std::shared_ptr<ast::func> node = nullptr;
      func(module &);

      int next_uid(void);
      block *new_block(void);
      void add_block(block *b);
      void print(std::ostream &, bool = false, int = 0);
    };



    // an iir module is what is produced out of a
    class module : public scope {
      std::string name;
      scope *mod_scope;
      module *mod = this;

     public:
      std::vector<value *> globals;

      module(std::string name);
      // creates a function
      func *create_func(std::shared_ptr<ast::func>);
      // create an intrinsic function which will call to a special part of the
      // compiler once we get to this stage
      func *create_intrinsic(std::string name, std::shared_ptr<ast::type_node>);
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
      instruction *create_inst(inst_type, type &);
      instruction *create_inst(inst_type, type &, slice<value *>);



     public:
      builder(func &);
      void set_target(block *);

      value *create_alloc(type &);
      value *create_global(type &);
      value *create_poparg(type &);
      void create_store(value *, value *);
      value *create_load(value *);


      void create_ret(value *);

      value *create_binary(inst_type, value *, value *);
      void create_branch(value *, block *, block *);

      void create_jmp(block *);


      inline void insert_block(block *b) { current_func.add_block(b); }
      inline block *new_block(void) { return current_func.new_block(); }
    };




  }  // namespace iir
}  // namespace helion



namespace std {
  template <>
  struct hash<helion::iir::type> {
    size_t operator()(helion::iir::type &t) const {
      size_t x = 0;
      size_t y = 0;
      size_t mult = 1000003UL;  // prime multiplier

      x = 0x345678UL;
      if (t.is_var()) {
        auto v = t.as_var();
        x = 0x098172354UL;
        x ^= std::hash<std::string>()(v->name);
        return x;
      } else if (t.is_named()) {
        auto v = t.as_named();
        x = 0x856819292UL;
        x ^= std::hash<std::string>()(v->name);
        for (auto *p : v->params) {
          y = operator()(*p);
          x = (x ^ y) * mult;
          mult += (size_t)(852520UL + 2);
        }
        return x;
      }

      return x;
    }
  };


  template <>
  struct hash<helion::iir::type *> {
    size_t operator()(helion::iir::type *t) const { return std::hash<helion::iir::type>()(*t); }
  };
}  // namespace std
#endif
