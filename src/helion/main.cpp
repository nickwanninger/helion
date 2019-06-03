// [License]
// MIT - See LICENSE.md file in the package.

#include <gc/gc.h>
#include <helion.h>
#include <stdio.h>
#include <sys/socket.h>
#include <uv.h>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <sys/stat.h>

#define GC_THREADS
#include <gc/gc.h>

#include <dlfcn.h>

#define PULL_LLVM
#include <helion/core.h>

#include <CLI11.hpp>

using namespace helion;
using namespace std::string_literals;


extern "C" void GC_allow_register_threads();




void parse_args(jit::enviroment *, std::vector<std::string> &);



int main(int argc, char **argv) {
  CLI::App app;


  std::string driver_path = ":NONE";
  std::string driver_opts = ":NONE";
  app.add_option("-D,--driver", driver_path, "path to the driver dylib");
  app.add_option("-d,--driver_opts", driver_opts,
                 "options to pass into the driver");




  std::string entry_point;
  auto file_opt = app.add_option("entry point", entry_point, "the entry file");
  file_opt->required(true);

  /*
  std::string llvm_args;
  app.add_option("-C,--llvm-args", llvm_args, "options to pass to the llvm
  compiler"); puts("llvm args:", llvm_args);
  */

  app.allow_extras(true);

  CLI11_PARSE(app, argc, argv);

  // start the garbage collector
  GC_INIT();
  GC_allow_register_threads();


  helion::init();



  auto &A = datatype::create("A");
  auto &B = datatype::create("B", A);


  puts(A.str());
  puts(B.str());

  if (subtype(&A, &B)) {
    puts("subtype");
  }

  const char *ep_ptr = entry_point.c_str();


  // check that the file exists before trying to read it
  struct stat sinfo;
  int sres = stat(ep_ptr, &sinfo);
  if (sres != 0) {
    puts("Unable to open file", entry_point);
    return 1;
  }

  // print every token from the file
  text src = read_file(ep_ptr);

  try {
    auto res = parse_module(src, entry_point);
    puts(res->str());
  } catch (syntax_error &e) {
    puts(e.what());
  } catch (std::exception &e) {
    puts("Other error:", e.what());
  }
  return 0;
}

