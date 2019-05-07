#include <helion.h>
#include <stdio.h>
#include <iostream>

using namespace helion;


int main(int argc, char **argv) {
  gc::set_stack_root(&argv);

  int count = 10;
  void **ptrs = (void**)gc::malloc(sizeof(void*) * count);

  for (int i = 0; i < count; i++) {
    printf("%d %p\n", i, ptrs + i);
    ptrs[i] = gc::malloc(i);
  }
  return 0;
}
