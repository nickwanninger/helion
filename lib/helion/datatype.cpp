// [License]
// MIT - See LICENSE.md file in the package.

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

static std::vector<std::unique_ptr<datatype>> types;
static ska::flat_hash_map<llvm::Type *, datatype *> llvm_to_datatype_map;


void helion::init_types(void) {
  // the base Any type has Any as a supertype (recursively)
  any_type = &datatype::create("Any");

  bool_type = &datatype::create_integer("Bool", 1);
  int8_type = &datatype::create_integer("Byte", 8);
  int16_type = &datatype::create_integer("Short", 16);
  int32_type = &datatype::create_integer("Int", 32);
  int64_type = &datatype::create_integer("Long", 64);

  float32_type = &datatype::create_float("Float", 32);
  float64_type = &datatype::create_float("Double", 64);

  generic_ptr_type =
      &datatype::create("ptr", int8_type->to_llvm()->getPointerTo());

  datatype_type = &datatype::create_struct("datatype");
  datatype_type->add_field("specialized", bool_type);
  datatype_type->add_field("completed", bool_type);

  datatype_type->to_llvm()->print(llvm::errs());
  puts();
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

    if (specialized) {
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
    } else {
      if (ti->param_names.size() > 0) {
        s += "{";
        for (size_t i = 0; i < ti->param_names.size(); i++) {
          s += ti->param_names[i];
          if (i < ti->param_names.size() - 1) s += ", ";
        }
        s += "}";
      }
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

datatype &datatype::create(std::string name, datatype &sup,
                           std::vector<std::string> params) {
  std::unique_ptr<datatype> t(new datatype(name, sup));
  t->ti->param_names = params;
  int tid = types.size();
  types.emplace_back(std::move(t));
  return *types[tid];
}



datatype &datatype::create_integer(std::string name, int bits) {
  std::unique_ptr<datatype> t(new datatype(name, *int32_type));
  int tid = types.size();
  t->ti->bits = bits;
  t->specialized = true;
  t->ti->style = type_style::INTEGER;
  types.emplace_back(std::move(t));
  return *types[tid];
}


datatype &datatype::create_float(std::string name, int bits) {
  std::unique_ptr<datatype> t(new datatype(name, *float32_type));
  int tid = types.size();
  t->ti->bits = bits;
  t->specialized = true;
  t->ti->style = type_style::FLOATING;
  types.emplace_back(std::move(t));
  return *types[tid];
}


datatype &datatype::create_struct(std::string name) {
  std::unique_ptr<datatype> t(new datatype(name, *any_type));
  int tid = types.size();
  t->specialized = true;
  t->ti->style = type_style::STRUCT;
  types.emplace_back(std::move(t));
  return *types[tid];
}

datatype &datatype::create_buffer(int len) {
  std::unique_ptr<datatype> t(new datatype("buffer", *any_type));
  int tid = types.size();
  t->specialized = true;
  t->type_decl = llvm::ArrayType::get(llvm::Type::getInt8Ty(llvm_ctx), len);
  t->ti->style = type_style::STRUCT;
  types.emplace_back(std::move(t));
  return *types[tid];
}


datatype &datatype::create(std::string name, llvm::Type *lt) {
  std::unique_ptr<datatype> t(new datatype(name, *any_type));
  int tid = types.size();
  t->specialized = true;
  t->type_decl = lt;
  t->ti->style = type_style::STRUCT;
  types.emplace_back(std::move(t));
  return *types[tid];
}



void datatype::add_field(std::string name, datatype *type) {
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
  } else if (ti->style == type_style::OBJECT) {
    std::string s = str();
    auto stct = llvm::StructType::create(llvm_ctx, s);
    type_decl = stct;

    std::string tname = str();
    std::vector<llvm::Type *> flds;
    // push back a voidptr type

    auto vd = llvm::Type::getInt8PtrTy(llvm_ctx);
    flds.push_back(vd);

    flds.push_back(any_type->to_llvm());
    // TODO(superclass): Add in superclass fields here.
    for (auto &f : fields) {
      llvm::Type *field = f.type->to_llvm();
      if (f.type->ti->style == type_style::OBJECT) {
        field = field->getPointerTo();
      }
      flds.push_back(field);
    }

    stct->setBody(flds);
  } else if (ti->style == type_style::SLICE) {
    std::string s = str();
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
  } else if (ti->style == type_style::STRUCT) {
    std::string s = str();
    auto stct = llvm::StructType::create(llvm_ctx, s);
    type_decl = stct;
    std::string tname = str();
    std::vector<llvm::Type *> flds;
    for (auto &f : fields) {
      llvm::Type *field = f.type->to_llvm();
      if (f.type->ti->style == type_style::OBJECT) {
        field = field->getPointerTo();
      }
      flds.push_back(field);
    }
    stct->setBody(flds);
  }

  {
    // lower the type to its lowest non-pointer
    // version and store that in the global map
    auto t = type_decl;
    while (t->isPointerTy()) t = t->getPointerElementType();
    llvm_to_datatype_map[t] = this;
  }

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

