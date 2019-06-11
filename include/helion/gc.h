// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __HELION_GC__
#define __HELION_GC__

#include <utility>

namespace helion {
  namespace gc {


    void *raw_alloc(int);
    void raw_free(void *);


    template <typename T, typename... Args>
    inline T *make_collected(Args &&... args) {
      T *thing = reinterpret_cast<T *>(raw_alloc(sizeof(T)));
      new (thing) T(std::forward<Args>(args)...);
      return thing;
    }


    inline void *alloc(int m) { return raw_alloc(m); }
    inline void free(void *p) { return raw_free(p); }
  };  // namespace gc
};    // namespace helion

#endif
