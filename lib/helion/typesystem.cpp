// [License]
// MIT - See LICENSE.md file in the package.


#include <helion/ast.h>
#include <helion/core.h>
#include <helion/gc.h>
#include <helion/iir.h>
#include <immer/set.hpp>



using namespace helion;
using namespace helion::iir;


namespace std {
  template <>
  struct hash<type> {
    size_t operator()(type &t) const {
      size_t x = 0;
      size_t y = 0;
      size_t mult = 1000003UL;  // prime multiplier

      x = 0x345678UL;
      if (t.is_var()) {
        auto v = t.as_var();
        x = 0x098172354UL;
        x ^= std::hash<std::string>()(v->name);
        return x;
      } else if (t.is_named()) {
        auto v = t.as_named();
        x = 0x856819292UL;
        x ^= std::hash<std::string>()(v->name);
        for (auto *p : v->params) {
          y = operator()(*p);
          x = (x ^ y) * mult;
          mult += (size_t)(852520UL + 2);
        }
        return x;
      }

      return x;
    }
  };


  template <>
  struct hash<type *> {
    size_t operator()(type *t) const { return std::hash<type>()(*t); }
  };
}  // namespace std


named_type *type::as_named(void) {
  return is_named() ? static_cast<named_type *>(this) : nullptr;
}

var_type *type::as_var(void) {
  return is_var() ? static_cast<var_type *>(this) : nullptr;
}


std::string named_type::str(void) {
  std::string s;

  if (name == "->") {
    s += "(";
    s += params[0]->str();
    s += " -> ";
    s += params[1]->str();
    s += ")";
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

