// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __HELION_CORE_JIT__
#define __HELION_CORE_JIT__

#include <memory>
#include <vector>



#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"

// All the IR libs that are needed
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
// Various transformations
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Vectorize.h"

#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"



#include <memory>
#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

#include <helion/text.h>
#include <flat_hash_map.hpp>
#include <mutex>
#include <unordered_map>
#include "slice.h"
#include "util.h"


/*
 * this header file defines the core classes and data types used throughout the
 * helion compiler and jit runtime. For example, the module type, the basic type
 * classes, etc...
 */

namespace helion {


  namespace ast {
    class node;
    class typedef_node;
    class module;
    class type_node;

    class def;
    class func;
  };  // namespace ast



  namespace iir {
    class module;
  };

  extern llvm::LLVMContext llvm_ctx;

  struct binding;
  struct datatype_name;
  struct datatype;

  /*
    extern datatype *any_type;
    extern datatype *bool_type;
    extern datatype *int8_type;
    extern datatype *int16_type;
    extern datatype *int32_type;
    extern datatype *int64_type;
    extern datatype *integer_type;
    extern datatype *float32_type;
    extern datatype *float64_type;
    extern datatype *datatype_type;
    extern datatype *generic_ptr_type;
    */


  // a value is an opaque pointer to something garbage collected in the helion
  // jit runtime. It has no real meaning except as a better, typed replacement
  // for voidptr
  // struct value;



  // global_binding is an opaque pointer to a global variable binding which is
  // managed as an LLVM type later on.
  // struct global_binding;

  // we use LLVM structs to represent global variables, so we have to JIT some
  // LLVM functions to access the fields of the struct. Therefore, we have
  // global variable bindings to those functions after they are generated :)
  // TODO: please find a way to not do this.
  // extern std::function<global_binding *()> create_global_binding;



  class cg_scope;

  enum class type_style : unsigned char {
    OBJECT,    // normal reference type
    INTEGER,   // is an n bit integer
    FLOATING,  // is an n bit floating point number
    UNION,     // is a union of the parameters
    TUPLE,
    METHOD,    // first parameter is the return type, then each other parameter
               // is an argument
    SLICE,     // the first parameter is the type of the slice. Only one param
               // allowed
    OPTIONAL,  // first param is the type it is an optional of
    STRUCT,  // representation of a pure c struct, mainly for mirroring runtime
             // types
  };


  // forward decl of codegen structs
  struct cg_binding;
  /**
   * Represents the context for a single method compilation
   */
  class cg_ctx {
   public:
    llvm::IRBuilder<> builder;
    llvm::Function *func = nullptr;
    cg_ctx(llvm::LLVMContext &llvmctx) : builder(llvmctx) {}
  };




  struct cg_binding {
    std::string name;
    datatype *type;
    llvm::Value *val;
  };

  class cg_scope {
   protected:
    ska::flat_hash_map<std::string, datatype *> m_types;
    ska::flat_hash_map<std::string, llvm::Value *> m_bindings;
    ska::flat_hash_map<llvm::Value *, datatype *> m_val_types;
    cg_scope *m_parent;

    std::vector<std::unique_ptr<cg_scope>> children;

   public:

    cg_scope *spawn();
    llvm::Value *find_binding(std::string &name);
    inline void set_binding(std::string name, llvm::Value *v) {
      m_bindings[name] = v;
    }

    inline void set_parent(cg_scope *s) { m_parent = s; }
  };

  class cg_options {};



  // check if two types are
  bool subtype(datatype *A, datatype *B);




  using RTDyldObjHandleT = llvm::orc::VModuleKey;

  // orc jit execution engine
  // see julia's src/jitlayer.h for reference.
  // (most of the JIT design is stolen from julia's :>)
  class ojit_ee {
   public:
    using CompilerResultT = std::unique_ptr<llvm::MemoryBuffer>;
    typedef llvm::orc::LegacyRTDyldObjectLinkingLayer ObjLayer;
    typedef llvm::orc::LegacyIRCompileLayer<ObjLayer, llvm::orc::SimpleCompiler>
        CompileLayer;
    typedef llvm::orc::VModuleKey ModuleHandle;
    typedef llvm::StringMap<void *> SymbolTableT;
    typedef llvm::object::OwningBinary<llvm::object::ObjectFile> OwningObj;




    ojit_ee(llvm::TargetMachine &TM);


    void add_global_mapping(llvm::StringRef, uint64_t);



    ModuleHandle add_module(std::unique_ptr<llvm::Module>);
    void remove_module(ModuleHandle);

    const llvm::DataLayout &getDataLayout() const;
    const llvm::Triple &getTargetTriple() const;

    inline llvm::JITSymbol find_symbol(const std::string name) {
      return find_mangled_symbol(mangle(name));
    }

    inline uint64_t get_type_size(llvm::Type *t) {
      return DL.getTypeAllocSize(t);
    }

    void *get_function_address(std::string);

    inline void add_dlhandle(void *h) { dlhandles.push_back(h); }

   private:
    inline std::string mangle(const std::string &Name) {
      std::string MangledName;
      {
        llvm::raw_string_ostream MangledNameStream(MangledName);
        llvm::Mangler::getNameWithPrefix(MangledNameStream, Name, DL);
      }
      return MangledName;
    }


    std::unique_ptr<llvm::Module> opt_module(std::unique_ptr<llvm::Module>);



    llvm::JITSymbol find_mangled_symbol(const std::string &Name,
                                        bool ExportedSymbolsOnly = false);
    llvm::TargetMachine &TM;
    const llvm::DataLayout DL;
    // Should be big enough that in the common case, The
    // object fits in its entirety

    llvm::orc::ExecutionSession exec_session;
    std::shared_ptr<llvm::orc::SymbolResolver> symbol_resolver;
    std::shared_ptr<llvm::RTDyldMemoryManager> mem_mgr;
    ObjLayer obj_layer;
    CompileLayer compile_layer;

    using OptimizeFunction = std::function<std::unique_ptr<llvm::Module>(
        std::unique_ptr<llvm::Module>)>;


    llvm::orc::LegacyIRTransformLayer<CompileLayer, OptimizeFunction> opt_layer;

    SymbolTableT GlobalSymbolTable;
    SymbolTableT LocalSymbolTable;
    std::vector<ModuleHandle> module_keys;

    std::vector<void *> dlhandles;
  };




  // execution engine is a nice global that exposes interfaces that
  // make sense for the use-cases of helion
  extern ojit_ee *execution_engine;


  std::string get_next_param_name(void);
  void register_param_name_as_used(std::string);


  /**
   * convert an ast module into an intermediate representation module
   */
  iir::module *compile_module(std::unique_ptr<ast::module> m);

  void init_types(void);
  void init_codegen(void);
  void init_iir(void);


  inline void init() {
    init_iir();
    init_types();
    init_codegen();
  }

};  // namespace helion

#endif
