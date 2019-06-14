// [License]
// MIT - See LICENSE.md file in the package.


#include <helion/ast.h>
#include <helion/core.h>
#include <helion/gc.h>
#include <helion/iir.h>


using namespace helion;
using namespace helion::iir;


// some common types
type *int_type = nullptr;
type *float_type = nullptr;

void helion::init_iir(void) {
  int_type = gc::make_collected<named_type>("Int");
  float_type = gc::make_collected<named_type>("Int");
}



type &value::get_type(void) { return *ty; };

void value::set_type(type &t) { ty = &t; }



value *iir::new_int(size_t v) {
  auto *e = gc::make_collected<const_int>();
  e->set_type(*int_type);
  e->val = v;
  return e;
}



value *iir::new_float(double v) {
  auto *e = gc::make_collected<const_flt>();
  e->set_type(*float_type);
  e->val = v;
  return e;
}




instruction::instruction(block &_bb, inst_type t, type &dt, slice<value *> as)
    : itype(t), bb(_bb) {
  uid = bb.fn.next_uid();
  set_type(dt);
  args = as;
}

instruction::instruction(block &_bb, inst_type t, type &dt)
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

  if (!(itype == inst_type::ret || itype == inst_type::br ||
        itype == inst_type::jmp)) {
    s << "%";
    s << std::to_string(uid);
    s << " = ";
  }


  s << inst_type_to_str(itype);
  s << " ";
  for (int i = 0; i < args.size(); i++) {
    args[i]->print(s, true);
    if (i < args.size() - 1) s << ", ";
  }
}


func::func(module &m) : mod(m) {}

block *func::new_block(void) {
  auto b = gc::make_collected<block>(*this);
  return b;
}

void func::add_block(block *b) {
  b->id = blocks.size();
  blocks.push_back(b);
}

void func::print(std::ostream &s, bool just_name, int depth) {
  if (just_name) {
    s << "DOTHIS";
    return;
  }

  std::string indent = "";
  for (int i = 0; i < depth; i++) indent += " ";

  s << indent;

  if (intrinsic) {
    s << "intrinsic ";
  } else {
    s << "func ";
  }
  s << name << " ";

  if (blocks.size() == 0) {
    s << "decl ";
  }

  s << get_type().str();
  for (int i = 0; i < blocks.size(); i++) {
    s << "\n";
    s << indent;
    s << "  ";
    blocks[i]->print(s, false, depth + 1);
  }
  s << "\n" << indent << "end";
}


void block::print(std::ostream &s, bool just_name, int depth) {
  if (just_name) {
    s << "&";
    s << std::to_string(id);
    return;
  }

  std::string indent = "";
  for (int i = 0; i < depth; i++) indent += " ";

  s << indent;
  s << "&";
  s << std::to_string(id);
  s << ":";
  for (int i = 0; i < insts.size(); i++) {
    s << "\n";
    s << indent;
    s << "  ";
    insts[i]->print(s, false, depth + 1);
    if (i < insts.size() - 1) {
      // s << "\n";
    }
  }
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
    handle(br);
    handle(jmp);
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




/*
 * module constructor
 */
iir::module::module(std::string name) : name(name) {}

func *iir::module::create_func(std::shared_ptr<ast::func> node) {
  func *fc = gc::make_collected<func>(*this);
  fc->node = node;
  fc->name = node->name;
  fc->intrinsic = false;
  fc->set_type(convert_type(node->proto->type));
  return fc;
}

func *iir::module::create_intrinsic(std::string name,
                                    std::shared_ptr<ast::type_node> tn) {
  func *fc = gc::make_collected<func>(*this);
  fc->node = nullptr;
  fc->name = name;
  fc->intrinsic = true;
  fc->set_type(convert_type(tn));
  bind(name, fc);
  return fc;
}