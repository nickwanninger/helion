// [License]
// MIT - See LICENSE.md file in the package.


#include <helion/ast.h>
#include <helion/core.h>
#include <helion/gc.h>
#include <helion/iir.h>

using namespace helion;
using namespace helion::iir;


named_type *type::as_named(void) {
  return is_named() ? static_cast<named_type *>(this) : nullptr;
}

method_type *type::as_method(void) {
  return is_method() ? static_cast<method_type *>(this) : nullptr;
}

var_type *type::as_var(void) {
  return is_var() ? static_cast<var_type *>(this) : nullptr;
}


std::string named_type::str(void) {
  std::string s;
  s += m_name;

  if (m_params.size() > 0) {
    s += "{";
    for (int i = 0; i < m_params.size(); i++) {
      s += m_params[i]->str();
      if (i < m_params.size() - 1) s += ", ";
    }
    s += "}";
  }
  return s;
}



std::string method_type::str(void) {
  std::string s;
  s += "(";
  s += m_types[0]->str();
  for (int i = 1; i < m_types.size(); i++) {
    s += " -> ";
    s += m_types[i]->str();
  }
  s += ")";
  return s;
}


std::string var_type::str(void) {
  if (&points_to != this) {
    return points_to.str();
  }
  return name;
}


static std::atomic<int> next_type_num;


static std::string get_next_var_name(void) {
  std::string name = "v";
  name += std::to_string(next_type_num++);
  return name;
}

var_type &iir::new_variable_type(void) {
  return *gc::make_collected<var_type>(get_next_var_name());
}



type &iir::convert_type(std::shared_ptr<ast::type_node> n) {

  if (n->style == type_style::METHOD) {
    std::vector<type *> args;
    for (auto &arg : n->params) {
      args.push_back(&convert_type(arg));
    }
    auto &t = *gc::make_collected<method_type>(args);
    return t;
  }

  if (n->style == type_style::OBJECT) {
    std::vector<type *> params;
    for (auto &p : n->params) params.push_back(&convert_type(p));
    auto &t = *gc::make_collected<named_type>(n->name, params);
    return t;
  }

  throw std::logic_error("UNKNOWN TYPE IN `IIR::CONVERT_TYPE`");
}
