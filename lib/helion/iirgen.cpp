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


iir::value *ast::binary_op::to_iir(iir::builder &b, iir::scope *sc) {
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
  return nullptr;
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
  return nullptr;
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

  auto *glob = b.create_global(v->get_type());
  glob->set_name(name);
  sc->bind(name, v);
  b.create_store(glob, v);
  return v;
}


iir::value *ast::var::to_iir(iir::builder &b, iir::scope *sc) {
  std::string name = str();
  iir::value *v = sc->find_binding(name);
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
  fn->set_type(iir::convert_type(this->proto->type));
  fn->add_block(bb);
  b2.set_target(bb);

  for (auto &arg: proto->args) {
    std::string name = arg->name;
    auto &ty = iir::convert_type(arg->type);
    auto pop = b2.create_poparg(ty);
    pop->set_name(name);
    ns->bind(name, pop);
  }

  for (auto &e : stmts) {
    e->to_iir(b2, ns);
  }
  return fn;
}


iir::value *ast::def::to_iir(iir::builder &b, iir::scope *sc) {
  return nullptr;
}

iir::value *ast::if_node::to_iir(iir::builder &b, iir::scope *sc) {
  iir::block *final_join = b.new_block();

  for (auto cond : conds) {
    iir::block *body = b.new_block();
    iir::block *join = nullptr;


    if (cond.cond != nullptr) {
      auto cond_val = cond.cond->to_iir(b, sc);
      join = b.new_block();
      b.create_branch(cond_val, body, join);
    } else {
      b.create_jmp(body);
    }



    b.insert_block(body);
    b.set_target(body);

    auto ns = sc->spawn();
    for (auto &s : cond.body) {
      s->to_iir(b, ns);
    }


    if (join != nullptr) {
      b.insert_block(join);
      b.create_jmp(join);
      b.set_target(join);
    } else {
      b.create_jmp(final_join);
      b.set_target(final_join);
    }
  }
  b.insert_block(final_join);

  return nullptr;
}


iir::value *ast::typedef_node::to_iir(iir::builder &b, iir::scope *sc) {
  return nullptr;
}

iir::value *ast::typeassert::to_iir(iir::builder &b, iir::scope *sc) {
  return nullptr;
}
