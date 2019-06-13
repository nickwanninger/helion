// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/core.h>
#include <helion/gc.h>
#include <helion/iir.h>


using namespace helion;
using namespace helion::iir;


iir::builder::builder(func &f) : current_func(f) {}

void iir::builder::set_target(block *b) { target = b; }


value *iir::builder::create_alloc(datatype *d) {
  return create_inst(inst_type::alloc, d);
}

void iir::builder::create_ret(value *v) {
  // add the type of this value to the function's return types
  current_func.return_types.insert(v->get_type());
  // and create the inst
  create_inst(inst_type::ret, v->get_type(), {v});
}


instruction *builder::add_inst(instruction *i) {
  target->add_inst(i);
  return i;
}


instruction *builder::create_inst(inst_type it, datatype *dt) {
  assert(target != nullptr);
  auto i = gc::make_collected<instruction>(*target, it, dt);
  return add_inst(i);
}

instruction *builder::create_inst(inst_type it, datatype *dt,
                                  slice<value *> as) {
  assert(target != nullptr);
  auto i = gc::make_collected<instruction>(*target, it, dt, as);
  return add_inst(i);
}
