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
  if (&points_to != this) {
    return points_to.str();
  }
  return name;
}


static std::atomic<int> next_type_num;


std::string helion::get_next_param_name(void) {
  std::string name = "t";
  name += std::to_string(next_type_num++);
  return name;
}

var_type &iir::new_variable_type(void) {
  return *gc::make_collected<var_type>(get_next_param_name());
}



type &iir::convert_type(std::shared_ptr<ast::type_node> n) {

  std::string name = n->name;

  std::vector<type *> params;
  for (auto &p : n->params) params.push_back(&convert_type(p));




  if (!n->parameter) {
    return *gc::make_collected<named_type>(name, params);
  } else {
    if (params.size() > 0) {
      throw std::logic_error("cannot have parameters on parameter type");
    }
    return *gc::make_collected<var_type>(name);
  }
  throw std::logic_error("UNKNOWN TYPE IN `IIR::CONVERT_TYPE`");
}

