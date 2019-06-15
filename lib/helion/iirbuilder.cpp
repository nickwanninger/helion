// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/core.h>
#include <helion/gc.h>
#include <helion/iir.h>


using namespace helion;
using namespace helion::iir;


iir::builder::builder(func &f) : current_func(f) {}

void iir::builder::set_target(block *b) { target = b; }


value *iir::builder::create_alloc(type &d) {
  return create_inst(inst_type::alloc, d);
}

void iir::builder::create_ret(value *v) {
  // and create the inst
  create_inst(inst_type::ret, v->get_type(), {v});
}


instruction *builder::add_inst(instruction *i) {
  target->add_inst(i);
  return i;
}


instruction *builder::create_inst(inst_type it, type &dt) {
  assert(target != nullptr);
  auto i = gc::make_collected<instruction>(*target, it, dt);
  return add_inst(i);
}

instruction *builder::create_inst(inst_type it, type &dt, slice<value *> as) {
  assert(target != nullptr);
  auto i = gc::make_collected<instruction>(*target, it, dt, as);
  return add_inst(i);
}



value *builder::create_binary(inst_type t, value *l, value *r) {
  return create_inst(t, new_variable_type(), {l, r});
}


void builder::create_branch(value *cond, block *if_true, block *if_false) {
  create_inst(inst_type::br, new_variable_type(), {cond, if_true, if_false});
}

void builder::create_jmp(block *b) {
  create_inst(inst_type::jmp, new_variable_type(), {b});
}


value *builder::create_global(type &t) {
  return create_inst(inst_type::global, t);
}



void builder::create_store(value *dst, value *val) {
  create_inst(inst_type::store, dst->get_type(), {dst, val});
}

value *builder::create_load(value *from) {
  return create_inst(inst_type::load, from->get_type());
}
