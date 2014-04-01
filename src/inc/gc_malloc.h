#ifndef GC_MALLOC_H_HEADER
#define GC_MALLOC_H_HEADER

#include "gc_common.h"
#include "gc_init.h"
#include "gc_config.h"

#include <stdlib.h>

// GC_malloc:
// also declared in gc.h
// returns GC_INVALID_PTR on failure, whose void* cast is guaranteed to be equal
// to NULL
#ifdef GC_DEBUG
#define GC_malloc(sz) GC_malloc2(__FILE__, __LINE__, (sz))
#else // GC_DEBUG
#define GC_malloc GC_malloc2
#endif // GC_DEBUG

GC_FUNC __capability void *
GC_malloc2
(
#ifdef GC_DEBUG
  const char * file, int line,
#endif // GC_DEBUG
  size_t sz
);

GC_FUNC __capability void *
GC_malloc_region
(
#ifdef GC_DEBUG
  const char * file, int line,
#endif // GC_DEBUG
  struct GC_region * region, size_t sz, int collect_on_failure
#ifdef GC_USE_GC_STACK_CLEAN
  , int * collected
#endif // GC_USE_GC_STACK_CLEAN
);

#endif // GC_MALLOC_H_HEADER
