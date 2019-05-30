// [License]
// MIT - See LICENSE.md file in the package.

#include <dlfcn.h>  // for dlopen and co.
#include <helion/jit.h>
#include <helion/util.h>
#include <gc/gc.h>

using namespace helion;
using namespace helion::jit;




bool jit::enviroment::init(void) {
  if (has_driver) {
    // try to load the driver

    driver_library_handle = dlopen(driver_path.c_str(), RTLD_LAZY);

    if (driver_library_handle == nullptr) {
      puts("unable to open driver at", driver_path);
      return false;
    }

    auto *driver_init = (int (*)(const char *))dlsym(driver_library_handle, "helion_driver_init");
    if (driver_init == nullptr) {
      puts("helion_driver_init function not found in driver");
      return false;
    }



    auto *_driver_alloc = (uint64_t (*)(int))dlsym(driver_library_handle, "helion_driver_alloc");
    if (_driver_alloc == nullptr) {
      puts("helion_driver_alloc function not found in driver");
      return false;
    }
    driver_alloc = _driver_alloc;

    auto *_driver_free = (int (*)(uint64_t))dlsym(driver_library_handle, "helion_driver_free");
    if (_driver_free == nullptr) {
      puts("helion_driver_free function not found in driver");
      return false;
    }
    driver_free = _driver_free;


    auto *_driver_read = (int (*)(uint64_t, int, int, void*))dlsym(driver_library_handle, "helion_driver_read");
    if (_driver_read == nullptr) {
      puts("helion_driver_read function not found in driver");
      return false;
    }
    driver_read = _driver_read;


    auto *_driver_write = (int (*)(uint64_t, int, int, void*))dlsym(driver_library_handle, "helion_driver_write");
    if (_driver_write == nullptr) {
      puts("helion_driver_write function not found in driver");
      return false;
    }
    driver_write = _driver_write;

    driver_init(driver_opts.c_str());

  } else {

    // some default non-remote remote functions...
    // TODO just don't use remote variables if there is no remote
    //      driver :)
    driver_alloc = [](int sz) -> uint64_t {
      return (uint64_t)GC_MALLOC(sz);
    };

    driver_free = [](uint64_t addr) -> int {
      GC_FREE((void*)addr);
      return 1;
    };


    driver_read = [](uint64_t addr, int off, int len, void *dst) -> int {
      auto *buf = reinterpret_cast<char*>(addr);
      memcpy(dst, buf + off, len);
      return 1;
    };


    driver_write = [](uint64_t addr, int off, int len, void *src) -> int {
      auto *dst = reinterpret_cast<char*>(addr);
      memcpy(dst + off, src, len);
      return 1;
    };
    
  }


  return true;
}

jit::enviroment::~enviroment() {
  // clean up the driver library
  if (driver_library_handle != nullptr) {
    dlclose(driver_library_handle);
  }
}
