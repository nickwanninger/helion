// [License]
// MIT - See LICENSE.md file in the package.

#include <gc/gc.h>
#include <helion.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <uv.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

#define GC_THREADS
#include <gc/gc.h>

#include <helion/core.h>
#include <helion/iir.h>

#include <immer/set.hpp>

#include <CLI11.hpp>

using namespace helion;
using namespace std::string_literals;

extern "C" void GC_allow_register_threads();


int main(int argc, char **argv) {
  // start the garbage collector
  GC_INIT();
  GC_allow_register_threads();


  CLI::App app("helion");
  std::string driver_path = ":NONE";
  std::string driver_opts = ":NONE";
  app.add_option("-D,--driver", driver_path, "path to the driver dylib");
  app.add_option("-d,--driver_opts", driver_opts,
                 "options to pass into the driver");

  std::string entry_point;
  auto file_opt = app.add_option("entry point", entry_point, "the entry file");
  file_opt->required(true);

  app.allow_extras(true);

  CLI11_PARSE(app, argc, argv);


  helion::init();

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
    // puts(res->str());
    compile_module(std::move(res));
  } catch (syntax_error &e) {
    puts(e.what());
  }
  return 0;
}

