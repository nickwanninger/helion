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
#include <map>
#include <sstream>
#include <cctype>
#include <iostream>
#include <string>

using namespace helion;

token::token() { type = tok_eof; }

token::token(uint8_t t, text v) {
  type = t;
  val = v;
}

tokenizer::tokenizer(text src) {
  source = src;
  index = 0;
}

rune tokenizer::next() {
  auto c = peek();
  index++;
  return c;
}

rune tokenizer::peek() {
  if (index > source.size()) {
    return -1;
  }
  return source[index];
}

bool in_charset(wchar_t c, const wchar_t *set) {
  for (int i = 0; set[i]; i++) {
    if (set[i] == c) return true;
  }

  return false;
}

token tokenizer::lex() {

  return token();
}

