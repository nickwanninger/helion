// [License]
// MIT - See LICENSE.md file in the package.
#pragma once

#ifndef __HELION_INFER_H__
#define __HELION_INFER_H__

#include "../immer/map.hpp"
#include "util.h"
#include <vector>

namespace helion {
  namespace iir {
    class type;
    class var_type;
    class value;
    class func;
  };  // namespace iir

  /*
   * an implementation of a varient of the Hindley-Milner type inferencer
   * (algorithm W) for the iir datastructures
   */
  namespace infer {








    struct substitution {
      iir::var_type *from;
      iir::type *to;
    };



    using subs = std::vector<substitution>;


    struct deduction {
      iir::type *type;
      subs S;
    };



    class context : public std::unordered_map<iir::value *, iir::type *> {
     public:
       iir::func *fn;
       void apply_subs(subs &S);
    };

    // using context = immer::map<iir::value*, iir::type*>;

    iir::type *find(iir::type *t);

    void do_union(iir::var_type *, iir::type *);

    void unify(iir::type *, iir::type *);

    iir::type *inst(iir::type *, context &);


    class analyze_failure : std::runtime_error {
     public:
      analyze_failure(iir::value *val)
          : std::runtime_error("failure analyzing expression"), val(val) {}
      iir::value *val;
    };


    class unify_error : std::runtime_error {
     public:
      unify_error(iir::type *t1, iir::type *t2, std::string err)
          : std::runtime_error(err), t1(t1), t2(t2) {}
      iir::type *t1, *t2;
    };
  };  // namespace infer
}  // namespace helion

#endif
