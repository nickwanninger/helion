// [License]
// MIT - See LICENSE.md file in the package.


#include <helion/core.h>
#include <helion/gc.h>
#include <helion/iir.h>


using namespace helion;
using namespace helion::iir;



datatype *value::get_type(void) { return ty; };
void value::set_type(datatype *t) { ty = t; }



value *iir::new_int(size_t v) {
  auto *e = gc::make_collected<const_int>();
  e->set_type(integer_type);
  e->val = v;
  return e;
}



value *iir::new_float(double v) {
  auto *e = gc::make_collected<const_flt>();
  e->set_type(float64_type);
  e->val = v;
  return e;
}




instruction::instruction(block &_bb, inst_type t, datatype *dt,
                         slice<value *> as)
    : itype(t), bb(_bb) {
  uid = bb.fn.next_uid();
  set_type(dt);
  args = as;
}

instruction::instruction(block &_bb, inst_type t, datatype *dt)
    : itype(t), bb(_bb) {
  uid = bb.fn.next_uid();
  set_type(dt);
}


void instruction::print(std::ostream &s, bool just_name, int depth) {
  if (just_name) {
    s << "%";
    s << std::to_string(uid);
    return;
  }

  std::string indent = "";
  for (int i = 0; i < depth; i++) indent += " ";

  s << indent;
  s << "(";
  s << inst_type_to_str(itype);

  if (!(itype == inst_type::ret || itype == inst_type::branch)) {
    s << " %";
    s << std::to_string(uid);
  }
  s << " ";
  s << get_type()->str();
  for (int i = 0; i < args.size(); i++) {
    s << " ";
    args[i]->print(s, true);
  }
  s << ")";
}


func::func(module &m) : mod(m) {}

block *func::new_block(void) {
  auto b = gc::make_collected<block>(*this);
  blocks.push_back(b);
  return b;
}


void func::print(std::ostream &s, bool just_name, int depth) {
  if (just_name) {
    s << "DOTHIS";
    return;
  }

  std::string indent = "";
  for (int i = 0; i < depth; i++) indent += " ";

  s << indent;
  s << "(func ";
  s << "\n";
  for (int i = 0; i < blocks.size(); i++) {
    s << indent;
    s << "  ";
    blocks[i]->print(s, false, depth + 1);
    if (i < blocks.size() - 1) {
      s << "\n";
    }
  }
  s << ")";
}


void block::print(std::ostream &s, bool just_name, int depth) {
  std::string indent = "";
  for (int i = 0; i < depth; i++) indent += " ";

  s << indent;
  s << "(&";
  s << std::to_string(id);
  s << "\n";
  for (int i = 0; i < insts.size(); i++) {
    s << indent;
    s << "  ";
    insts[i]->print(s, false, depth + 1);
    if (i < insts.size() - 1) {
      s << "\n";
    }
  }
  s << ")";
}


int func::next_uid(void) { return uid++; }

block::block(func &_fn) : fn(_fn) {}

void block::add_inst(instruction *i) { insts.push_back(i); }

/**
 * massive ugly function to convert an enum name to a string
 */
const char *iir::inst_type_to_str(inst_type t) {
#define handle(name)    \
  case inst_type::name: \
    return #name;
  switch (t) {
    handle(unknown);
    handle(ret);
    handle(branch);
    handle(call);
    handle(add);
    handle(sub);
    handle(mul);
    handle(div);
    handle(invert);
    handle(dot);
    handle(alloc);
    handle(load);
    handle(store);
    handle(cast);
  };

#undef handle

  return "unknown";
}
