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


void *helion::gc::raw_alloc(int n) {
  return GC_MALLOC(n);
}

void helion::gc::raw_free(void *p) {
  GC_FREE(p);
}
