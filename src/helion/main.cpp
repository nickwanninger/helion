#include <helion.h>
#include <stdio.h>
#include <iostream>
#include <memory>

using namespace helion;



struct foo {
  int a;
  int b;
};

int main(int argc, char **argv) {
  gc::set_stack_root(&argv);

  printf("%s\n", OS_TYPE);
  int i = 0;
  while (false) {
    i++;
    std::shared_ptr<foo> ptr;

    ptr = std::make_shared<foo>();
    ptr->a = i;
    printf("%d\n", ptr->a);
  }
  return 0;
}
