/*
 * MIT License
 *
 * Copyright (c) 2018 Nick Wanninger
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <helion/text.h>
#include <helion/tokenizer.h>
#include <helion/util.h>
#include <cctype>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

using namespace helion;


static auto is_space(rune c) { return c == ' ' || c == '\t'; }

token::token(uint8_t t, text v, std::shared_ptr<text> src, size_t l, size_t c) {
  type = t;
  val = v;
  source = src;
  line = l;
  col = c;
}

tokenizer::tokenizer(text src, text pa) {
  path = pa;
  source = std::make_shared<text>(src);
  tokens = std::make_shared<std::vector<token>>();
  index = 0;
}



token tokenizer::get(size_t i) {
  if ((int)i < 0) {
    return token(tok_eof, "", nullptr, 0, 0);
  }

  if (i >= tokens->size()) {
    if (!done) {
      lex();
      return get(i);
    } else {
      return token(tok_eof, "", nullptr, 0, 0);
    }
  }
  return tokens->at(i);
}



token tokenizer::emit(uint8_t t, text v) {
  token tok(t, v, source, line, column);
  if (last_emit_ended != -1)
    if (is_space(source->operator[](last_emit_ended - 1)))
      tok.space_before = true;
  last_emit_ended = index;
  tokens->push_back(tok);
  return tok;
}


rune tokenizer::next() {
  auto c = peek();
  // increment book-keeping information
  index++;
  column++;
  if (c == '\n') {
    line++;
    column = 0;
  }
  return c;
}

rune tokenizer::peek() {
  if (index > source->size()) {
    return -1;
  }
  return source->operator[](index);
}

bool in_charset(rune c, const text set) {
  for (int i = 0; set[i]; i++) {
    if (set[i] == c) return true;
  }
  return false;
}



static auto in_set(text &set, rune c) {
  for (auto &n : set) {
    if (n == c) return true;
  }
  return false;
}

token tokenizer::lex() {
top:

  last_emit_ended = index;



  /**
   * a lambda function that accepts as long as the next rune is
   * in the set of runes supplied by the parameter `set`
   */
  auto accept_run = [&](text set) {
    text buf;
    while (in_set(set, peek())) {
      buf += next();
    }
    return buf;
  };



#ifdef DO_INDENT
  /**
   * handle indentation and dedents.
   * single newline lines are removed by the newline parser.
   * if we get here, and the column == 0, we need to check if we indent
   * or dedent.
   */
  if (group_depth == 0) {
    if (depth_delta > 0) {
      depth_delta--;
      depth++;
      // printf("INDENT\n");
      return emit(tok_indent);
    }
    if (depth > 0) {
      if (depth_delta < 0) {
        depth_delta++;
        depth--;
        // printf("DEDENT\n");
        return emit(tok_dedent);
      }
    }

    if (column == 0) {
      // if the depth is 0, we have no indentation. Therefore we need to check
      // for indentation and possibly make that indentation the new indent text
      if (depth == 0) {
        // if the first char in the line is a space, ' ' or '\t', we need to
        // accept that entire run of spaces or tabs as the indent.
        if (is_space(peek())) {
          text in = accept_run(" \t");
          indent = in;
          depth_delta = 1;
          goto top;
        }
      } else {
        // if we aren't at the start of an indentation, (depth==0), we need to
        // figure out what the indentation is, and set that to needs_to_dedent.
        if (is_space(peek())) {
          text spaces = accept_run(" \t");

          int indent_len = indent.size();
          int spaces_len = spaces.size();
          // first check is if the space char count is a multiple of the
          // indentation char count.
          if (spaces_len % indent_len != 0) {
            throw std::logic_error("invalid indentation");
          }
          // nd : newdepth, the depth of this indentation
          int nd = 0;
          int s = 0;
          while (s < spaces_len) {
            for (int i = 0; i < indent_len; i++) {
              if (spaces[s] == indent[i]) {
                s++;
                nd++;
              } else {
                throw std::logic_error("invalid indentation");
              }
            }
          }
          depth_delta = nd - depth;
          goto top;
        } else {
          depth_delta = -depth;
          goto top;
        }
      }
    }
  }

