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
#include <helion/gcconfig.h>
#include <pthread.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#include <algorithm>
#include <mutex>

#include <helion/gc.h>


#define GC_THREADS
#include <gc/gc.h>


#define allocate _alloc
#define deallocate _free

static void* _alloc(size_t size) {
  void* p = malloc(size);
  return p;
}

static void _free(void* ptr) {
  if (ptr != nullptr) {
    free(ptr);
  }
}




#ifdef __GLIBC__
#define _NOEXCEPT _GLIBCXX_USE_NOEXCEPT
#endif

void* operator new(size_t size) { return allocate(size); }
void* operator new[](size_t size) { return allocate(size); }

void operator delete(void* ptr)_NOEXCEPT { deallocate(ptr); }
void operator delete[](void* ptr) _NOEXCEPT { deallocate(ptr); }
void operator delete(void* ptr, std::size_t s)_NOEXCEPT { deallocate(ptr); }
void operator delete[](void* ptr, std::size_t s) _NOEXCEPT { deallocate(ptr); }



void *helion::gc::alloc(int n) {
  return GC_MALLOC(n);
}

void helion::gc::free(void *p) {
  GC_FREE(p);
}
