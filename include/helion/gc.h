// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __HELION_GC__
#define __HELION_GC__


namespace helion {
  namespace gc {
    void *alloc(int);
    void free(void*);
  };
};

#endif
