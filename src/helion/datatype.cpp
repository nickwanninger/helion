// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/core.h>

using namespace helion;



// Any is the base supertype of all types.
// It must be asserted that the supertype is never null, and in
// a situation that it would be, it should be any instead
datatype *helion::any_type;
datatype *helion::int32_type;
datatype *helion::float32_type;


static std::vector<std::unique_ptr<datatype>> types;



void helion::init_types(void) {
  // the base Any type has Any as a supertype (recursively)
  any_type = &datatype::create("Any");
  int32_type = &datatype::create_integer("Int", 32);
  float32_type = &datatype::create_float("Float", 32);
}

// line for line implementation of the subtype algorithm from the julia paper.
// ([1209.5145v1] Julia: A Fast Dynamic Language for Technical, Algorithm 3)
//
// Check if A is <= B (A is a subtype or equal to B)
bool helion::subtype(datatype *A, datatype *B) {
  using ts = typeinfo::type_style;


  if (!A->specialized || !B->specialized)
    throw std::logic_error("unable to check subtype of unspecialized types");


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
  if (ti->style == typeinfo::type_style::INTEGER) {
    s += "Int";
    s += std::to_string(ti->bits);
    return s;
  } else if (ti->style == typeinfo::type_style::FLOATING) {
    s += "Float";
    s += std::to_string(ti->bits);
    return s;
  } else if (ti->style == typeinfo::type_style::OBJECT ||
             ti->style == typeinfo::type_style::TUPLE ||
             ti->style == typeinfo::type_style::UNION) {
    if (ti->style == typeinfo::type_style::OBJECT) s += ti->name;
    if (ti->style == typeinfo::type_style::TUPLE) s += "Tuple";
    if (ti->style == typeinfo::type_style::UNION) s += "Union";

    if (specialized) {
      if (param_types.size() > 0) {
        s += "<";
        for (size_t i = 0; i < param_types.size(); i++) {
          auto &v = param_types[i];
          if (v != nullptr) {
            s += v->str();
          }
          if (i < param_types.size() - 1) s += ", ";
        }
        s += ">";
      }
    } else {
      if (ti->param_names.size() > 0) {
        s += "<";
        for (size_t i = 0; i < ti->param_names.size(); i++) {
          s += ti->param_names[i];
          if (i < ti->param_names.size() - 1) s += ", ";
        }
        s += ">";
      }
    }

  } else {
    s += "";
  }

  return s;
}

datatype &datatype::create(std::string name, datatype &sup,
                           std::vector<std::string> params) {
  std::unique_ptr<datatype> t(new datatype(name, sup));
  t->ti->param_names = params;
  t->specialized = params.size() == 0;
  int tid = types.size();
  types.emplace_back(std::move(t));
  return *types[tid];
}



datatype &datatype::create_integer(std::string name, int bits) {
  std::unique_ptr<datatype> t(new datatype(name, *int32_type));
  int tid = types.size();
  t->ti->bits = bits;
  t->specialized = true;
  t->ti->style = typeinfo::type_style::INTEGER;
  types.emplace_back(std::move(t));
  return *types[tid];
}


datatype &datatype::create_float(std::string name, int bits) {
  std::unique_ptr<datatype> t(new datatype(name, *float32_type));
  int tid = types.size();
  t->ti->bits = bits;
  t->ti->specialized = true;
  t->ti->style = typeinfo::type_style::FLOATING;
  types.emplace_back(std::move(t));
  return *types[tid];
}

void datatype::add_field(std::string name, datatype *type) {
  field f;
  f.name = name;
  f.type = type;
  fields.push_back(f);
}



/*
datatype *datatype::specialize(std::vector<datatype *> params) {
  // very quick test
  if (params.size() != ti->param_names.size()) {
    std::string err;
    err += "Unable to specialize type ";
    err += ti->name;
    err += " with invalid number of parameters. Expected ";
    err += std::to_string(ti->param_names.size());
    err += ". Got ";
    err += std::to_string(params.size());
    throw std::logic_error(err.c_str());
  }
  if (specialized) return this;
  return nullptr;
}
*/


llvm::Type *datatype::to_llvm(void) {
  if (type_decl != nullptr) return type_decl;


  if (ti->style == typeinfo::type_style::INTEGER) {
    type_decl = llvm::Type::getIntNTy(llvm_ctx, ti->bits);
  } else if (ti->style == typeinfo::type_style::FLOATING) {
    if (ti->bits == 32) {
      type_decl = llvm::Type::getFloatTy(llvm_ctx);
    } else if (ti->bits == 64) {
      type_decl = llvm::Type::getDoubleTy(llvm_ctx);
    } else {
      throw std::logic_error("Floats must be 32 or 64 bit");
    }
  } else if (ti->style == typeinfo::type_style::OBJECT) {
    if (!specialized)
      throw std::logic_error(
          "cannot lower an unspecialized type to llvm::Type");
    std::string tname = str();
    std::vector<llvm::Type *> flds;
    // push back a voidptr type
    auto vd = llvm::Type::getInt8PtrTy(llvm_ctx);
    flds.push_back(vd);
    for (auto &f : fields) {
      flds.push_back(f.type->to_llvm());
    }

    auto stct = llvm::StructType::get(llvm_ctx, flds);
    type_decl = llvm::PointerType::get(stct, 0);

    // build an LLVM struct
  }

  return type_decl;
}
