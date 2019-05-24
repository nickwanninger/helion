// [License]
// MIT - See LICENSE.md file in the package.


#include <helion/ast.h>


using namespace helion;
using namespace helion::ast;


text ast::module::str(int depth) {
  text s;

  char ptr[32];
  sprintf(ptr, "# begin module %p\n\n", this);
  s += ptr;

  for (auto stmt : stmts) {
    s += stmt->str(0);
    s += "\n\n";
  }

  sprintf(ptr, "# end module %p\n", this);
  s += ptr;
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
  for (int i = 0; i < depth; i++) indent += "    ";
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


