// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __HELION_SLICE_JIT__
#define __HELION_SLICE_JIT__

#include <helion/gc.h>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>


namespace helion {
  /**
   * a slice is the internal representation of an array within helion. It is
   * meant to hold references and values only, no special-move-semantic classes
   * from c++'s std::
   *
   * the slice struct must only be used with garbage collected data. No
   * unique_ptrs, shared_ptrs, etc.. because if you append to a slice and a new
   * buffer of memory is needed, the old one will not be freed and instead, will
   * be left to be garbage collected.
   */
  template <typename T>
  class slice {
   protected:
    int len = 0;
    int cap = 0;
    T *m_data = nullptr;


    friend std::hash<slice<T>>;

   public:
    typedef T *iterator;
    typedef const T *const_iterator;

    T *begin() { return m_data; }
    T *end() { return m_data + len; }
    const T *begin() const { return m_data; }
    const T *end() const { return m_data + len; }



    inline slice() { reserve(1); }
    inline explicit slice(int count) {
      reserve(count);
      len = cap;
    }

    inline slice(slice<T> &o) { *this = o; }

    inline slice(std::initializer_list<T> init) {
      reserve(init.size());
      for (auto &el : init) {
        push_back(el);
      }
    }

    inline slice(std::vector<T> &v) { *this = v; }



    inline slice &operator=(slice<T> &v) {
      clear();
      for (auto &e : v) {
        push_back(e);
      }
      return *this;
    }


    inline slice &operator=(std::vector<T> &v) {
      clear();
      for (auto &e : v) {
        push_back(e);
      }
      return *this;
    }


    inline slice<T> cut(int s, int e) {
      int newlen = e - s;
      slice<T> n(*this);
      n.m_data += s;
      n.len = n.cap = newlen;
      return n;
    }


    // access specified element with bounds checking
    inline T &at(int i) {
      if (!(i < size())) {
        throw std::out_of_range("out of range access into slice");
      }
      return m_data[i];
    }

    // access specified element w/o bounds checking
    inline T &operator[](int i) const { return m_data[i]; }


    // direct access to the underlying array
    const T *data(void) const { return m_data; }

    inline void clear(void) {
      len = 0;
      memset(m_data, 0, sizeof(T) * cap);
    }

    inline bool empty(void) const { return len == 0; }


    // returns the number of elements
    inline int size(void) const { return len; }


    // returns the number of free spaces
    inline int capacity(void) const { return cap; }

    // returns a reference to the first element
    inline T &front(void) const { return m_data[0]; }
    // returns a reference to the last element
    inline T &back(void) const { return m_data[len - 1]; }

    inline void reserve(int new_cap) {
      // do nothing if new cap is lte to old cap
      if (new_cap <= cap) return;

      // otherwise, we want to allocate the resources with
      // the garbage collector.
      T *buf = reinterpret_cast<T *>(gc::alloc(sizeof(T) * new_cap));
      // and copy all elements over from the old array.
      for (int i = 0; i < len; i++) {
        buf[i] = m_data[i];
      }
      cap = new_cap;
      // and set the buffer
      m_data = buf;
    }

    inline void push_back(const T &value) {
      if (len == cap) {
        reserve(cap * 2);
      }
      m_data[len++] = value;
    }

    inline void pop_back(void) {
      if (len > 0) len--;
    }

    /**
     * swap simply swaps around the data stored in the struct
     */
    inline void swap(slice<T> &other) {
      std::swap(len, other.len);
      std::swap(cap, other.cap);
      std::swap(m_data, other.m_data);
    }

    inline operator std::vector<T>(void) {
      std::vector<T> v;
      for (auto &el : *this) v.push_back(el);
      return v;
    }
  };

  template <typename T>
  std::ostream &operator<<(std::ostream &os, slice<T> &sl) {
    os << "{";

    for (int i = 0; i < sl.size(); i++) {
      os << sl[i];
      if (i != sl.size() - 1) os << ", ";
    }
    os << "}";
    return os;
  }



  template <typename T>
  bool operator==(const slice<T> &lhs, const slice<T> &rhs) {
    if (lhs.size() != rhs.size()) return false;

    for (int i = 0; i < lhs.size(); i++) {
      if (lhs[i] != rhs[i]) return false;
    }
    return true;
  }

  template <typename T>
  bool operator!=(const slice<T> &lhs, const slice<T> &rhs) {
    return !(lhs == rhs);
  }

  template <class T>
  inline bool operator<(const slice<T> &x, const slice<T> &y) {
    return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
  }

  template <class T>
  inline bool operator>(const slice<T> &x, const slice<T> &y) {
    return y < x;
  }

  template <class T>
  inline bool operator>=(const slice<T> &x, const slice<T> &y) {
    return !(x < y);
  }

  template <class T>
  inline bool operator<=(const slice<T> &x, const slice<T> &y) {
    return !(y < x);
  }

}  // namespace helion



namespace std {
  template <typename T>
  inline void swap(helion::slice<T> &l, helion::slice<T> &r) {
    l.swap(r);
  }




  template <typename T>
  struct std::hash<helion::slice<T>> {
    std::size_t operator()(const helion::slice<T> &k) const {
      std::hash<T> hf;
      size_t x = 0;
      size_t y = 0;
      size_t mult = 1000003UL;  // prime multiplier

      x = 0x345678UL;

      for (auto &it : k) {
        y = hf(it);
        x = (x ^ y) * mult;
        mult += (size_t)(852520UL + 2);
      }
      return x;
    }
  };

}  // namespace std
#endif
