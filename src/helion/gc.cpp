#include <stdio.h>

// Include file for GC-aware allocators.
// alloc allocates uncollected objects that are scanned by the collector
// for pointers to collectable objects.  Gc_alloc allocates objects that
// are both collectable and traced.  Single_client_alloc and
// single_client_gc_alloc do the same, but are not thread-safe even
// if the collector is compiled to be thread-safe.  They also try to
// do more of the allocation in-line.
#include <fcntl.h>
#include <gc/gc_allocator.h>
#include <helion/gc.h>
#include <helion/gcconfig.h>
#include <pthread.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#include <algorithm>
#include <mutex>


#define GC_DEBUG

// #define allocate helion::gc::malloc
// #define deallocate helion::gc::free



#define allocate ::malloc
#define deallocate ::free

#define ROUND_UP(N, S) ((((N) + (S)-1) / (S)) * (S))

void* operator new(size_t size) { return allocate(size); }

#ifdef __GLIBC__
#define _NOEXCEPT _GLIBCXX_USE_NOEXCEPT
#endif

void operator delete(void* ptr)_NOEXCEPT { deallocate(ptr); }

void operator delete(void* ptr, std::size_t s)_NOEXCEPT { deallocate(ptr); }


#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define PAGE_SIZE_ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define HEADER_SIZE (ALIGN(sizeof(gc::blk_t)))
#define OVERHEAD (ALIGN(sizeof(gc::heap_segment::free_header)))
#define GET_FREE_HEADER(blk) \
  ((gc::heap_segment::free_header*)((char*)blk + HEADER_SIZE))
#define ADJ_SIZE(given) ((given < OVERHEAD) ? OVERHEAD : ALIGN(given))
#define GET_BLK(header) ((blk_t*)((char*)(header)-HEADER_SIZE))
#define IS_FREE(blk) (*(blk)&1UL)
#define GET_SIZE(blk) (*(blk) & ~1UL)
#define SET_FREE(blk) (*(blk) |= 1UL)
#define SET_USED(blk) (*(blk) &= ~1UL)
#define SET_SIZE(blk, newsize) (*(blk) = ((newsize) & ~1UL) | IS_FREE(blk))
#define NEXT_BLK(blk) (blk_t*)((char*)(blk) + GET_SIZE(blk))


/**
 * an attempt to make my own GC:
 */
using namespace helion;
using namespace helion::gc;




static std::mutex heaps_lock;
static gc::heap_segment* heaps = nullptr;
/**
 * add a heap to the heap tree for efficient lookup
 * of a pointer within the set of all GC owned heaps
 */
static void add_heap(gc::heap_segment* hs) {
  // unfortunately, adding a heap requires a global lock
  // TODO possibly do some atomic stuff?
  std::unique_lock lock(heaps_lock);
  // initial setup case
  if (heaps == nullptr) {
    heaps = hs;
    return;
  }

  heaps->store_heap_in_tree(hs);
}



static gc::heap_segment* find_heap(void* ptr) {
  auto* hs = heaps;

  while (true) {
    if (hs == 0) return nullptr;
    int cmp = hs->check_pointer(ptr);
    if (cmp < 0) hs = hs->left;
    if (cmp > 0) hs = hs->right;
    if (cmp == 0) break;
  }
  return hs;
}

void gc::heap_segment::store_heap_in_tree(heap_segment* hs) {
  // won't happen, but just to be safe...
  if (hs == this) return;
  int cmp = (char*)hs - (char*)this;
  if (cmp < 1) {
    if (left == nullptr) {
      left = hs;
    } else {
      left->store_heap_in_tree(hs);
    }
  } else {
    if (right == nullptr) {
      right = hs;
    } else {
      right->store_heap_in_tree(hs);
    }
  }
}


blk_t* gc::heap_segment::get_next_free(blk_t* curr) {
  curr = NEXT_BLK(curr);
  while (1) {
    if ((void*)curr >= mem_heap_hi()) return NULL;
    if (IS_FREE(curr)) return curr;
    curr = NEXT_BLK(curr);
  }
}

