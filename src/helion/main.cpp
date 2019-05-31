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

#define GC_THREADS
#include <gc/gc.h>

#include <dlfcn.h>

#define PULL_LLVM
#include <helion/core.h>

using namespace helion;
using namespace std::string_literals;

using namespace llvm;
using namespace llvm::orc;

static LLVMContext ctx;
static IRBuilder<> builder(ctx);
static std::unique_ptr<Module> mod;
static std::unique_ptr<legacy::FunctionPassManager> fpm;
static std::unique_ptr<legacy::PassManager> mpm;


static std::unique_ptr<helion::jit::enviroment> env;


extern "C" void GC_allow_register_threads();




void jit_test(void);


void parse_args(jit::enviroment *, std::vector<std::string> &);

int main(int argc, char **argv) {
  // start the garbage collector
  GC_INIT();
  GC_allow_register_threads();


  helion::init_codegen();

  return 0;


  /*

  std::vector<std::string> args;
  for (int i = 1; i < argc; i++) args.push_back(argv[i]);

  // create the enviroment
  auto env = std::make_unique<jit::enviroment>();

  parse_args(env.get(), args);

  // attempt to initialize the enviroment...
  // ie: setup drivers, etc...
  if (!env->init()) {
    printf("failed to initialize enviroment\n");
    return 1;
  }
  */



  // print every token from the file
  std::string path = env->entry_file;
  text src = read_file(path.c_str());


  try {
    auto res = parse_module(src, env->entry_file);
    puts(res->str());
    delete res;
  } catch (syntax_error &e) {
    puts(e.what());
  } catch (std::exception &e) {
    puts("Other error:", e.what());
  }
  return 0;
}


static auto usage(int status = 1) {
  puts("Usage: helion [<flags>] file");
  puts("Flags:");

  puts(" --driver, -D          Set the remote reference driver library");
  puts(" --opts, -O            Set the driver options (specified per-driver)");
  puts(" --help                Show this menu");
  puts(" --verbose, -v         Enable verbose output to stderr");

  puts();
  exit(status);
}



/**
 * parse arguments and initialize the enviroment. This function will print usage
 * and exit the program if the arguments are invalid.
 */
void parse_args(jit::enviroment *env, std::vector<std::string> &args) {
  bool found_file = false;
  for (size_t i = 0; i < args.size(); i++) {
    auto curr = args[i];


    bool more = i < args.size() - 1;
    bool tack = false;

    if (curr[0] == '-') {
      tack = true;
      while (curr[0] == '-') {
        curr.erase(0, 1);
      }
    }

    if (tack) {
      if (curr == "driver" || curr == "D") {
        if (!more) {
          puts(
              "--driver needs a path to a dynamic library to act as the "
              "driver");
          usage();
        }
        auto path = args[++i];
        env->has_driver = true;
        env->driver_path = path;
      } else if (curr == "driver-opts" || curr == "O") {
        if (!more) {
          puts("--driver-opts needs driver options as the next argument");
          usage();
        }
        env->driver_opts = args[++i];
      } else if (curr == "verbose" || curr == "v") {
        env->verbose = true;
      } else if (curr == "help") {
        usage(0);
      }


    } else {
      if (!more) {
        found_file = true;
        env->entry_file = curr;
      } else {
        puts("The file to run must be the last argument");
        usage();
      }
    }
  }
  if (!found_file) {
    puts("No file provided!");
    usage();
  }
}

