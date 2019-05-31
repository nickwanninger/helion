// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __HELION_CORE_JIT__
#define __HELION_CORE_JIT__

#include <helion/text.h>
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


  // a value is an opaque pointer to something garbage collected in the helion
  // jit runtime. It has no real meaning except as a better, typed replacement
  // for voidptr
  struct value;



  struct datatype_name {};

  struct datatype {
    std::shared_ptr<datatype_name> name;
    std::shared_ptr<datatype> super = nullptr;
    // a list of type parameters. ie: Vector<Int>
    std::vector<std::shared_ptr<datatype>> parameters;

    std::vector<text> names;
    llvm::Type *struct_decl = nullptr;  // declaration of the struct in LLVM
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
    void addGlobalMapping(StringRef Name, uint64_t Addr);
    void addGlobalMapping(const GlobalValue *GV, void *Addr);
    void *getPointerToGlobalIfAvailable(StringRef S);
    void *getPointerToGlobalIfAvailable(const GlobalValue *GV);
    void addModule(std::unique_ptr<Module> M);
    void removeModule(ModuleHandleT H);
    JL_JITSymbol findSymbol(const std::string &Name, bool ExportedSymbolsOnly);
    JL_JITSymbol findUnmangledSymbol(const std::string Name);
    JL_JITSymbol resolveSymbol(const std::string &Name);
    uint64_t getGlobalValueAddress(const std::string &Name);
    uint64_t getFunctionAddress(const std::string &Name);
    Function *FindFunctionNamed(const std::string &Name);
    */


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
    std::map<std::string, binding *> bindings;
  };



  class codectx {
   public:
    llvm::IRBuilder<> builder;
    llvm::Function *func = nullptr;
  };


  llvm::Type *datatype_to_llvm(datatype *);

  void init_codegen(void);

};  // namespace helion

#endif
