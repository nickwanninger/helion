// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/ast.h>
#include <helion/core.h>
#include <helion/util.h>

using namespace helion;



// Any is the base supertype of all types.
// It must be asserted that the supertype is never null, and in
// a situation that it would be, it should be any instead
datatype *helion::any_type;
datatype *helion::bool_type;
datatype *helion::int8_type;
datatype *helion::int16_type;
datatype *helion::int32_type;
datatype *helion::int64_type;
datatype *helion::float32_type;
datatype *helion::float64_type;
datatype *helion::datatype_type;
datatype *helion::generic_ptr_type;

static std::vector<datatype *> types;
static ska::flat_hash_map<llvm::Type *, datatype *> llvm_to_datatype_map;


void helion::init_types(void) {
  // the base Any type has Any as a supertype (recursively)
  any_type = datatype::create("Any");

  bool_type = datatype::create_integer("Bool", 1);
  int8_type = datatype::create_integer("Byte", 8);
  int16_type = datatype::create_integer("Short", 16);
  int32_type = datatype::create_integer("Int", 32);
  int64_type = datatype::create_integer("Long", 64);

  float32_type = datatype::create_float("Float", 32);
  float64_type = datatype::create_float("Double", 64);

  generic_ptr_type =
      datatype::create("ptr", int8_type->to_llvm()->getPointerTo());

  datatype_type = datatype::create_struct("datatype");
  datatype_type->add_field("specialized", bool_type);
  datatype_type->add_field("completed", bool_type);
}

// line for line implementation of the subtype algorithm from the julia paper.
// ([1209.5145v1] Julia: A Fast Dynamic Language for Technical, Algorithm 3)
//
// Check if A is <= B (A is a subtype or equal to B)
bool helion::subtype(datatype *A, datatype *B) {
  using ts = type_style;

  if (A->ti->style != B->ti->style) return false;

  if (A->ti->style == ts::INTEGER || A->ti->style == ts::FLOATING) {
    return A->ti->bits <= B->ti->bits;
  }



  if (A->ti->style == ts::TUPLE) {
    if (A->param_types.size() != B->param_types.size()) {
      return false;
    }

    // all of the parameter types must be subtypes
    for (size_t i = 0; i < A->param_types.size(); i++) {
      auto &T = A->param_types[i];
      auto &S = B->param_types[i];
      // if T is not a subtype of S, then A isn't a subtype of B
      if (subtype(T, S) == false) {
        return false;
      }
    }
    return true;
  }

  if (A->ti->style == ts::OBJECT) {
    // if the objects are of different length, then !(A <= B)
    if (A->param_types.size() != B->param_types.size()) {
      return false;
    }
    // now we check if the base type A <= B by walking the inheritence list
    while (A != any_type) {
      // TODO: use type ids, not string names...
      if (A->ti->name == B->ti->name) {
        // all of the parameter types must be subtypes
        for (size_t i = 0; i < A->param_types.size(); i++) {
          auto &T = A->param_types[i];
          auto &S = B->param_types[i];
          // if T is not a subtype of S, then A isn't a subtype of B
          if (subtype(T, S) == false) {
            return false;
          }
        }

        return true;
      }
      A = A->ti->super;
    }

    // all the checks pass for object types
    return B == any_type;
  }


  return false;
}




text datatype::str() {
  text s;
  if (ti->style == type_style::INTEGER) {
    s += ti->name;
    return s;
  } else if (ti->style == type_style::FLOATING) {
    s += ti->name;
    return s;
  } else if (ti->style == type_style::OBJECT ||
             ti->style == type_style::TUPLE || ti->style == type_style::UNION ||
             ti->style == type_style::STRUCT) {
    // please ignore this terrible code.
    if (ti->style == type_style::OBJECT || ti->style == type_style::STRUCT)
      s += ti->name;
    if (ti->style == type_style::TUPLE) s += "Tuple";
    if (ti->style == type_style::UNION) s += "Union";

    if (param_types.size() > 0) {
      s += "{";
      for (size_t i = 0; i < param_types.size(); i++) {
        auto &v = param_types[i];
        if (v != nullptr) {
          s += v->str();
        }
        if (i < param_types.size() - 1) s += ", ";
      }
      s += "}";
    }

  } else if (ti->style == type_style::METHOD) {
    s += "Fn{";
    if (specialized) {
      for (size_t i = 1; i < param_types.size(); i++) {
        auto &param = param_types[i];
        s += param->str();
        if (i < param_types.size() - 1) {
          s += ", ";
        }
      }

      if (param_types[0] != nullptr) {
        s += " : ";
        s += param_types[0]->str();
      }
    } else {
      s += "UNKNOWN, UNEXPECTED UNSPECIALIZED METHOD TYPE";
    }
    s += "}";
  } else {
    s += "";
  }

  return s;
}

