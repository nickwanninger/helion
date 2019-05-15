#include <helion.h>
#include <stdio.h>
#include <iostream>
#include <memory>

using namespace helion;

#include <gc/gc.h>
#include <unordered_map>
#include <vector>


int *deopt;

struct foo {
  int i;
  int j;
};

int main(int argc, char **argv) {
  text src = read_file(argv[1]);
  tokenizer t(src);
  while (true) {
    auto tok = t.lex();
    if (tok.type == tok_eof) break;
    puts("Tok:", tok);
  }
  return 0;
}
