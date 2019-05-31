// [License]
// MIT - See LICENSE.md file in the package.

#define PULL_LLVM
#include <helion/core.h>


using namespace helion;

llvm::Type *helion::datatype_to_llvm(datatype *dt) {

  if (dt->struct_decl != nullptr) return dt->struct_decl;


  return nullptr;

}
