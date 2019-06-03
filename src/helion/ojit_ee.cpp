// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/core.h>
#include <iostream>

using namespace helion;


ojit_ee::ojit_ee(llvm::TargetMachine& TM)
    : TM(TM),
      DL(TM.createDataLayout()),
      ES(),
      ObjectLayer(ES,
                  [this](RTDyldObjHandleT) {
                    ObjLayerT::Resources result;
                    result.MemMgr = mem_mgr;
                    result.Resolver = symbol_resolver;
                    return result;
                  }),
      CompileLayer(ObjectLayer, CompilerT(this)) {}



const llvm::DataLayout& ojit_ee::getDataLayout() const { return DL; }

const llvm::Triple& ojit_ee::getTargetTriple() const {
  return TM.getTargetTriple();
}


void ojit_ee::add_global_mapping(llvm::StringRef name, uint64_t addr) {
  bool successful =
      GlobalSymbolTable.insert(std::make_pair(name, (void*)addr)).second;
  (void)successful;
  assert(successful);
}

llvm::JITSymbol ojit_ee::find_symbol(const std::string& name, bool exp_only) {
  void* addr = nullptr;
  if (exp_only) {
    /*
      // Step 1: Check against list of known external globals
      addr = getPointerToGlobalIfAvailable(name);
      */
  }
  // Step 2: Search all previously emitted symbols
  if (addr == nullptr) addr = LocalSymbolTable[name];
  return llvm::JITSymbol((uintptr_t)addr, llvm::JITSymbolFlags::Exported);
}
