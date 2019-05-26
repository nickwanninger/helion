// [License]
// MIT - See LICENSE.md file in the package.
#pragma once

#ifndef __HELION_TYPES_H__
#define __HELION_TYPES_H__

#include <helion/text.h>
#include <vector>

namespace helion {

  // forward declaration
  class function;

  /**
   * A type in helion is an abstract representation of a set of values.
   * Types can have fields of other types, which they inherit from parent types.
   */
  class type {
   public:
    class field {
     public:
      type *typ;
      text name;
    };

    // if the type is primitive, this means it is represented by a set number of
    // bits.
    bool primitive = false;
    int bitcount = 0;

    // once a type is done being created, it is considered 'locked' which means no
    // code can update it. Types are immutable as a result
    bool locked = false;

    type *extends = nullptr;
    std::vector<field> fields;
    std::vector<function *> method;
  };

  class function : public type {};

};  // namespace helion

#endif
