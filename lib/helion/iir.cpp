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
  float_type = gc::make_collected<named_type>("Float");
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



std::atomic<int> next_inst_uid = 0;

instruction::instruction(block &_bb, inst_type t, type &dt, slice<value *> as)
    : itype(t), bb(_bb) {
  uid = next_inst_uid++;
  set_type(dt);
  args = as;
}


instruction::instruction(block &_bb, inst_type t, type &dt)
    : itype(t), bb(_bb) {
  uid = next_inst_uid++;
  set_type(dt);
}


void instruction::print(std::ostream &s, bool just_name, int depth) {
  if (just_name) {
    if (name != "") {
      s << "%" << name;
    } else {
      s << "%" << std::to_string(uid);
    }
    return;
  }

  std::string indent = "";
  for (int i = 0; i < depth; i++) indent += " ";


  if (!(itype == inst_type::ret || itype == inst_type::br ||
        itype == inst_type::jmp)) {
    print(s, true);

    s << ": " << get_type().str() << " = ";
  }

  s << inst_type_to_str(itype) << " ";
  for (int i = 0; i < args.size(); i++) {
    args[i]->print(s, true, depth + 3);
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
  std::string indent = "";
  for (int i = 0; i < depth; i++) indent += " ";

  if (intrinsic) {
    s << "intrinsic ";
  } else {
    s << "func ";
  }
  s << name << "";

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
  // s << "\n" << indent << "end";
}


void block::print(std::ostream &s, bool just_name, int depth) {
  if (just_name) {
    if (name != "") {
      s << "&";
      s << name;
    } else {
      s << "&bb";
    }
    s << "." << std::to_string(id);
    return;
  }

  std::string indent = "";
  for (int i = 0; i < depth; i++) indent += " ";

  print(s, true);
  s << ":";
  for (int i = 0; i < insts.size(); i++) {
    s << "\n";
    s << indent << indent;
    s << "  ";
    insts[i]->print(s, false, depth + 1);
  }

  if (terminated()) {
    s << "\n";
    s << indent << indent;
    s << "  ";
    terminator->print(s, false, depth + 1);
  }
  s << "\n";
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
    handle(global);
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
    handle(poparg);
  };

#undef handle

  return "unknown";
}




/*
 * module constructor
 */
iir::module::module(std::string name) : scope(), name(name) {}

func *iir::module::create_func(std::shared_ptr<ast::func> node) {
  func *fc = gc::make_collected<func>(*this);
  fc->node = node;
  fc->name = node->name;
  fc->intrinsic = false;
  fc->set_type(*convert_type(node->proto->type));
  return fc;
}

func *iir::module::create_intrinsic(std::string name,
                                    std::shared_ptr<ast::type_node> tn) {
  func *fc = gc::make_collected<func>(*this);
  fc->node = nullptr;
  fc->name = name;
  fc->intrinsic = true;
  fc->set_type(*convert_type(tn));
  bind(name, fc);
  return fc;
}
