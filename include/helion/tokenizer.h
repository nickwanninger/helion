// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#include <helion/text.h>
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
    int8_t type;
    text val;
    token();
    token(uint8_t, text);
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
    buf += "')";

    os << buf;

    return os;
  }

  // a lexer takes in a unicode string and will allow you to
  // call .lex() that returns a new token for each token in a stream
  class tokenizer {
   private:
    uint64_t index = 0;
    text source;
    token lex_num();
    rune next();
    rune peek();

   public:
    explicit tokenizer(text);
    token lex();
  };

  /*
  class reader {
   private:
    lexer *m_lexer;

    std::vector<token> tokens;
    token tok;
    uint64_t index = 0;

    token peek(int);
    token move(int);

    token next(void);
    token prev(void);

   public:
    explicit reader();

    void lex_source(cedar::runes);
    ref read_one(bool*);

    // simply read the top level expressions until a
    // tok_eof is encountered, returning the list
    std::vector<ref> run(cedar::runes);
    // the main switch parsing function. Callable and
    // will parse any object, delegating to other functions
    ref parse_expr(void);
    // parse list literals
    ref parse_list(void);

    ref parse_vector(void);
    ref parse_dict(void);

    // parse things like
    //    'a  -> (quote a)
    //    `a  -> (quasiquote a)
    //    `@a -> (quasiunquote a)
    ref parse_special_syntax(cedar::runes);

    // parse symbols (variable names)
    // and keyword variants
    ref parse_symbol(void);
    ref parse_string(void);

    ref parse_number(void);
    ref parse_hash_modifier(void);
    ref parse_backslash_lambda(void);

    ref parse_special_grouping_as_call(cedar::runes name, tok_type closing);
  };
  */

}  // namespace helion

