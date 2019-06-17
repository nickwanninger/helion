// [License]
// MIT - See LICENSE.md file in the package.
#pragma once

#ifndef __HELION_INFER_H__
#define __HELION_INFER_H__

#include "iir.h"

#include "../immer/map.hpp"

namespace helion {



  /*
   * an implementation of a varient of the Hindley-Milner type inferencer
   * (algorithm W) for the iir datastructures
   */
  namespace infer {

    using context = immer::map<iir::value*, iir::type*>;

    iir::type *find(iir::type *t);

    void do_union(iir::var_type *, iir::type*);

    void unify(iir::type *, iir::type *);

    iir::type *inst(iir::type *, context &);
  };
}  // namespace helion

#endif
