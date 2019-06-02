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
    ast::node* p = stmt.get();
    char* demangled = abi::__cxa_demangle(typeid(*p).name(), 0, 0, &status);
    s += demangled;
    free(demangled);
    sprintf(ptr, " at %p\n", stmt.get());
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
    if (i < args.size() - 1) t += ", ";
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


text ast::keyword::str(int) {
  text s;
  s += val;
  return s;
}

text ast::nil::str(int) {
  text s;
  s += "nil";
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
    s += e->str(depth + 1);
    s += "\n";
  }
  s += indent;
  s += "end";

  return s;
}



text ast::type_node::str(int) {
  text s;

  if (type == NORMAL_TYPE) {
    s += name;

    if (params.size() > 0) {
      s += "<";

      for (size_t i = 0; i < params.size(); i++) {
        auto& param = params[i];
        s += param->str();
        if (i < params.size() - 1) {
          s += ", ";
        }
      }
      s += ">";
    }
    return s;
  }
  if (type == SLICE_TYPE) {
    s += "[";
    s += params[0]->str();
    s += "]";
    return s;
  }
  return s;
}


text ast::prototype::str(int) {
  text s;

  s += "(";

  for (size_t i = 0; i < args.size(); i++) {
    auto& arg = args[i];
    if (arg.type != nullptr) {
      s += arg.type->str();
      s += " ";
    } else {
      s += "Auto ";
    }
    s += arg.name;
    if (i < args.size() - 1) {
      s += ", ";
    }
  }
  s += ")";


  s += " : ";

  if (return_type != nullptr) {
    s += return_type->str();
  } else {
    s += "Auto";
  }
  return s;
}

text ast::func::str(int depth) {
  text s;
  s += proto->str();
  s += " -> ";
  s += body->str(depth + 1);
  return s;
}


text ast::def::str(int depth) {
  text indent = "";
  for (int i = 0; i < depth; i++) indent += "  ";

  text s;

  s += "def ";
  s += name;
  s += " ";
  if (proto != nullptr) {
    s += proto->str();
  }
  s += "\n";
  for (auto& e : exprs) {
    s += indent;
    s += "  ";
    s += e->str(depth + 1);
    s += "\n";
  }
  s += indent;
  s += "end";
  return s;
}


text ast::typedef_node::str(int depth) {
  text indent = "";
  for (int i = 0; i < depth; i++) indent += "  ";

  text s;
  s += "type ";
  s += type->str();
  if (extends != nullptr) {
    s += " extends ";
    s += extends->str();
  }
  s += "\n";


  s += indent;
  s += "  ";
  s += "# Fields\n";
  for (auto & field : fields) {
    s += indent;
    s += "  ";
    s += field.type->str();
    s += " ";
    s += field.name;
    s += "\n";
  }


  s += indent;
  s += "  ";
  s += "# Methods\n";
  for (auto & def : defs) {
    s += indent;
    s += "  ";
    s += def->str(depth+1);
    s += "\n";
  }

  s += indent;
  s += "end";
  return s;
}


text ast::return_node::str(int depth) {
  text s;
  s += "return ";
  s += val->str(depth + 1);
  return s;
}


text ast::if_node::str(int depth) {
  text s;


  text indent = "";
  for (int i = 0; i < depth; i++) indent += "  ";


  bool printed_if = false;
  for (size_t i = 0; i < conds.size(); i++) {
    auto& c = conds[i];

    if (printed_if) {
      s += indent;
    }

    if (c.cond) {
      s += !printed_if ? "if " : "elif ";
      printed_if = true;
      s += c.cond->str();
      s += " then\n";

    } else {
      s += "else\n";
    }
    for (auto& e : c.body) {
      s += indent;
      s += "  ";
      s += e->str(depth + 1);
      s += "\n";
    }
  }
  s += indent;
  s += "end";
  return s;
}
