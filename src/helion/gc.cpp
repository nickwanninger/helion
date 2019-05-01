#include <stdio.h>

// Include file for GC-aware allocators.
// alloc allocates uncollected objects that are scanned by the collector
// for pointers to collectable objects.  Gc_alloc allocates objects that
// are both collectable and traced.  Single_client_alloc and
// single_client_gc_alloc do the same, but are not thread-safe even
// if the collector is compiled to be thread-safe.  They also try to
// do more of the allocation in-line.
#include <gc/gc_allocator.h>
#include <helion/gc.h>
#include <pthread.h>
#include <setjmp.h>
#include <sys/resource.h>
#include <mutex>

#define GC_THREADS
#include <gc/gc.h>

extern "C" void GC_allow_register_threads();

#ifdef USE_GC
#define allocate GC_MALLOC
#define deallocate GC_FREE

struct gc_startup {
  gc_startup() {
    GC_set_all_interior_pointers(1);
    GC_INIT();
    GC_allow_register_threads();
  }
};
static gc_startup init;

#else
#define allocate helion::gc::alloc
#define deallocate helion::gc::free
#endif

#define ROUND_UP(N, S) ((((N) + (S)-1) / (S)) * (S))

void* operator new(size_t size) { return allocate(size); }
void* operator new[](size_t size) { return allocate(size); }

#ifdef __GLIBC__
#define _NOEXCEPT _GLIBCXX_USE_NOEXCEPT
#endif

void operator delete(void* ptr)_NOEXCEPT { deallocate(ptr); }

void operator delete[](void* ptr) _NOEXCEPT { deallocate(ptr); }

void operator delete(void* ptr, std::size_t s)_NOEXCEPT { deallocate(ptr); }

void operator delete[](void* ptr, std::size_t s) _NOEXCEPT { deallocate(ptr); }




/**
 * an attempt to make my own GC:
 */
using namespace helion;
using namespace helion::gc;

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define OVERHEAD (ALIGN(sizeof(header)))

struct header {
  bool marked = false;
  int32_t size = 0;
  header* prev = nullptr;
  header* next = nullptr;

  inline bool contains(void* p) {
    char* lo = ((char*)this) + sizeof(header);
    char* hi = (lo + size);
    return p >= lo && p < hi;
  }
};


static header* objects = nullptr;

// TODO(threads) have seperate regions and whatnot
thread_local void** stack_root;

void gc::set_stack_root(void* sb) {
  //
  stack_root = (void**)sb;
}

size_t live_objects = 0;

void free_object(header* hdr) {
  live_objects--;
  if (hdr == objects) objects = hdr->next;

  if (hdr->next != nullptr) hdr->next->prev = hdr->prev;
  if (hdr->prev != nullptr) hdr->prev->next = hdr->next;
  // printf("FREE\n");
  ::free(hdr);
}



void* gc::alloc(size_t s) {
  size_t size = ALIGN(s + sizeof(header));
  void* p = malloc(size);

  header* n = (header*)p;
  n->size = size;
  // now add the header to the objects list
  n->next = objects;
  n->prev = nullptr;
  n->marked = false;
  if (objects != nullptr) {
    objects->prev = n;
  }
  objects = n;
  // TODO(do this atomically, somehow)
  live_objects++;

  void* r = (void*)(n + 1);
  return r;
}




void gc::free(void* ptr) {
  header* h = ((header*)ptr) - 1;
  free_object(h);
}




/**
 * sweep a region of memory and return how many objects were marked
 */
static size_t mark_region(void** bottom, void** top) {
  size_t marked = 0;

  for (void** _p = bottom; _p < top; _p++) {
    void* p = *_p;
    header* o = objects;
    while (o != nullptr) {
      if (o->contains(p)) {
        if (!o->marked) {
          o->marked = true;
          void* base = ((char*)o) + sizeof(header);
          void* top = ((char*)o) + o->size;
          mark_region((void**)base, (void**)top);
        }
      }
      o = o->next;
    }
  }

  return marked;
}

static size_t sweep(void) {
  header* o = objects;
  while (o != nullptr) {
    if (!o->marked) {
      header* dead = o;
      o = o->next;
      free_object(dead);
    } else {
      o = o->next;
    }
  }
  return 0;
}

static void do_collect(void) {
  size_t start_live = live_objects;
  void* sp;
  mark_region(&sp, stack_root);
  sweep();

  size_t total = 0;

  // now we need to walk through and clear all marked bits
  header* o = objects;
  while (o != nullptr) {
    o->marked = false;
    total += o->size;
    o = o->next;
  }

  printf("%zu -> %zu objs, %zu bytes\n", start_live, live_objects, total);
  return;
}

void gc::collect(void) {
#define LOG_GC_TIME
#ifdef LOG_GC_TIME
  auto start = std::chrono::steady_clock::now();
#endif
  // offload the registers to the stack
  jmp_buf b;
  setjmp(b);
  // do_collect in
  do_collect();
#ifdef LOG_GC_TIME
  auto end = std::chrono::steady_clock::now();
  auto el = std::chrono::duration_cast<std::chrono::microseconds>(end - start)
                .count();
  printf("%fms\n", el / 1000.0);
#endif
}
