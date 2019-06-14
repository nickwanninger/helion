// [License]
// MIT - See LICENSE.md file in the package.

#include <dlfcn.h>
#include <helion/ast.h>
#include <helion/core.h>
#include <helion/gc.h>
#include <iostream>
#include <unordered_map>


using namespace helion;



/*
llvm::Function *helion::compile_method(method_instance *mi, cg_scope *s,
                                       llvm::Module *m) {
  cg_ctx ctx(llvm_ctx);

  auto voidt = llvm::Type::getVoidTy(llvm_ctx);

  std::vector<llvm::Type *> arg_types;
  // for (auto t : mi->

  auto *type = llvm::FunctionType::get(voidt, arg_types, false);

  // create the function in the provided module
  ctx.func = llvm::Function::Create(type, llvm::Function::LinkageTypes::ExternalLinkage, mi->of->name, m);


  auto bb = llvm::BasicBlock::Create(llvm_ctx, "entry", ctx.func);
  ctx.builder.SetInsertPoint(bb);
  return ctx.func;
}

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
  auto leftv = left->codegen(ctx, sc);
  auto rightv = left->codegen(ctx, sc);
  return nullptr;
}

llvm::Value *ast::dot::codegen(cg_ctx &ctx, cg_scope *sc) {
  auto into = expr->codegen(ctx, sc);
  // TODO: implement dot
  return nullptr;
}


llvm::Value *ast::subscript::codegen(cg_ctx &ctx, cg_scope *sc) {
  auto into = expr->codegen(ctx, sc);
  // TODO: implement subscript
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
*/
