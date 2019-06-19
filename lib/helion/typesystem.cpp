// [License]
// MIT - See LICENSE.md file in the package.


#include <helion/ast.h>
#include <helion/core.h>
#include <helion/gc.h>
#include <helion/iir.h>
#include <immer/set.hpp>



using namespace helion;
using namespace helion::iir;



bool iir::operator==(type &a, type &b) {
  auto hf = std::hash<type>();
  return hf(a) == hf(b);
}

named_type *type::as_named(void) {
  return is_named() ? static_cast<named_type *>(this) : nullptr;
}

var_type *type::as_var(void) {
  return is_var() ? static_cast<var_type *>(this) : nullptr;
}


std::string named_type::str(void) {
  std::string s;

  if (name == "->") {
    s += params[0]->str();
    s += " -> ";
    s += params[1]->str();
    return s;
  }

  if (name == "()") {
    s += "(";
    for (size_t i = 0; i < params.size(); i++) {
      auto &param = params[i];
      s += param->str();
      if (i < params.size() - 1) {
        s += ", ";
      }
    }
    s += ")";
    return s;
  }


  s += name;

  for (auto &param : params) {
    s += " ";
    s += param->str();
  }
  return s;
}


std::string var_type::str(void) {
  if (points_to != nullptr && points_to != this) {
    return points_to->str();
  }
  return name;
}


std::string helion::get_next_param_name(void) {
  static std::atomic<int> next_type_num = 0;
  std::string name = "%";
  name += std::to_string(next_type_num++);
  return name;
}



var_type &iir::new_variable_type(void) {
  return *gc::make_collected<var_type>(get_next_param_name());
}


type *iir::convert_type(std::shared_ptr<ast::type_node> n, iir::scope *sc) {
  std::string name = n->name;
  std::vector<type *> params;
  for (auto &p : n->params) params.push_back(convert_type(p, sc));

  if (!n->parameter) {
    return gc::make_collected<named_type>(name, params);
  } else {
    if (params.size() > 0) {
      throw std::logic_error("cannot have parameters on parameter type");
    }


    if (sc == nullptr) {
      die("scope cannot be null when converting a variable datatype");
    }
    // if the type is a variable, check first for a definition in the scope.
    auto found = sc->find_vtype(name);
    if (found != nullptr) return found;
    auto new_var = gc::make_collected<var_type>(name);
    sc->set_vtype(name, new_var);
    return new_var;
  }
  throw std::logic_error("UNKNOWN TYPE IN `IIR::CONVERT_TYPE`");
}


type *iir::convert_type(std::string s, iir::scope *sc) {
  return convert_type(ast::parse_type(s), sc);
}