datatype *datatype::create(text name, datatype &sup, slice<text> params) {
  auto t = gc::make_collected<datatype>(name, sup);
  t->ti->param_names = params;
  int tid = types.size();
  types.emplace_back(t);
  return types[tid];
}



datatype *datatype::create_integer(text name, int bits) {
  auto t = gc::make_collected<datatype>(name, *int64_type);
  int tid = types.size();
  t->ti->bits = bits;
  t->specialized = true;
  t->ti->style = type_style::INTEGER;
  types.emplace_back(t);
  return types[tid];
}


datatype *datatype::create_float(text name, int bits) {
  auto t = gc::make_collected<datatype>(name, *float32_type);
  int tid = types.size();
  t->ti->bits = bits;
  t->specialized = true;
  t->ti->style = type_style::FLOATING;
  types.emplace_back(t);
  return types[tid];
}


datatype *datatype::create_struct(text name) {
  auto t = gc::make_collected<datatype>(name, *any_type);
  int tid = types.size();
  t->specialized = true;
  t->ti->style = type_style::STRUCT;
  types.emplace_back(t);
  return types[tid];
}

datatype *datatype::create_buffer(int len) {
  auto t = gc::make_collected<datatype>("buffer", *any_type);
  int tid = types.size();
  t->specialized = true;
  t->type_decl = llvm::ArrayType::get(llvm::Type::getInt8Ty(llvm_ctx), len);
  t->ti->style = type_style::STRUCT;
  types.emplace_back(t);
  return types[tid];
}


datatype *datatype::create(text name, llvm::Type *lt) {
  auto t = gc::make_collected<datatype>(name, *any_type);
  int tid = types.size();
  t->specialized = true;
  t->type_decl = lt;
  t->ti->style = type_style::STRUCT;
  types.emplace_back(t);
  return types[tid];
}



void datatype::add_field(text name, datatype *type) {
  for (auto &f : fields) {
    if (f.name == name) {
      f.type = type;
      return;
    }
  }

  field f;
  f.name = name;
  f.type = type;
  fields.push_back(f);
}




llvm::Type *datatype::to_llvm(void) {
  if (type_decl != nullptr) return type_decl;


  std::string s = str();

  if (ti->style == type_style::INTEGER) {
    type_decl = llvm::Type::getIntNTy(llvm_ctx, ti->bits);
  } else if (ti->style == type_style::FLOATING) {
    if (ti->bits == 32) {
      type_decl = llvm::Type::getFloatTy(llvm_ctx);
    } else if (ti->bits == 64) {
      type_decl = llvm::Type::getDoubleTy(llvm_ctx);
    } else {
      throw std::logic_error("Floats must be 32 or 64 bit");
    }
  } else if (is_obj() || is_struct()) {
    auto stct = llvm::StructType::create(llvm_ctx, s);
    type_decl = stct;
    std::string tname = str();
    std::vector<llvm::Type *> flds;
    // push back a voidptr type

    if (is_obj()) {
      // TODO(superclass): Add in superclass fields here.
      // flds.push_back(any_type->to_llvm());
    }
    for (auto &f : fields) {
      llvm::Type *field = f.type->to_llvm();
      if (f.type->is_obj()) {
        field = field->getPointerTo();
      }
      flds.push_back(field);
    }
    stct->setBody(flds);

  } else if (ti->style == type_style::SLICE) {
    auto stct = llvm::StructType::create(llvm_ctx, s);

    std::vector<llvm::Type *> flds;

    flds.push_back(llvm::Type::getInt32Ty(llvm_ctx));
    flds.push_back(llvm::Type::getInt32Ty(llvm_ctx));

    for (auto &f : fields) {
      llvm::Type *field = f.type->to_llvm();
      if (f.type->ti->style == type_style::OBJECT) {
        field = field->getPointerTo();
      }
      flds.push_back(field->getPointerTo());
    }

    type_decl = stct;
  }

  {
    // lower the type to its lowest non-pointer
    // version and store that in the global map
    auto t = type_decl;
    while (t->isPointerTy()) t = t->getPointerElementType();
    llvm_to_datatype_map[t] = this;
  }


  type_decl->print(llvm::errs());
  puts();
  return type_decl;
}