static inline constexpr bool adjacent(blk_t* a, blk_t* b) {
  return (NEXT_BLK(a) == b);
}


static inline blk_t* attempt_free_union(blk_t* this_blk) {
  using free_header = gc::heap_segment::free_header;
  free_header* fh = GET_FREE_HEADER(this_blk);
  free_header* p = fh->prev;
  free_header* n = fh->next;
  blk_t* prev_blk = GET_BLK(p);
  blk_t* next_blk = GET_BLK(n);


  if (adjacent(this_blk, next_blk)) {
    size_t sz = GET_SIZE(this_blk);
    fh->next = n->next;
    n->next->prev = fh;
    sz += GET_SIZE(next_blk);
    SET_SIZE(this_blk, sz);
#ifdef GC_DEBUG
    // printf("RIGHT UNION!\n");
#endif
  }

  while (adjacent(prev_blk, this_blk)) {
    size_t sz = GET_SIZE(prev_blk);
    sz += GET_SIZE(this_blk);
    SET_SIZE(prev_blk, sz);
    free_header* old_pprev = p->prev;
    p->next = fh->next;
    fh->next->prev = p;

    this_blk = prev_blk;
    fh = p;

    p = old_pprev;
    prev_blk = GET_BLK(p);
#ifdef GC_DEBUG
    // printf("LEFT UNION!\n");
#endif
  }



  return this_blk;
}
/**
 * free_block frees a block... of course, but it also unions
 * adjacent free blocks into a single, larger free block. It also updates
 * the free list
 */
void gc::heap_segment::free_block(blk_t* blk) {
  // update the state on the newly freed block.
  free_header* hdr = GET_FREE_HEADER(blk);

  hdr->next = nullptr;
  hdr->prev = nullptr;

  // now we have to f24d the next free block so we can put ourselved
  // in the list of freed nodes (and so we can union with adjacent nodes)
  blk_t* next_free = get_next_free(blk);
  // if we were at the end of the heap...
  if (next_free == NULL) {
    hdr->next = &free_entry;
    hdr->prev = hdr->next->prev;
    hdr->next->prev = hdr;
    hdr->prev->next = hdr;
    blk = attempt_free_union(blk);
    SET_FREE(blk);
  } else {
    free_header* succ = GET_FREE_HEADER(next_free);
    hdr->next = succ;
    hdr->prev = succ->prev;
    hdr->prev->next = hdr;
    succ->prev = hdr;

    blk = attempt_free_union(blk);
    SET_FREE(blk);
  }
}


// define the number of pages in a block
// TODO(optim) decide on how many pages a block should have
//             in it.
int gc::heap_segment::page_count = 1;

/**
 * allocate a new block with a minimum number of bytes in the heap. It will
 * always allocate 1 extra page for administration overhead.
 */
heap_segment* gc::heap_segment::alloc(size_t size) {
  printf("allocate new heap\n");
  heap_segment* b = nullptr;
  size = PAGE_SIZE_ALIGN(size);
  // #define MAP_HEAP_INTO_FILE

#ifndef MAP_HEAP_INTO_FILE
  void* mapped_region = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
#else
  int fd = open("heap", O_RDWR | O_CREAT, (mode_t)0600);
  lseek(fd, size, SEEK_SET);
  write(fd, "", 1);
  void* mapped_region =
      mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
#endif


  b = (heap_segment*)mapped_region;
  // initialize the heap segment
  new (b) heap_segment();
  b->size = size;


  size_t real_size = size - ALIGN(sizeof(heap_segment));
  // the first thing that needs to be setup in a block segment is
  // the initial header.
  blk_t* hdr = (blk_t*)(b + 1);
  b->first_block = hdr;
  SET_FREE(hdr);
  SET_SIZE(hdr, real_size);

  free_header* fh = GET_FREE_HEADER(hdr);
  fh->next = &b->free_entry;
  fh->prev = &b->free_entry;
  b->free_entry.next = fh;
  b->free_entry.prev = fh;
  add_heap(b);
  return b;
}


