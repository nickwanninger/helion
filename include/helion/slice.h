// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __HELION_SLICE_JIT__
#define __HELION_SLICE_JIT__

#include <helion/gc.h>
#include <initializer_list>
#include <iterator>
#include <iostream>
#include <stdexcept>


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
    int len = 0;
    int cap = 0;
    T *m_data = nullptr;

   public:
    class iterator {
     public:
      typedef iterator self_type;
      typedef T value_type;
      typedef T &reference;
      typedef T *pointer;
      typedef std::forward_iterator_tag iterator_category;
      typedef int difference_type;
      iterator(pointer ptr) : ptr_(ptr) {}
      self_type operator++() {
        self_type i = *this;
        ptr_++;
        return i;
      }
      self_type operator++(int junk) {
        ptr_++;
        return *this;
      }
      reference operator*() { return *ptr_; }
      pointer operator->() { return ptr_; }
      bool operator==(const self_type &rhs) { return ptr_ == rhs.ptr_; }
      bool operator!=(const self_type &rhs) { return ptr_ != rhs.ptr_; }

     private:
      pointer ptr_;
    };

    class const_iterator {
     public:
      typedef const_iterator self_type;
      typedef T value_type;
      typedef T &reference;
      typedef T *pointer;
      typedef int difference_type;
      typedef std::forward_iterator_tag iterator_category;
      const_iterator(pointer ptr) : ptr_(ptr) {}
      self_type operator++() {
        self_type i = *this;
        ptr_++;
        return i;
      }
      self_type operator++(int junk) {
        ptr_++;
        return *this;
      }
      reference operator*() { return *ptr_; }
      const pointer operator->() { return ptr_; }
      bool operator==(const self_type &rhs) { return ptr_ == rhs.ptr_; }
      bool operator!=(const self_type &rhs) { return ptr_ != rhs.ptr_; }

     private:
      pointer ptr_;
    };


    iterator begin() { return iterator(m_data); }

    iterator end() { return iterator(m_data + len); }

    const_iterator begin() const { return const_iterator(m_data); }

    const_iterator end() const { return const_iterator(m_data + len); }

    inline slice() { reserve(1); }
    inline explicit slice(int count) {
      reserve(count);
      len = cap;
    }

    inline slice(std::initializer_list<T> init) {
      reserve(init.size());
      for (auto &el : init) {
        push_back(el);
      }
    }


    // access specified element with bounds checking
    inline T &at(int i) {
      if (!(i < size())) {
        throw std::out_of_range("out of range access into slice");
      }
      return m_data[i];
    }

    // access specified element w/o bounds checking
    inline T &operator[](int i) { return m_data[i]; }

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
}  // namespace helion

#endif
