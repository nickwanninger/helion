// [License]
// MIT - See LICENSE.md file in the package.

#include <helion/core.h>
#include <helion/ast.h>

using namespace helion;


helion::method::method(module *m) {
  mod = m;
}


text helion::method::str(void) {
  text s;

  s += "# Method\n";

  for (auto &fn : definitions) {
    s += fn->str();
    s += "\n";
  }

  return s;
}
