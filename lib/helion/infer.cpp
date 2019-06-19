// [License]
// MIT - See LICENSE.md file in the package.


#include <helion/iir.h>
#include <helion/infer.h>
#include <stdexcept>

using namespace helion;




static bool has_var_type_definition(iir::type *t) {
  if (t->is_var()) {
    if (t->as_var()->points_to == t) return true;
  }

  if (t->is_named()) {
    auto n = t->as_named();
    for (auto &p : n->params) {
      if (has_var_type_definition(p)) return true;
    }
  }
  return false;
}


// find the real type for some type. Basically expands the type
// variable into its real named variable
iir::type *infer::find(iir::type *t) {
  if (t->is_var()) {
    auto tv = t->as_var();
    if (tv->points_to != tv) {
      // tv->points_to = find(tv->points_to);
      return find(tv->points_to);
    }
  }
  return t;
}


static bool is_arrow(iir::type *t) {
  if (!t->is_named()) return false;
  auto n = t->as_named();
  return n->name == "->";
}


static bool occurs(iir::var_type *ta, iir::type *tb) {
  auto t = infer::find(tb);
  if (t->is_var()) return *ta == *tb;
  if (t->is_named()) {
    auto n = t->as_named();
    for (auto &p : n->params) {
      if (occurs(ta, p)) return true;
    }
  }
  return false;
}




void infer::do_union(iir::var_type *ta, iir::type *tb) {
  if (*ta == *tb) return;
  // check for recursive types, can't quite do that yet
  if (occurs(ta, tb))
    throw infer::unify_error(ta, tb, "recursive type breaks unification");
  ta->points_to = tb;
}




void infer::unify(iir::type *ta, iir::type *tb) {
  // drop the types to their lowest forms
  auto t1 = find(ta);
  auto t2 = find(tb);

  // puts("unify", t1->str(), "and", t2->str());

  if (is_arrow(t1) && is_arrow(t2)) {
    auto f1 = t1->as_named();
    auto f2 = t2->as_named();

    auto a1 = f1->params[0];
    auto b1 = f1->params[1];

    auto a2 = f2->params[0];
    auto b2 = f2->params[1];


    unify(a1, a2);
    unify(b1, b2);

    // unify all the types in the methods
    return;
  }


  // unify two named types of the same name
  if (t1->is_named() && t2->is_named()) {
    auto n1 = t1->as_named();
    auto n2 = t2->as_named();
    if (n1->name != n2->name || n1->params.size() != n2->params.size()) {
      throw infer::unify_error(t1, t2, "type mismatch");
    }

    for (int i = 0; i < n1->params.size(); i++) {
      unify(n1->params[i], n2->params[i]);
    }
    return;
  }


  if (t1->is_var() && t2->is_var()) {
    auto tv = t1->as_var();
    tv->points_to = t2;
    return;
  }



  if (t1->is_var()) {
    auto tv = t1->as_var();
    puts("here");
    do_union(tv, t2);
    return;
  }

  if (t2->is_var()) {
    auto tv = t2->as_var();
    do_union(tv, t1);
    return;
  }

  if (*t1 != *t2) throw unify_error(t1, t2, "unify error, type mismatch");
};



// the constant values are the easiest to work with
infer::deduction iir::const_int::deduce(infer::context &) {
  return {iir::int_type};
}
infer::deduction iir::const_flt::deduce(infer::context &) {
  return {iir::float_type};
}



static infer::deduction deduce_ret(infer::context &gamma,
                                   iir::instruction *ins) {
  // returns need to know about the return type of their function, and therefore
  // will try to unify the return type of the function with the value of this
  // expression
  auto val = ins->args[0];
  auto vd = val->deduce(gamma);
  infer::unify(vd.type, gamma.fn->return_type());
  return vd;
}




// simple local allocation, usually on the stack. value is simply the type it is
// allocating for
static infer::deduction deduce_alloc(infer::context &gamma,
                                     iir::instruction *ins) {
  gamma[ins] = &ins->get_type();
  return {&ins->get_type()};
}


static infer::deduction deduce_store(infer::context &gamma,
                                     iir::instruction *ins) {
  auto dst = ins->args[0];
  auto src = ins->args[1];

  auto src_ded = src->deduce(gamma);

  auto dst_type = &dst->get_type();
  auto src_type = src_ded.type;

  try {
    infer::unify(dst_type, src_type);
  } catch (infer::unify_error &e) {
    die("UNIFICATION OF STORE FAILED");
  }

  gamma[ins] = &dst->get_type();
  return {&dst->get_type()};
}



static infer::deduction deduce_load(infer::context &gamma,
                                    iir::instruction *ins) {
  auto src = ins->args[0];
  src->deduce(gamma);
  gamma[ins] = &src->get_type();
  return {&src->get_type()};
}




static infer::deduction deduce_call(infer::context &gamma,
                                    iir::instruction *ins) {
  // create an un-scoped variable type to represent the return type
  // of the function. We will unify this to figure out what it should be in the
  // end
  auto ret_type = &iir::new_variable_type();



  gamma[ins] = ret_type;
  return {ret_type};
}



infer::deduction iir::instruction::deduce(infer::context &gamma) {
  // if this instruction has already been analyzied, simply return the old value
  // case 1 in Algorithm W
  if (gamma.count(this) != 0) return {gamma.at(this)};

  switch (itype) {
    case inst_type::ret:
      return deduce_ret(gamma, this);

    case inst_type::alloc:
      return deduce_alloc(gamma, this);


    case inst_type::store:
      return deduce_store(gamma, this);


    case inst_type::load:
      return deduce_load(gamma, this);


    case inst_type::call:
      return deduce_call(gamma, this);

    case inst_type::br:
    case inst_type::jmp:
    case inst_type::global:
    case inst_type::add:
    case inst_type::sub:
    case inst_type::mul:
    case inst_type::div:
    case inst_type::invert:
    case inst_type::dot:
    case inst_type::cast:
    case inst_type::poparg:

    default:
      return {};
      throw infer::analyze_failure(this);
  }
}

infer::deduction iir::block::deduce(infer::context &gamma) {
  // list of substs for this block
  infer::subs S;
  for (auto &inst : this->insts) {
    auto ded = inst->deduce(gamma);
    gamma[inst] = ded.type;
    for (auto &sub : ded.S) {
      S.push_back(sub);
    }
    // TODO, run subst on the subst on gamma
  }


  if (terminated()) {
    terminator->deduce(gamma);
  }

  return {nullptr, S};
}


infer::deduction iir::func::deduce(infer::context &G) {
  infer::context gamma;
  gamma.fn = this;

  for (auto &b : this->blocks) {
    b->deduce(gamma);
  }
  G[this] = &get_type();


  auto ret = get_type().as_named()->params[1];

  if (has_var_type_definition(ret)) {
    puts("Cannot declare type variable in return type");
    throw infer::analyze_failure(this);
  }


  // TODO:
  // run over all the blocks and do some kind of substitution i guess

  return {&get_type()};
}



void infer::context::apply_subs(subs &S) { puts("applying subs"); }
