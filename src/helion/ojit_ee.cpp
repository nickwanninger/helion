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