blk_t* gc::heap_segment::find_block(void* ptr) {
  blk_t* top = (blk_t*)((char*)this + size);
  blk_t* c = first_block;
  for (; c < top; c = NEXT_BLK(c)) {
    size_t sz = GET_SIZE(c);
    char* data_base = (char*)c + HEADER_SIZE;

    if (ptr == data_base || (ptr >= data_base && ((char*)c + sz) > ptr))
      return c;
  }
  return nullptr;
}

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"

void gc::heap_segment::dump(void) {
  blk_t* top = (blk_t*)((char*)this + size);
  blk_t* c = first_block;
  printf("   %p: [", this);
  if (full) {
    printf("1");
  } else {
    printf("0");
  }
  printf("] ");
  for (; c < top; c = NEXT_BLK(c)) {
    auto color = IS_FREE(c) ? ANSI_COLOR_GREEN : ANSI_COLOR_RED;
    printf("%s%li%s ", color, GET_SIZE(c), ANSI_COLOR_RESET);
    printf(ANSI_COLOR_RESET);
  }
  printf("\n");

  if (left != nullptr) left->dump();
  if (right != nullptr) right->dump();
}



void* gc::heap_segment::malloc(size_t size) {
  heaps->dump();
  size = ADJ_SIZE(size);

  free_header* fit = find_fit(size);
  if (fit == nullptr) {
    return nullptr;
  }


  // at this point, we need to split the block, and insert the
  blk_t* blk = GET_BLK(fit);
  blk_t* split_block = split(blk, size);

  // if the split block is the same block there wasn't a split
  // because I place the new block at the end of the old big block
  if (split_block == blk) {
    printf("OPES: %p %p\n", fit->prev, fit->next);
    fit->prev->next = fit->next;
    fit->next->prev = fit->prev;
  }
  blk = split_block;

  SET_USED(blk);
#ifdef GC_DEBUG
  printf("ALLOC %zu\n", size);
  heaps->dump();
#endif
  return (char*)blk + HEADER_SIZE;
}


/**
 * given a block, split it into two blocks. If the change in block size
 * would result in an invalid free_header on one, simply return the original
 * block
 */
blk_t* gc::heap_segment::split(blk_t* blk, size_t size) {
  size_t current_size = GET_SIZE(blk);
  size_t target_block_size = size + HEADER_SIZE;

  size_t final_size = current_size - target_block_size;

  if (final_size < (size_t)OVERHEAD) return blk;

  blk_t* split_block = (blk_t*)((char*)blk + final_size);
  // only change the block's size if it is a different memory address
  if (blk != split_block) {
    SET_SIZE(blk, final_size);
    SET_SIZE(split_block, target_block_size);
  }
  return split_block;
}



gc::heap_segment::free_header* gc::heap_segment::find_fit(size_t size) {
  free_header* h = &free_entry;
  h = h->next;

  while (h != &free_entry && h != nullptr) {
    blk_t* b = GET_BLK(h);
    if (GET_SIZE(b) >= size) {
      return h;
    }
    h = h->next;
  }
  return nullptr;
}



void* gc::heap_segment::mem_heap_lo(void) { return first_block; }
void* gc::heap_segment::mem_heap_hi(void) {
  return (void*)((char*)this + size);
}


static gc::heap_segment* walk_heaps_for_usable(size_t sz, gc::heap_segment *h) {
  // base case, heap not found
  if (h == nullptr) return nullptr;


  std::unique_lock lock(h->lock);


  auto fit = h->find_fit(sz);

  if (fit != nullptr) {
    printf("reuse\n");
    h->full = false;
    return h;
  }

  auto l = walk_heaps_for_usable(sz, h->left);
  if (l != nullptr) return l; 

  auto r = walk_heaps_for_usable(sz, h->right);
  if (r != nullptr) return r; 


  return nullptr;
}

static gc::heap_segment* walk_heaps_for_usable(size_t sz) {
  auto *hp = walk_heaps_for_usable(sz, heaps);
  return hp;
}



// TODO(threads) have seperate regions and whatnot
thread_local void** stack_root;
static thread_local heap_segment* heap;

void gc::set_stack_root(void* sb) {
  //
  stack_root = (void**)sb;
}

