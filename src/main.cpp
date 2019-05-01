#include <helion.h>
#include <stdio.h>
#include <iostream>

using namespace helion;



void *recur(int d) {
  auto *p = (int *)gc::alloc(sizeof(int));
  gc::collect();
  *p = d;
  if (d > 0) {
    recur(d - 1);
  } else {
    // gc::collect();
  }
  return p;
}


struct node {
  int val = 0;
  node *next;
};


node *make_list(int size) {
  node *n = nullptr;
  for (int i = 0; i < size; i++) {
    auto *c = (node *)gc::alloc(sizeof(node));
    c->next = n;
    c->val = i;
    n = c;
  }
  return n;
}

int main(int argc, char **argv) {
  gc::set_stack_root(&argv);
  unsigned char i = 0;
  while (true) {
    auto *n = make_list(i);

    i++;
    n = nullptr;
    gc::collect();
  }
  return 0;
}
