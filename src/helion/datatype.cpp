// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/core.h>

using namespace helion;



// Any is the base supertype of all types.
// It must be asserted that the supertype is never null, and in
// a situation that it would be, it should be any instead
datatype *helion::any_type;


static std::vector<std::unique_ptr<datatype>> types;



void helion::init_types(void) {
  // the base Any type has Any as a supertype (recursively)
  any_type = &datatype::create("Any");
}

// line for line implementation of the subtype algorithm from the julia paper.
// ([1209.5145v1] Julia: A Fast Dynamic Language for Technical, Algorithm 3)
//
// Check if A is <= B (A is a subtype or equal to B)
bool helion::subtype(datatype *A, datatype *B) {
  using ts = datatype::type_style;


  if (A->style != B->style) return false;

  if (A->style == ts::INTEGER || A->style == ts::FLOATING) {
    return A->bits <= B->bits;
  }



  if (A->style == ts::TUPLE) {
    if (A->parameters.size() != B->parameters.size()) {
      return false;
    }

    // all of the parameter types must be subtypes
    for (size_t i = 0; i < A->parameters.size(); i++) {
      auto &T = A->parameters[i];
      auto &S = B->parameters[i];
      // if T is not a subtype of S, then A isn't a subtype of B
      if (subtype(T, S) == false) {
        return false;
      }
    }
    return true;
  }

  if (A->style == ts::OBJECT) {
    // if the objects are of different length, then !(A <= B)
    if (A->parameters.size() != B->parameters.size()) {
      return false;
    }



    // now we check if the base type A <= B by walking the inheritence list
    while (A != any_type) {
      // TODO: use type ids, not string names...
      if (A->name == B->name) {
        // all of the parameter types must be subtypes
        for (size_t i = 0; i < A->parameters.size(); i++) {
          auto &T = A->parameters[i];
          auto &S = B->parameters[i];
          // if T is not a subtype of S, then A isn't a subtype of B
          if (subtype(T, S) == false) {
            return false;
          }
        }

        return true;
      }
      A = &A->super;
    }

    // all the checks pass for object types
    return B == any_type;
  }


  return false;
}




text datatype::str() {
  text s;
  if (style == type_style::INTEGER) {
    s += "Int";
    s += std::to_string(bits);
    return s;
  } else if (style == type_style::FLOATING) {
    s += "Float";
    s += std::to_string(bits);
    return s;
  } else if (style == type_style::OBJECT || style == type_style::TUPLE ||
             style == type_style::UNION) {
    if (style == type_style::OBJECT) s += name;
    if (style == type_style::TUPLE) s += "Tuple";
    if (style == type_style::UNION) s += "Union";

    if (parameters.size() > 0) {
      s += "<";
      for (size_t i = 0; i < parameters.size(); i++) {
        auto &v = parameters[i];
        if (v != nullptr) {
          s += v->str();
        }
        if (i < parameters.size() - 1) s += ", ";
      }
      s += ">";
    }
  } else {
    s += "Unkown!!!!";
  }

  return s;
}

datatype &datatype::create(std::string name, datatype &sup,
                           std::vector<datatype *> params) {
  std::unique_ptr<datatype> t(new datatype(name, sup));
  t->parameters = params;
  int tid = types.size();
  t->tid = tid;

  types.emplace_back(std::move(t));
  return *types[tid];
}
