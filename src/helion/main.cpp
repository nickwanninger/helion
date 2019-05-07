#include <helion.h>
#include <stdio.h>
#include <iostream>
#include <memory>

using namespace helion;



struct foo {
  int a;
  int b;
};

foo *deop;

int main(int argc, char **argv) {
  gc::set_stack_root(&argv);

  int i = 0;
  while (true) {
    i++;
    std::shared_ptr<foo> ptr;

    ptr = std::make_shared<foo>();
    ptr->a = i;
    printf("%d\n", ptr->a);
  }
  return 0;
}
