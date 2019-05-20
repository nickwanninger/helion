// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __PSTATE_H__
#define __PSTATE_H__

#include <helion/util.h>
#include <helion/tokenizer.h>

namespace helion {

  class pstate {
    int ind = 0;
    std::shared_ptr<tokenizer> tokn;

   public:
    inline pstate() {
      ind = 0;
      tokn = nullptr;
    }
    inline pstate(std::shared_ptr<tokenizer> t, int i = 0) {
      ind = i;
      tokn = t;
    }

    inline pstate(text src, int i = 0) {
      ind = i;
      tokn = std::make_shared<tokenizer>(src);
    }

    inline token first(void) {
      static auto eof = token(tok_eof, "", nullptr, 0, 0);
      if (tokn != nullptr) {
        return tokn->get(ind);
      } else {
        return eof;
      }
    }
    inline pstate next(void) { return pstate(tokn, ind + 1); }
    inline bool done(void) { return first().type == tok_eof; }

    inline operator bool(void) { return !done(); }
    inline operator token(void) { return first(); }
    inline pstate operator++(int) {
      pstate self = *this;
      *this = next();
      return self;
    }


    inline pstate operator--(int) {
      pstate self = *this;
      ind--;
      return self;
    }
  };




}  // namespace helion


#endif
