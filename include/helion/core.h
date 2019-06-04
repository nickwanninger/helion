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

#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/LambdaResolver.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"



#include <helion/ast.h>
#include <helion/text.h>

/*
 * this header file defines the core classes and data types used throughout the
 * helion compiler and jit runtime. For example, the module type, the basic type
 * classes, etc...
 */

namespace helion {

  extern llvm::LLVMContext llvm_ctx;

  struct binding;
  struct datatype_name;
  struct datatype;


  // a linkage to the 'Any' type
  extern datatype *any_type;
  // a linkage to the basic int type
  extern datatype *int32_type;
  // a linkage to the basic float type
  extern datatype *float32_type;


  // a value is an opaque pointer to something garbage collected in the helion
  // jit runtime. It has no real meaning except as a better, typed replacement
  // for voidptr
  struct value;

  struct datatype_name {};



  struct typeinfo {
    // a type can have multiple 'styles'. For example, Int32 has the style of
    // INTEGER, and thne has a size of 32. This is helpful when lowering to
    // llvm::Type
    enum class type_style : unsigned char {
      OBJECT,    // normal reference type
      INTEGER,   // is an n bit integer
      FLOATING,  // is an n bit floating point number
      UNION,     // is a union of the parameters
      TUPLE,
      METHOD,  // first parameter is
    };
    struct field {
      datatype *type;
      std::string name;
    };

    std::shared_ptr<ast::type_node> node;

    type_style style = type_style::OBJECT;
    std::string name;

    // how many bits this type is in memory (for primitive types)
    int bits;

    // a type is specialized iff it's parameters are filled in correctly
    bool specialized = false;

    // the super type of this type. Defaults to any_type
    datatype *super;

    std::vector<std::string> param_names;
    std::vector<field> fields;

    std::vector<std::unique_ptr<datatype>> specializations;
  };

  struct datatype {
    std::shared_ptr<typeinfo> ti;

    bool specialized = true;
    // a list of type parameters. ie: Vector<Int>
    std::vector<datatype *> param_types;
    // declaration of the type in LLVM as an llvm::Type
    llvm::Type *type_decl = nullptr;

    static datatype &create(std::string, datatype & = *any_type,
                            std::vector<std::string> = {});
    static inline datatype &create(std::string n, std::vector<std::string> p) {
      return datatype::create(n, *any_type, p);
    };

    static datatype &create_integer(std::string, int);
    static datatype &create_float(std::string, int);

    void add_field(std::string, datatype *);
    datatype *specialize(std::vector<datatype *>);

    llvm::Type *to_llvm(void);

    text str(void);

   private:
    inline datatype(std::string name, datatype &s) {
      ti = std::make_shared<typeinfo>();
      ti->name = name;
      ti->super = &s;
    }

    inline datatype(datatype &other) {
      ti = other.ti;
      specialized = other.specialized;
    }
  };




  // check if two types are
  bool subtype(datatype *A, datatype *B);

  struct method_sig {
    datatype *return_type;
  };

  class method_instance;

  class method {
   public:
    std::string name;
    std::string file;
    std::shared_ptr<ast::node> src;
    // table of all method_instance specializations that we've compiled
    std::vector<method_instance *> specializations;
  };

  class method_instance {
   public:
    // what method is this an instance of?
    method *of;
  };

  // This type represents an executable operation
  struct code_instance {
    // the defining method instance
    method_instance *def;
    llvm::Function *llvm_func;

    datatype *return_type;
    std::vector<datatype *> arg_types;
  };




  using RTDyldObjHandleT = llvm::orc::VModuleKey;

  // orc jit execution engine
  // see julia's src/jitlayer.h for reference.
  // (most of the JIT design is stolen from julia's :>)
  class ojit_ee {
   public:
    using CompilerResultT = std::unique_ptr<llvm::MemoryBuffer>;
    struct CompilerT {
      CompilerT(ojit_ee *pjit) : jit(*pjit) {}
      CompilerResultT operator()(llvm::Module &M);

     private:
      ojit_ee &jit;
    };

    typedef llvm::orc::LegacyRTDyldObjectLinkingLayer ObjLayerT;
    typedef llvm::orc::LegacyIRCompileLayer<ObjLayerT, CompilerT> CompileLayerT;
    typedef llvm::orc::VModuleKey ModuleHandleT;
    typedef llvm::StringMap<void *> SymbolTableT;
    typedef llvm::object::OwningBinary<llvm::object::ObjectFile> OwningObj;

    ojit_ee(llvm::TargetMachine &TM);

    /*
    void *getPointerToGlobalIfAvailable(StringRef S);
    void *getPointerToGlobalIfAvailable(const GlobalValue *GV);
    void addModule(std::unique_ptr<Module> M);
    void removeModule(ModuleHandleT H);
    JL_JITSymbol findUnmangledSymbol(const std::string Name);
    JL_JITSymbol resolveSymbol(const std::string &Name);
    uint64_t getGlobalValueAddress(const std::string &Name);
    uint64_t getFunctionAddress(const std::string &Name);
    Function *FindFunctionNamed(const std::string &Name);
    */

    void add_global_mapping(llvm::StringRef, uint64_t);


    llvm::JITSymbol find_symbol(const std::string &Name,
                                bool ExportedSymbolsOnly = false);


    void add_module(std::unique_ptr<llvm::Module> M);
    void remove_module(ModuleHandleT H);

    const llvm::DataLayout &getDataLayout() const;
    const llvm::Triple &getTargetTriple() const;

   private:
    /*
    std::string getMangledName(const std::string &Name);
    std::string getMangledName(const GlobalValue *GV);
    */

    llvm::TargetMachine &TM;
    const llvm::DataLayout DL;
    // Should be big enough that in the common case, The
    // object fits in its entirety

    llvm::orc::ExecutionSession ES;
    std::shared_ptr<llvm::orc::SymbolResolver> symbol_resolver;
    std::shared_ptr<llvm::RTDyldMemoryManager> mem_mgr;
    ObjLayerT ObjectLayer;
    CompileLayerT CompileLayer;

    SymbolTableT GlobalSymbolTable;
    SymbolTableT LocalSymbolTable;
  };

  extern ojit_ee *execution_engine;


  // a binding is what helion global variables are stored in
  struct binding {
    datatype *type;
    text name;
    value *data;
  };

  class module {
   public:
    module *parent = nullptr;
    text name;
    std::map<std::string, datatype *> types;
    std::map<std::string, binding *> bindings;
  };


  llvm::Type *datatype_to_llvm(datatype *);



  void init_types(void);
  void init_codegen(void);


  inline void init() {
    init_types();
    init_codegen();
  }

};  // namespace helion

#endif
