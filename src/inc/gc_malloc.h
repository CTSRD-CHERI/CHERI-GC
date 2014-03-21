#ifndef GC_MALLOC_H_HEADER
#define GC_MALLOC_H_HEADER

#include "gc_init.h"

#include <stdlib.h>

// also declared in gc.h
// returns GC_INVALID_PTR on failure, whose void* cast is guaranteed to be equal
// to NULL
__capability void *
GC_malloc (size_t sz);

__capability void *
GC_malloc_region (struct GC_region * region, size_t sz, int collect_on_failure);


#endif // GC_MALLOC_H_HEADER
