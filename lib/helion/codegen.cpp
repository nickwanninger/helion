// [License]
// MIT - See LICENSE.md file in the package.

#include <dlfcn.h>
#include <helion/ast.h>
#include <helion/core.h>
#include <helion/gc.h>
#include <iostream>
#include <unordered_map>


using namespace helion;

llvm::Value *ast::number::codegen(cg_ctx &ctx, cg_scope *sc) {
  llvm::Value *val = nullptr;
  if (type == num_type::integer) {
    val = llvm::ConstantInt::get(int32_type->to_llvm(), as.integer);
    sc->set_val_type(val, int32_type);
  } else if (type == num_type::floating) {
    val = llvm::ConstantInt::get(float32_type->to_llvm(), as.floating);
    sc->set_val_type(val, float32_type);
  }
  if (val == nullptr) {
    die("unknown number type in codegen");
  }
  return val;
}


llvm::Value *ast::binary_op::codegen(cg_ctx &ctx, cg_scope *sc) {
  return nullptr;
}

llvm::Value *ast::dot::codegen(cg_ctx &ctx, cg_scope *sc) { return nullptr; }


llvm::Value *ast::subscript::codegen(cg_ctx &ctx, cg_scope *sc) {
  return nullptr;
}

llvm::Value *ast::call::codegen(cg_ctx &ctx, cg_scope *sc) { return nullptr; }



llvm::Value *ast::tuple::codegen(cg_ctx &ctx, cg_scope *sc) { return nullptr; }


llvm::Value *ast::string::codegen(cg_ctx &ctx, cg_scope *sc) { return nullptr; }

llvm::Value *ast::keyword::codegen(cg_ctx &ctx, cg_scope *sc) {
  return nullptr;
}

llvm::Value *ast::nil::codegen(cg_ctx &ctx, cg_scope *sc) { return nullptr; }

llvm::Value *ast::do_block::codegen(cg_ctx &ctx, cg_scope *sc) {
  return nullptr;
}

llvm::Value *ast::return_node::codegen(cg_ctx &ctx, cg_scope *sc) {
  // codegen the return value
  auto rv = val->codegen(ctx, sc);
  auto ret = ctx.builder.CreateRet(rv);
  return ret;
}


llvm::Value *ast::type_node::codegen(cg_ctx &ctx, cg_scope *sc) {
  return nullptr;
}

llvm::Value *ast::var_decl::codegen(cg_ctx &ctx, cg_scope *sc) {
  return nullptr;
}


llvm::Value *ast::var::codegen(cg_ctx &ctx, cg_scope *sc) { return nullptr; }

llvm::Value *ast::prototype::codegen(cg_ctx &ctx, cg_scope *sc) {
  return nullptr;
}


llvm::Value *ast::func::codegen(cg_ctx &ctx, cg_scope *sc) { return nullptr; }


llvm::Value *ast::def::codegen(cg_ctx &ctx, cg_scope *sc) { return nullptr; }

llvm::Value *ast::if_node::codegen(cg_ctx &ctx, cg_scope *sc) {
  return nullptr;
}


llvm::Value *ast::typedef_node::codegen(cg_ctx &ctx, cg_scope *sc) {
  return nullptr;
}

llvm::Value *ast::typeassert::codegen(cg_ctx &ctx, cg_scope *sc) {
  return nullptr;
}