void* gc::malloc(size_t s) {
top:
  /*
   * allocation requires a heap on the current thread.
   */
  if (heap == nullptr) {
    size_t hs = std::max(s, (size_t)(gc::heap_segment::page_count+1) * 4096);
    heap = gc::heap_segment::alloc(hs);
  }
  /*
   * attempt to allocate the memory in the current heap segment.
   */
  heap->lock.lock();
  void* p = heap->malloc(s);
  heap->lock.unlock();
  if (p == nullptr) {
    // if the pointer was null, we can assume that there was no
    // space to allocate that memory in the current heap. So it
    // is up to the current allocation call to move the current
    // heap to the heap tree. Then we either allocate a new heap
    // in this function and re-attempt the allocation
    heap->lock.lock();
    heap->full = true;
    heap->lock.unlock();


    heaps_lock.lock();
    auto found = walk_heaps_for_usable(s);
    heaps_lock.unlock();

    size_t hs = std::max(s, (size_t)(gc::heap_segment::page_count+1) * 4096);

    if (found == nullptr) {
      printf("%zu\n", hs);
      heap = gc::heap_segment::alloc(hs);
    } else {
      heap = found;
    }
    heaps->dump();
    goto top;
  }


  return p;
}




void gc::free(void* ptr) {
#ifdef GC_DEBUG
  // printf("<<<<< FREEING %p >>>>>\n", ptr);
#endif
  heap_segment* hp = find_heap(ptr);
  blk_t* block;

  if (hp == nullptr) {
    goto invalid_pointer_error;
  }

  hp->lock.lock();

  // now we have a valid heap, we need to find the block within it...
  block = hp->find_block(ptr);
  if (block == nullptr) {
    goto invalid_pointer_error;
  }

  hp->free_block(block);
  hp->full = false;

#ifdef GC_DEBUG
  // and after the free...
  printf("FREE\n");
  heaps->dump();
#endif
  hp->lock.unlock();
  return;  // return before throwing that error :)
invalid_pointer_error:
  hp->lock.unlock();
  throw std::logic_error("Invalid pointer (non-block) passed to gc::free");
}

void gc::collect(void) {}




#if defined(SEARCH_FOR_DATA_START)
/* The I386 case can be handled without a search.  The Alpha case     */
/* used to be handled differently as well, but the rules changed      */
/* for recent Linux versions.  This seems to be the easiest way to    */
/* cover all versions.                                                */

#if defined(LINUX)
/* Some Linux distributions arrange to define __data_start.  Some   */
/* define data_start as a weak symbol.  The latter is technically   */
/* broken, since the user program may define data_start, in which   */
/* case we lose.  Nonetheless, we try both, preferring __data_start.*/
/* We assume gcc-compatible pragmas.                                */
EXTERN_C_BEGIN
#pragma weak __data_start
#pragma weak data_start
extern int __data_start[], data_start[];
EXTERN_C_END
#endif /* LINUX */
#define COVERT_DATAFLOW(w) ((GC_word)(w))

ptr_t GC_data_start = NULL;

void GC_init_linux_data_start(void) {
  ptr_t data_end = DATAEND;

#if (defined(LINUX)) && !defined(IGNORE_PROG_DATA_START)
  /* Try the easy approaches first: */
  if (COVERT_DATAFLOW(__data_start) != 0) {
    GC_data_start = (ptr_t)(__data_start);
  } else {
    GC_data_start = (ptr_t)(data_start);
  }
  if (COVERT_DATAFLOW(GC_data_start) != 0) {
    if ((size_t)GC_data_start > (size_t)data_end) {
      printf("Wrong __data_start/_end pair: %p .. %p", (void*)GC_data_start,
             (void*)data_end);
      exit(-1);
    }

    return;
  }
#ifdef DEBUG_ADD_DEL_ROOTS
  GC_log_printf("__data_start not provided\n");
#endif
#endif /* LINUX */

  /*
  if (GC_no_dls) {
    GC_data_start = data_end;
    return;
  }
  */

  // GC_data_start = (ptr_t)GC_find_limit(data_end, false);
#endif /* SEARCH_FOR_DATA_START */
