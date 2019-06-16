// [License]
// MIT - See LICENSE.md file in the package.


#pragma once
#ifndef __HELION_UTIL__
#define __HELION_UTIL__


#include <helion/text.h>
#include <stdarg.h>  // For va_start, etc.
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>


namespace helion {


  // some short wrappers around shared_ptr and make_shared
  template <typename T>
  using rc = std::shared_ptr<T>;

  template <class _Tp, class... _Args>
  auto make(_Args &&... __args) {
    return std::shared_ptr<_Tp>::make_shared(std::forward<_Args>(__args)...);
  }

  inline text read_file(char *filename) {
    auto wif = std::ifstream(filename);
    wif.imbue(std::locale());
    std::stringstream wss;
    wss << wif.rdbuf();
    std::string o = wss.str();
    return o;
  }


  inline text read_file(const char *filename) {
    return read_file(const_cast<char *>(filename));
  }
  inline text read_file(text &filename) {
    std::string fn = filename;
    return read_file(fn.c_str());
  }


  template <typename... Args>
  inline void puts(Args... args) {
    (void)(int[]){0, ((void)(std::cout << args << " "), 0)...};
    std::cout << std::endl;
  }


  template <typename... Args>
  inline void die(Args... args) {
    (void)(int[]){0, ((void)(std::cout << args << " "), 0)...};
    std::cout << std::endl;
    exit(1);
  }



  inline std::string strfmt(const std::string fmt, ...) {
    int size =
        ((int)fmt.size()) * 2 + 50;  // Use a rubric appropriate for your code
    std::string str;
    va_list ap;
    while (1) {  // Maximum two passes on a POSIX system...
      str.resize(size);
      va_start(ap, fmt);
      int n = vsnprintf((char *)str.data(), size, fmt.c_str(), ap);
      va_end(ap);
      if (n > -1 && n < size) {  // Everything worked
        str.resize(n);
        return str;
      }
      if (n > -1)      // Needed size returned
        size = n + 1;  // For null char
      else
        size *= 2;  // Guess at a larger size (OS specific)
    }
    return str;
  }



  template <typename T>
  struct ptr_hash {
    size_t operator()(T t) const {
      return std::hash<typename std::remove_pointer<T>::type>()(*t);
    }
  };


  template <typename T>
  struct ptr_equal {
    constexpr bool operator()(const T lhs, const T rhs) const {
      return *lhs == *rhs;
    }
  };

  template <typename K, typename V>
  using ptr_map = std::unordered_map<K, V, ptr_hash<K>, ptr_equal<K>>;


  template <typename K>
    class ptr_set : public std::unordered_set<K, ptr_hash<K>, ptr_equal<K>> {
      public:
        inline bool contains(const K& key) {
          return this->count(key) != 0;
        }
    };

}  // namespace helion

#endif
