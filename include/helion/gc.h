// [License]
// MIT - See LICENSE.md file in the package.

#pragma once

#ifndef __HELION_GC__
#define __HELION_GC__

#include <mutex>

namespace helion {
  namespace gc {



    class state {};



    using blk_t = size_t;

    /**
     * a heap_segment in the GC represents a region of mapped memory
     * (maybe on the heap, maybe allocated with mmap), that acts
     * as a stand-alone heap.
     *
     * When a thread wants to mark, allocate or sweep it and locks
     * the heap_segment for ownership, then does the work it needs. They
     * are not meant to be allocated with `new`, as they need to be
     * allocated at the base of a memory mapped region of memory.
     * The static function `alloc` allows you to allocate a heap_segment
     */
    struct heap_segment {
      /**
       * free_headers are stored immediately after the header
       * block
       */
      struct free_header {
        free_header *next = nullptr;
        free_header *prev = nullptr;
      };

      // the size of the block's mapped region
      size_t size;
      // a block has a certain number of pages in it.
      // defined in `src/helion/gc.cpp`
      static int page_count;
      free_header free_entry;
      blk_t *first_block;

      heap_segment *left = nullptr;
      heap_segment *right = nullptr;

      static heap_segment *alloc(size_t size = (4096 * page_count));

      void *malloc(size_t);
      using header = size_t;

      void dump(void);


      blk_t *split(blk_t *, size_t);



      // finds the block for a given pointer. Returns nullptr if the
      // pointer is not in the heap.
      blk_t *find_block(void *);


      /**
       * returns the first usable byte in the heap
       */
      void *mem_heap_lo();
      /**
       * return the byte immediately after the last byte in the heap.
       * being greater than this means you are above this heap.
       */
      void *mem_heap_hi();
      free_header *find_fit(size_t);
    };



    size_t *split(size_t *, size_t);

    class alloc_error : public std::exception {};

    void set_stack_root(void *);
    void *malloc(size_t);
    void free(void *);
    void collect(void);
  }  // namespace gc
}  // namespace helion


#endif
