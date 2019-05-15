// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#include <helion/text.h>
#include <memory>
#include <stdexcept>
#include <vector>




namespace helion {


  enum tok_type {
#define TOKEN(name, code, s) name = code,
#include "tokens.inc"
#undef TOKEN
  };

  class unexpected_eof_error : public std::exception {
   private:
    std::string _what = nullptr;

   public:
    unexpected_eof_error(std::string what_arg) : _what(what_arg) {}
    const char* what() const noexcept { return (const char*)_what.c_str(); };
  };

  // a token represents a single atomic lexeme that gets parsed
  // out of a string with cedar::parser::lexer
  class token {
   public:
    std::shared_ptr<text> source;
    size_t line;
    size_t col;
    int8_t type;
    text val;
    token(uint8_t, text, std::shared_ptr<text>, size_t line, size_t col);
  };

  inline std::ostream& operator<<(std::ostream& os, const token& tok) {
    static const char* tok_names[] = {
#define TOKEN(name, code, s) s,
#include "tokens.inc"
#undef TOKEN
    };
    text buf;
    buf += '(';
    buf += tok_names[tok.type];
    buf += ", ";
    buf += '\'';
    buf += tok.val;
    buf += "' <";
    buf += std::to_string(tok.line);
    buf += ",";
    buf += std::to_string(tok.line);
    buf += ">)";
    os << buf;
    return os;
  }


  // a lexer takes in a unicode string and will allow you to
  // call .lex() that returns a new token for each token in a stream
  class tokenizer {
   private:
    size_t index = 0;
    size_t line = 0;
    size_t column = 0;

    // depth is the indent depth of the current line
    size_t depth = 0;
    int depth_delta = 0;
    text indent;
    std::shared_ptr<text> source;
    rune next();
    rune peek();

    /**
     * emit will create a token with line number information and everything
     * according to the current state in the tokenizer
     */
    token emit(uint8_t, text);
    token emit(uint8_t);

    void panic(std::string msg);

   public:
    text get_line(long);
    explicit tokenizer(text);
    token lex();
  };

}  // namespace helion

