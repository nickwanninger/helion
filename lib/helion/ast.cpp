// [License]
// MIT - See LICENSE.md file in the package.


#include <cxxabi.h>
#include <helion/ast.h>
#include <helion/pstate.h>
#include <atomic>

using namespace helion;
using namespace helion::ast;


text ast::module::str(int depth) {
  text s;


  for (auto d : typedefs) {
    s += d->str(0);
    s += "\n";
  }


  if (globals.size() > 0) {
    s += "# global variables\n";
    for (auto d : globals) {
      s += d->str(0);
      s += "\n";
    }
    s += "\n";
  }


  for (auto d : stmts) {
    s += d->str(0);
    s += "\n";
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
  // s += "(";
  s += left->str();
  s += " ";
  s += op;
  s += " ";
  s += right->str();
  // s += ")";
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


std::atomic<int> var_index = 0;
ast::var_decl::var_decl(scope* s) : node(s) { ind = var_index++; }

text ast::var_decl::str(int d) {
  text s;

  if (!is_arg) s += "let ";
  if (global) s += "global ";

  s += name;

  if (type != nullptr) {
    s += ": ";
    s += type->str();
  }

  if (value != nullptr) {
    s += " = ";
    s += value->str(d);
  }
  return s;
}

text ast::var::str(int) {
  if (global) return global_name;
  return decl->name;
}



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
  for (int i = 0; i < depth; i++) indent += ".  ";

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


text ast::typeassert::str(int) {
  text s;
  s += "(";
  s += val->str();
  s += " :: ";
  s += type->str();
  s += ")";
  return s;
}


text ast::type_node::str(int) {
  text s;


  if (name == "->") {
    if (params[0]->params.size() == 1) {
      s += params[0]->params[0]->str();
    } else {
      s += params[0]->str();
    }
    s += " -> ";
    s += params[1]->str();
    return s;
  }

  if (name == "()") {
    s += "(";
    for (size_t i = 0; i < params.size(); i++) {
      auto& param = params[i];
      s += param->str();
      if (i < params.size() - 1) {
        s += ", ";
      }
    }
    s += ")";
    return s;
  }


  s += name;

  for (auto& param : params) {
    s += " ";
    s += param->str();
  }
  return s;
}


text ast::prototype::str(int) {
  text s;

  s += "(";
  for (size_t i = 0; i < args.size(); i++) {
    auto& arg = args[i];
    s += arg->str();
    if (i < args.size() - 1) {
      s += ", ";
    }
  }
  s += ")";

  if (type != nullptr) {
    auto typ = type->params.back();
    s += ": ";
    s += typ->str();
  }
  return s;
}

text ast::func::str(int depth) {
  text indent = "";
  for (int i = 0; i < depth; i++) indent += "  ";

  text s;

  s += "[";

  int i = 0;
  for (auto& c : captures) {
    i++;
    s += c;
    if (i < captures.size() - 1) s += ", ";
  }

  s += "]";

  if (proto != nullptr) {
    s += proto->str();
  } else {
    s += "()";
  }
  s += " => ";

  s += stmt->str(depth);
  return s;
}


text ast::def::str(int depth) {
  text indent = "";
  for (int i = 0; i < depth; i++) indent += "  ";

  text s;

  s += "def ";
  s += fn->name;
  s += " ";

  if (fn->proto != nullptr) s += fn->proto->str();
  s += "\n";
  for (auto& e : fn->stmt->as<ast::do_block*>()->exprs) {
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

  for (auto& field : fields) {
    s += indent;
    s += "  ";
    s += field.type->str();
    s += " ";
    s += field.name;
    s += "\n";
  }


  for (auto& def : defs) {
    s += indent;
    s += "  ";
    s += def->str(depth + 1);
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


  s += "if ";
  s += cond->str();
  s += " then ";
  s += true_expr->str(depth);

  if (false_expr != nullptr) {
    s += " else ";
    s += false_expr->str(depth);
  }

  return s;
}
