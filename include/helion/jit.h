// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __HELION_JIT__
#define __HELION_JIT__


/**
 * for now, helion only works on x86_64, so the JIT should check this
 */
#ifndef __x86_64__
#error "Helion does not support your architecture. Requires x86_64"
#endif

#include <asmjit/src/asmjit/asmjit.h>

#include <vector>  // for std::vector

namespace helion {

  /**
   * the jit namespace holds all the jit compiler related functions,
   * classes, and sructs.
   */
  namespace jit {

    /**
     * code handles are what hold code so the GC can clear them
     * up and free the executable pages when they are no longer
     * needed.
     */
    class code_handle {
      // TODO(stub)
    };

    class compiler {
      // TODO(stub)
    };




    /**
     * func_sig allows the compiler to generate function signatures
     * dynamically for the JIT instead of relying on asmjit's
     * FuncSignatureN<..> type for everything.
     *
     * This type is very short-lived, and exists only to build a sig
     * dynamically.
     */
    class func_sig {
      // return type starts out as void
      uint8_t ret = asmjit::TypeIdOf<void>::kTypeId;
      // arguments to the function, these are only ever added to.
      std::vector<uint8_t> args;

     public:
      /**
       * add an argument from the template parameter V
       */
      template <typename V>
      inline void add_arg() {
        args.push_back(asmjit::TypeIdOf<V>::kTypeId);
      }

      /**
       * set the return type via a template function call. It will
       * grab the type from the template argument and use that
       * to determine the internal type to set the return to.
       */
      template <typename R>
      inline void set_ret() {
        ret = asmjit::TypeIdOf<R>::kTypeId;
      }

      /**
       * finalize the function signature after building
       * it up by setting the return and the args previously
       */
      inline asmjit::FuncSignature finalize() {
        uint32_t ccId = asmjit::CallConv::kIdHost;
        asmjit::FuncSignature f;
        f.init(ccId, ret, args.data(), args.size());
        return f;
      }
    };

  }  // namespace jit

}  // namespace helion

#endif