datatype *datatype::from(llvm::Value *v) {
  return datatype::from(v->getType());
}

datatype *datatype::from(llvm::Type *t) {
  // reduce the type to the lowest non-pointer version, as that is what
  // is stored in the global map
  while (t->isPointerTy()) t = t->getPointerElementType();
  // if the llvm type is not found in the map, return nothing
  if (llvm_to_datatype_map.count(t) == 0) return nullptr;
  return llvm_to_datatype_map.at(t);
}




text datatype::mangled_name(void) {
  text n;
  if (is_obj()) {
    n += "o.";
    n += ti->name;
  }
  if (is_int()) {
    n += "i";
    n += std::to_string(ti->bits);
    return n;
  }
  if (is_flt()) {
    if (ti->bits == 32) return "float";
    if (ti->bits == 64) return "double";
    n += "f";
    n += std::to_string(ti->bits);
    return n;
  }

  if (is_union()) n += "union.";
  if (is_tuple()) n += "tuple.";
  if (is_method()) n += "method.";
  if (is_slice()) n += "slice.";

  if (is_struct()) {
    n += "s.";
    n += ti->name;
  }

  if (param_types.size() > 0) {
    text ps;
    for (int i = 0; i < param_types.size(); i++) {
      auto name = param_types[i]->mangled_name();
      ps += "_";
      ps += std::to_string(name.size());
      ps += name;
    }

    n += "$";
    n += std::to_string(ps.size());
    n += ps;
  }

  return n;
}




static datatype *spec_type_node(std::shared_ptr<ast::type_node> &tn,
                                cg_scope *sc) {
  /**
   * Type nodes can be difficult. This is because a typenode can be a reference
   * to a type, or to an alias through type parameters. There are two times we
   * want to parameterize on a type: when the name of the type_node == dt, or
   * when we have parameters on the typenode.
   */
  auto dt = sc->find_type(tn->name);
  if (dt == nullptr) {
    std::string err;
    err += "unable to find type ";
    err += tn->name;
    throw std::logic_error(err);
  }


  // the only times we don't specialize on a type is when the names aren't the
  // same, and there are no parameters on the typenode
  if (!(dt->ti->name == tn->name) && tn->params.size() == 0) return dt;
  slice<datatype *> params;

  for (auto &p : tn->params) {
    params.push_back(spec_type_node(p, sc));
  }

  return dt->ti->specialize(params, sc);
}



datatype *typeinfo::specialize(slice<datatype *> &params, cg_scope *sc) {
  // the param list must be the same as the expected number of parameters
  if (params.size() != param_names.size()) {
    std::string err;
    err += "Invalid number of parameters to type ";
    err += name;
    err += ". Expected ";
    err += std::to_string(param_names.size());
    err += ", got ";
    err += std::to_string(params.size());
    throw std::logic_error(err);
  }


  // first, search through the specialization list for an existing
  // specialization
  for (auto &dt : specializations) {
    // if the param types are the same, this is the specialization we want
    if (params == dt->param_types) {
      return dt;
    }
  }
  // we didn't find the type, so we need to make a new specialization
  auto *dt = gc::make_collected<datatype>(this);
  dt->param_types = params;
  dt->specialized = true;
  specializations.push_back(dt);

  // create a scope to calculate the specialization in
  auto ns = sc->spawn();

  // loop over and fill in the new scope with the parameters we are asked for
  for (size_t i = 0; i < param_names.size(); i++) {
    ns->set_type(param_names[i], params[i]);
  }


  // some types don't have a definition
  if (def) {
    // go through and add all the fields
    for (auto &f : def->fields) {
      datatype *ft = spec_type_node(f.type, ns);
      // puts("field", f.name, "is", ft->str(), "| originally", f.type->name);
      dt->add_field(f.name, ft);
    }
  }

  return dt;
}


datatype *typeinfo::specialize(cg_scope *sc) {
  slice<datatype *> params;

  auto ns = sc->spawn();

  for (auto &name : param_names) {
    auto p = ns->find_type(name);
    if (p == nullptr) {
      puts(name);

      ns->print_types();
      throw std::logic_error("CANNOT FIND TYPE!");
    }
    params.push_back(p);
  }
  return specialize(params, ns);
}

