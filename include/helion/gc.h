// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __HELION_GC__
#define __HELION_GC__

#include <mutex>

namespace helion {
  namespace gc {



    class state {
    };

    void set_stack_root(void*);
    void *alloc(size_t);
    void free(void*);
    void collect(void);
  }
}  // namespace helion


#endif
