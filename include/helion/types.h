// [License]
// MIT - See LICENSE.md file in the package.
#pragma once

#ifndef __HELION_TYPES_H__
#define __HELION_TYPES_H__

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
    bool primitive = false;
    type *extends = nullptr;
    // Type parameters, ie: Type{T, U} etc...
    std::vector<type *> params;
    std::vector<type *> fields;
    std::vector<function *> method;
  };

  class function : public type {};

};  // namespace helion

#endif
