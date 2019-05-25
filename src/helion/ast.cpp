// [License]
// MIT - See LICENSE.md file in the package.


#include <cxxabi.h>
#include <helion/ast.h>

using namespace helion;
using namespace helion::ast;


text ast::module::str(int depth) {
  text s;

  char ptr[32];
  sprintf(ptr, "# begin module %p\n\n", this);
  s += ptr;

  for (auto stmt : stmts) {
    s += "# ";
    int status;
    char* demangled = abi::__cxa_demangle(typeid(*stmt).name(), 0, 0, &status);
    s += demangled;
    free(demangled);
    sprintf(ptr, " at %p\n", stmt);
    s += ptr;
    s += stmt->str(0);
    s += "\n\n";
  }

  return s;
}




text ast::number::str(int) {
  if (type == floating) {
    return std::to_string(as.floating);
  } else {
    return std::to_string(as.integer);
  }
}




text ast::argument::str(int) {
  text s;
  s += type;
  s += " ";
  s += name;
  return s;
}




text ast::def::str(int depth) {
  text indent = "";
  for (int i = 0; i < depth; i++) indent += "  ";
  text s;
  s += indent;
  s += "def ";

  s += dst->str();
  s += " ";
  for (size_t i = 0; i < args.size(); i++) {
    s += args[i]->str();
    if (i < args.size() - 1) {
      s += ", ";
    }
  }
  s += "\n";

  /**
   * TODO: stringify the body
   */
  s += indent;
  s += "# body...\n";

  s += indent;
  s += "end";
  return s;
}



text ast::binary_op::str(int depth) {
  text s;
  s += "(";
  s += left->str();
  s += " ";
  s += op;
  s += " ";
  s += right->str();
  s += ")";
  return s;
}


text ast::dot::str(int) {
  text s;
  // s += "(";
  s += expr->str();
  s += ".";
  s += sub;
  // s += ")";
  return s;
}


text ast::subscript::str(int) {
  text s;
  s += "(";
  s += expr->str();
  s += "[";
  for (size_t i = 0; i < subs.size(); i++) {
    auto& v = subs[i];
    if (v != nullptr) {
      s += v->str();
    }
    if (i < subs.size() - 1) s += ", ";
  }
  s += "])";
  return s;
}


text ast::var::str(int) { return value; }


text ast::call::str(int) {
  text t;
  t += func->str();
  t += "(";
  for (size_t i = 0; i < args.size(); i++) {
    auto& v = args[i];
    if (v != nullptr) {
      t += v->str();
    }
    if (i < args.size() - 1) t += ",";
  }
  t += ")";
  return t;
}



text ast::tuple::str(int) {
  text t;
  t += "(";
  for (size_t i = 0; i < vals.size(); i++) {
    auto& v = vals[i];
    if (v != nullptr) {
      t += v->str();
    }
    if (vals.size() == 1) {
      t += ",";
      break;
    }
    if (i < vals.size() - 1) t += ", ";
  }
  t += ")";
  return t;
}



text ast::string::str(int) {
  text s;
  s += "'";
  s += val;
  s += "'";
  return s;
}





text ast::do_block::str(int depth) {


  text indent = "";
  for (int i = 0; i < depth; i++) indent += "  ";

  text s;

  s += "do\n";
  for (auto e : exprs) {
    s += indent;
    s += "  ";
    s += e->str(depth+1);
    s += "\n";
  }
  s += indent;
  s += "end";

  return s;
}
