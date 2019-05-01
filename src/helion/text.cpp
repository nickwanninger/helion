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
#include <codecvt>
#include <iostream>
#include <locale>
#include <string>



using namespace helion;


static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> unicode_conv;

static std::string to_utf8(std::u32string const& s) {
  return unicode_conv.to_bytes(s);
}

static std::u32string to_utf32(std::string const & s) {
	return unicode_conv.from_bytes(s);
}


void text::ingest_utf8(std::string const& s) { buf = to_utf32(s); }


text::text() {}


text::text(const char* s) { ingest_utf8(std::string(s)); }

text::text(char* s) { ingest_utf8(std::string(s)); }


text::text(const char32_t* s) {
  buf.clear();
  for (uint64_t l = 0; s[l] != '\0'; l++) {
    buf.push_back(s[l]);
  }
}

text::text(std::string const& s) { ingest_utf8(s); }
// copy constructor
text::text(const helion::text& other) {
  buf = other.buf;
  return;
}


text::text(std::u32string const& s) { buf = s; }


text& text::operator=(const text& o) {
  buf = o.buf;
  return *this;
}


text::~text() {}


text::iterator text::begin(void) { return buf.begin(); }
text::iterator text::end(void) { return buf.end(); }

text::const_iterator text::cbegin(void) const { return buf.cbegin(); }
text::const_iterator text::cend(void) const { return buf.cend(); }

uint32_t text::size(void) { return buf.size(); }

uint32_t text::length(void) { return buf.size(); }

uint32_t text::max_size(void) { return buf.max_size(); }

uint32_t text::capacity(void) { return buf.capacity(); }

void text::clear(void) { buf.clear(); }

bool text::empty(void) { return buf.empty(); }


text& text::operator+=(const text& other) {
  for (auto it = other.cbegin(); it != other.cend(); it++) {
    buf.push_back(*it);
  }
  return *this;
}


text& text::operator+=(const char* other) { return operator+=(text(other)); }


text& text::operator+=(std::string const& other) { return operator+=(text(other)); }


text& text::operator+=(char c) {
  buf.push_back(c);
  return *this;
}
text& text::operator+=(int c) {
  buf.push_back(c);
  return *this;
}

text& text::operator+=(rune c) {
  buf.push_back(c);
  return *this;
}

bool text::operator==(const text& other) { return other.buf == buf; }
bool text::operator==(const text& other) const { return other.buf == buf; }


text::operator std::string() const { return to_utf8(buf); }
text::operator std::u32string() const { return buf; }

rune text::operator[](size_t i) { return buf[i]; }

rune text::operator[](size_t i) const { return buf[i]; }

void text::push_back(rune& r) { buf.push_back(r); }

void text::push_back(rune&& r) { buf.push_back(r); }
