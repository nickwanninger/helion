#include <helion.h>
#include <stdio.h>
#include <iostream>

using namespace helion;


int main(int argc, char **argv) {
  gc::set_stack_root(&argv);

  int count = 10;
  void **ptrs = (void**)gc::malloc(sizeof(void*) * count);

  for (int i = 0; i < count; i++) {
    ptrs[i] = gc::malloc(i);
    memset(ptrs[i], 'a', i);
  }


  for (int i = 0; i < count; i++) {
    gc::free(ptrs[i]);
  }
  return 0;
}
