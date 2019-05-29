// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __HELION_JIT__
#define __HELION_JIT__


#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
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
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/IPO.h"
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

#include <vector>  // for std::vector

#include <helion/types.h>

namespace helion {

  using namespace llvm;
  using namespace llvm::orc;

  namespace jit {



    /*
     * the helion::jit::isolate class acts as an application specific wrapper
     * around LLVM's internal ideas and whatnot. It allows adding, removing,
     * and accessing modules. Looking up symbols, and adding symbols
     */
    class isolate {
     protected:
      using ObjLayer = LegacyRTDyldObjectLinkingLayer;
      using CompileLayer = LegacyIRCompileLayer<ObjLayer, SimpleCompiler>;

      ExecutionSession session;
      std::shared_ptr<SymbolResolver> symbol_resolver;
      std::unique_ptr<TargetMachine> target_machine;
      DataLayout data_layout;
      ObjLayer obj_layer;
      CompileLayer compile_layer;
      std::vector<VModuleKey> module_keys;

     public:
      isolate()
          : symbol_resolver(createLegacyLookupResolver(
                session,
                [this](const std::string &Name) {
                  return obj_layer.findSymbol(Name, true);
                },
                [](Error Err) {
                  cantFail(std::move(Err), "lookupFlags failed");
                })),
            target_machine(EngineBuilder().selectTarget()),
            data_layout(target_machine->createDataLayout()),
            obj_layer(session,
                      [this](VModuleKey) {
                        return ObjLayer::Resources{
                            std::make_shared<SectionMemoryManager>(),
                            symbol_resolver};
                      }),
            compile_layer(obj_layer, SimpleCompiler(*target_machine)) {
        llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
      }

      inline TargetMachine &get_target_machine() { return *target_machine; }

      inline VModuleKey add_module(std::unique_ptr<Module> m) {
        auto K = session.allocateVModule();
        cantFail(compile_layer.addModule(K, std::move(m)));
        module_keys.push_back(K);
        return K;
      }


      void remove_module(VModuleKey key) {
        module_keys.erase(find(module_keys, key));
        cantFail(compile_layer.removeModule(key));
      }

      JITSymbol find_symbol(const std::string Name) {
        return findMangledSymbol(mangle(Name));
      }

      // mangle a string
      std::string mangle(const std::string &name) {
        std::string mangled_name;
        auto mangled_name_stream = raw_string_ostream(mangled_name);
        Mangler::getNameWithPrefix(mangled_name_stream, name, data_layout);
        return mangled_name;
      }


     private:




      JITSymbol findMangledSymbol(const std::string &Name) {
        const bool ExportedSymbolsOnly = true;

        // Search modules in reverse order: from last added to first added.
        // This is the opposite of the usual search order for dlsym, but makes
        // more sense in a REPL where we want to bind to the newest available
        // definition.
        for (auto H : make_range(module_keys.rbegin(), module_keys.rend()))
          if (auto Sym =
                  compile_layer.findSymbolIn(H, Name, ExportedSymbolsOnly))
            return Sym;

        // If we can't find the symbol in the JIT, try looking in the host
        // process.
        if (auto SymAddr = RTDyldMemoryManager::getSymbolAddressInProcess(Name))
          return JITSymbol(SymAddr, JITSymbolFlags::Exported);

#ifdef _WIN32
        // For Windows retry without "_" at beginning, as RTDyldMemoryManager
        // uses GetProcAddress and standard libraries like msvcrt.dll use names
        // with and without "_" (for example "_itoa" but "sin").
        if (Name.length() > 2 && Name[0] == '_')
          if (auto SymAddr = RTDyldMemoryManager::getSymbolAddressInProcess(
                  Name.substr(1)))
            return JITSymbol(SymAddr, JITSymbolFlags::Exported);
#endif

        return nullptr;
      }
    };



    /**
     * an enviroment is where higher level value concepts are stored, like
     * function->method mappings, modules, file caches, etc.
     */
    class enviroment : public isolate {};



  }  // namespace jit

}  // namespace helion

#endif
