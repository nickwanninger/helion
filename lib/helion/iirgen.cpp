// [License]
// MIT - See LICENSE.md file in the package.

#include <dlfcn.h>
#include <helion/ast.h>
#include <helion/core.h>
#include <helion/gc.h>
#include <helion/iir.h>
#include <iostream>
#include <unordered_map>


using namespace helion;



iir::value *ast::number::to_iir(iir::builder &b, iir::scope *sc) {
  if (type == num_type::integer) {
    return iir::new_int(as.integer);
  } else if (type == num_type::floating) {
    return iir::new_float(as.floating);
  }
  return nullptr;
}



static iir::value *compile_assign(iir::builder &b, iir::scope *sc,
                                  std::shared_ptr<ast::node> to_n,
                                  std::shared_ptr<ast::node> val_n) {
  using namespace iir;

  auto val = val_n->to_iir(b, sc);


  value *dst = nullptr;


  if (auto var = to_n->as<ast::var *>()) {
    std::string name = var->str();
    dst = sc->find_binding(name);
    if (dst == nullptr) throw std::logic_error("unable to find var in assignment");

    b.create_store(dst, val);
    return val;
  }



  throw std::logic_error("failed to create assignment, invalid lhs");

}




iir::value *ast::binary_op::to_iir(iir::builder &b, iir::scope *sc) {
  // assignment is a special case of the binary op, as normally destinations
  // return the value of a variable, instead of a reference to its storage
  // location
  if (this->op == "=") return compile_assign(b, sc, left, right);

  auto lhs = left->to_iir(b, sc);
  auto rhs = right->to_iir(b, sc);




  if (this->op == "+") return b.create_binary(iir::inst_type::add, rhs, lhs);
  if (this->op == "-") return b.create_binary(iir::inst_type::sub, rhs, lhs);
  if (this->op == "*") return b.create_binary(iir::inst_type::mul, rhs, lhs);
  if (this->op == "/") return b.create_binary(iir::inst_type::div, rhs, lhs);
  return nullptr;
}

iir::value *ast::dot::to_iir(iir::builder &b, iir::scope *sc) {
  // TODO: implement dot
  return nullptr;
}


iir::value *ast::subscript::to_iir(iir::builder &b, iir::scope *sc) {
  // TODO: implement subscript
  return nullptr;
}

iir::value *ast::call::to_iir(iir::builder &b, iir::scope *sc) {
  auto callee = func->to_iir(b, sc);

  std::vector<iir::value *> args;
  for (auto &arg : this->args) {
    args.push_back(arg->to_iir(b, sc));
  }

  return b.create_call(callee, args);
}

iir::value *ast::tuple::to_iir(iir::builder &b, iir::scope *sc) {
  return nullptr;
}


iir::value *ast::string::to_iir(iir::builder &b, iir::scope *sc) {
  return nullptr;
}

iir::value *ast::keyword::to_iir(iir::builder &b, iir::scope *sc) {
  return nullptr;
}

iir::value *ast::nil::to_iir(iir::builder &b, iir::scope *sc) {
  return nullptr;
}

iir::value *ast::do_block::to_iir(iir::builder &b, iir::scope *sc) {
  auto ns = sc->spawn();

  iir::value *last_expr = nullptr;
  for (auto &s : exprs) {
    last_expr = s->to_iir(b, ns);
  }
  return last_expr;
}

iir::value *ast::return_node::to_iir(iir::builder &b, iir::scope *sc) {
  // to_iir the return value
  auto rv = val->to_iir(b, sc);
  b.create_ret(rv);
  return rv;
}


iir::value *ast::type_node::to_iir(iir::builder &b, iir::scope *sc) {
  return nullptr;
}

iir::value *ast::var_decl::to_iir(iir::builder &b, iir::scope *sc) {
  auto *v = value->to_iir(b, sc);

  iir::value *dst;

  // if we are in the global scope, make a global
  if (sc->mod == sc) {
    dst = b.create_global(v->get_type());
  } else {
    dst = b.create_alloc(v->get_type());
  }
  dst->set_name(name);
  sc->bind(name, dst);
  b.create_store(dst, v);
  return v;
}


iir::value *ast::var::to_iir(iir::builder &b, iir::scope *sc) {
  std::string name = str();
  iir::value *v = sc->find_binding(name);
  if (v == nullptr) {
    puts(name);
    throw std::logic_error("variable not found");
  }
  return b.create_load(v);
}

iir::value *ast::prototype::to_iir(iir::builder &b, iir::scope *sc) {
  return nullptr;
}


iir::value *ast::func::to_iir(iir::builder &b, iir::scope *sc) {
  auto *fn = gc::make_collected<iir::func>(*sc->mod);

  iir::builder b2(*fn);
  auto ns = sc->spawn();

  auto bb = fn->new_block();
  fn->set_type(*iir::convert_type(this->proto->type));
  fn->add_block(bb);
  b2.set_target(bb);

  for (auto &arg : proto->args) {
    std::string name = arg->name;
    auto ty = iir::convert_type(arg->type);
    auto pop = b2.create_poparg(*ty);
    pop->set_name(name);
    ns->bind(name, pop);
  }

  stmt->to_iir(b2, ns);
  return fn;
}


iir::value *ast::def::to_iir(iir::builder &b, iir::scope *sc) {
  return nullptr;
}

iir::value *ast::if_node::to_iir(iir::builder &b, iir::scope *sc) {
  puts(str());
  auto cond_val = cond->to_iir(b, sc);


  if (false_expr == nullptr) {
    auto if_true = b.new_block("if_true");
    auto join = b.new_block("if_join");

    b.create_branch(cond_val, if_true, join);

    b.set_target(if_true);
    b.insert_block(if_true);

    true_expr->to_iir(b, sc);

    b.create_jmp(join);
    b.insert_block(join);
    b.set_target(join);
  } else {
    auto if_true = b.new_block("if_true");
    auto if_false = b.new_block("if_false");
    auto if_join = b.new_block("if_join");

    // in entry, create branch
    b.create_branch(cond_val, if_true, if_false);

    // switch to if_true branch and codegen it w/ a jmp to join
    b.insert_block(if_true);
    b.set_target(if_true);
    true_expr->to_iir(b, sc);
    b.create_jmp(if_join);

    // do the same for if_false
    b.insert_block(if_false);
    b.set_target(if_false);
    false_expr->to_iir(b, sc);
    b.create_jmp(if_join);

    // and set the target to the join
    b.insert_block(if_join);
    b.set_target(if_join);

  }


  return nullptr;
}


iir::value *ast::typedef_node::to_iir(iir::builder &b, iir::scope *sc) {
  return nullptr;
}

iir::value *ast::typeassert::to_iir(iir::builder &b, iir::scope *sc) {
  return nullptr;
}
