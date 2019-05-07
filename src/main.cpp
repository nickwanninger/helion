#include <helion.h>
#include <stdio.h>
#include <iostream>

using namespace helion;


int main(int argc, char **argv) {
  gc::set_stack_root(&argv);

  while (true) {
    int count = 10;
    void **ptrs = (void **)gc::malloc(sizeof(void *) * count);

    for (int i = 0; i < count; i++) {
      size_t size = rand() % 40;
      ptrs[i] = gc::malloc(size);
      memset(ptrs[i], 'a', size);
    }

    for (int i = 0; i < count; i++) {
      gc::free(ptrs[i]);
    }
    gc::free(ptrs);
  }
  return 0;
}