#endif

  int32_t c = next();

  // newlines and semicolons are considered 'terminator' characters.
  // they signify the end of a line or other construct
  if (c == '\n' || c == ';') {
    while (peek() == '\n' || peek() == ';') {
      c = next();
    }
    /*  if (group_depth != 0) {
     *    goto top;
     *  }
     */
    return emit(tok_term, ";");
  }

  if (c == '#') {
    while (peek() != '\n' && (int32_t)peek() != -1) next();
    if (peek() == '\n') accept_run("\n");
    goto top;
  }

  if ((int32_t)c == -1 || c == 0) {
    // at the end of the file, we need to make sure all indents are completed.
    // This makes it so the lexer returns dedents until the depth becomes zero,
    // so any parser that uses the tokenizer can generate valid tokens for the
    // grammar
    if (depth > 0) {
      depth--;
      return emit(tok_dedent, "");
    }
    /**
     * this is where the tokenizer could be considered done
     */
    done = true;
    return emit(tok_eof, "");
  }


  /**
   * a complicated parsing system within helion is the parsing of
   * indentation and detents.
   */
  if (is_space(c)) {
    // go back to the top of the tokenizer
    goto top;
  }


  // the basic C escape codes
  static std::map<char, char> esc_mappings = {
      {'a', 0x07}, {'b', 0x08},  {'f', 0x0C}, {'n', 0x0A},
      {'r', 0x0D}, {'t', 0x09},  {'v', 0x0B}, {'\\', 0x5C},
      {'"', 0x22}, {'\'', '\''}, {'e', 0x1B},
  };



  if (c == '(') {
    group_depth++;
    return emit(tok_left_paren, "(");
  }
  if (c == ')') {
    group_depth--;
    return emit(tok_right_paren, ")");
  }


  if (c == '[') {
    group_depth++;
    return emit(tok_left_square, "[");
  }
  if (c == ']') {
    group_depth--;
    return emit(tok_right_square, "]");
  }
  if (c == '{') {
    group_depth++;
    return emit(tok_left_curly, "{");
  }
  if (c == '}') {
    group_depth--;
    return emit(tok_right_curly, "}");
  }


  if (c == ',') return emit(tok_comma, ",");


  if (c == '"' || c == '\'') {
    bool double_quotes = c == '"';

    text buf;

    bool escaped = false;
    // ignore the first quote because a string shouldn't
    // contain the encapsulating quotes in it's internal representation
    while (true) {
      c = next();
      if ((int32_t)c == -1) throw std::logic_error("unterminated string");
      if (c == (double_quotes ? '"' : '\'')) break;
      escaped = c == '\\';
      if (escaped) {
        char e = next();
        if (e == 'U' || e == 'u') {
          std::string hex;
          int l = 8;
          if (e == 'u') l = 4;
          for (int i = 0; i < l; i++) {
            hex += next();
          }
          std::cout << hex << std::endl;
          c = (int32_t)std::stoul(hex, nullptr, 16);
          printf("%u\n", c);
        } else {
          c = esc_mappings[e];
          if (c == 0) {
            throw std::logic_error("unknown escape sequence in string");
          }
        }
        escaped = false;
      }
      buf += c;
    }
    return emit(tok_str, buf);
  }




  // parse a number
  if (isdigit(c) || c == '.' || (c == '-' && isdigit(peek()))) {
    // it's a number (or it should be) so we should parse it as such
    text buf;
    buf += c;

    /*
    if (peek() == 'x') {
      next();
      text hex = accept_run("0123456789abcdefABCDEF");
      ssize_t x;
      std::stringstream ss;
      ss << std::hex << hex;
      ss >> x;
      auto s = std::to_string(x);
      return emit(tok_num, s);
    }

    if (peek() == 'b') {
      next();
      text binary = accept_run("01");
      unsigned long long x = 0;
      for (auto bit : binary) {
        x <<= 1;
        if (bit == '1') x |= 1;
      }
      auto s = std::to_string(x);
      return emit(tok_num, s);
    }
    */


    while (isdigit(peek()) || peek() == '.') {
      c = next();
      buf += c;
    }

    if (!(buf == ".")) {
      return emit(tok_num, buf);
    } else {
      return emit(tok_dot, ".");
    }
  }  // digit parsing




  // operator parsing
  static text operators = "?&\\*+-/%!=<>≤≥≠.←|&^";



  static std::map<std::string, uint8_t> op_mappings = {
      {"=", tok_assign},  {"==", tok_equal},   {"!=", tok_notequal},
      {">", tok_gt},      {">=", tok_gte},     {"<", tok_lt},
      {"<=", tok_lte},    {"+", tok_add},      {"-", tok_sub},
      {"*", tok_mul},     {"/", tok_div},      {".", tok_dot},
      {"->", tok_arrow},  {"|", tok_pipe},     {",", tok_comma},
      {"%", tok_mod},     {"::", tok_is_type}, {":", tok_colon},
      {"?", tok_question}};


  if (in_charset(c, operators)) {
    std::string op;
    op += c;

    while (in_charset(peek(), operators)) {
      auto v = next();
      if ((rune)v == (rune)-1 || v == 0) break;
      op += v;
    }


    if (op_mappings.count(op) != 0) {
      auto t = op_mappings[op];
      return emit(t, op);
    } else {
      std::string e;
      e += "invalid operator: ";
      e += op;
      throw std::logic_error(e.c_str());
    }
  }

  // now all we can do is parse ids and keywords


  text symbol;
  symbol += c;


  /*
  if (peek() == ':') {
    auto v = next();
    symbol += v;
  }
  */

  while (!in_charset(peek(), " :;\n\t(){}[],") &&
         !in_charset(peek(), operators)) {
    auto v = next();
    if ((int32_t)v == -1 || v == 0) break;
    symbol += v;
  }

  if (symbol.length() == 0)
    throw std::logic_error("lexer encountered zero-length identifier");

  uint8_t type = tok_var;

  if (symbol[0] == ':') {
    if (symbol.size() == 1) return emit(tok_colon, ":");
    type = tok_keyword;
  } else {
    // TODO(unicode)
    if (symbol[0] >= 'A' && symbol[0] <= 'Z') {
      // type = tok_type;
    }
    if (symbol[0] == '@') {
      text s;
      for (uint32_t i = 1; i < symbol.size(); i++) {
        s += symbol[i];
      }
      symbol = s;
      if (s.size() == 0) throw std::logic_error("invalid @ symbol syntax.");
      type = tok_self_var;
    }
  }

  /*
  // absorb question marks into symbols but not into types
  if (type != tok_type) {
    while (!in_charset(peek(), "?:")) {
      auto v = next();
      if ((int32_t)v == -1 || v == 0) break;
      symbol += v;
    }
  }
  */


  static std::map<std::string, uint8_t> special_token_type_map = {
      {"::", tok_is_type},  {"def", tok_def},         {"or", tok_or},
      {"let", tok_let},     {"const", tok_const},     {"global", tok_global},
      {"some", tok_some},   {"and", tok_and},         {"not", tok_not},
      {"do", tok_do},       {"if", tok_if},           {"then", tok_then},
      {"else", tok_else},   {"elif", tok_elif},       {"for", tok_for},
      {"while", tok_while}, {"return", tok_return},   {"type", tok_typedef},
      {"end", tok_end},     {"extends", tok_extends}, {"nil", tok_nil}};
  if (special_token_type_map.count(symbol) != 0) {
    type = special_token_type_map[symbol];
  }

  return emit(type, symbol);
}



text tokenizer::get_line(long want) {
  text &src = *source;
  text s;
  int cln = 0;
  size_t ind = 0;

  while (cln < want && ind < src.size()) {
    if (src[ind++] == '\n') {
      cln++;
    }
  }
  if (ind == src.size() - 1) {
    return "unable to find line!";
  }
  while (ind < src.size() && src[ind] != '\n') s += src[ind++];
  return s;
}


/**
 * TODO(ease-of-use) throw a better tokenization error type
 *                   to allow graceful recovery
 */
void tokenizer::panic(std::string msg) {
  puts("Error in Tokenization!");
  throw std::logic_error("TOKENIZATION FAILURE");
}
