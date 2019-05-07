#include <helion.h>
#include <stdio.h>
#include <iostream>
#include <memory>

using namespace helion;

#include <gc/gc.h>

struct foo {
  int a;
  int b;
};

#ifdef X86_64
#error "aaaaa"
#endif

int main(int argc, char **argv) {
  gc::set_stack_root(&argv);

  printf("%s %p %p\n", MACH_TYPE, DATASTART, DATAEND);
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
