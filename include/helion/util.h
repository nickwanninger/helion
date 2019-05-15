// [License]
// MIT - See LICENSE.md file in the package.


#pragma once
#ifndef __HELION_UTIL__
#define __HELION_UTIL__


#include <helion/text.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <sstream>


namespace helion {
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
  inline void puts(Args ... args) {
    (void)(int[]){0, ((void)(std::cout << args << " "), 0)...};
    std::cout << std::endl;
  }

}  // namespace helion

#endif
