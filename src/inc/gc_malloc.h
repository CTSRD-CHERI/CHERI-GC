#ifndef GC_MALLOC_H_HEADER
#define GC_MALLOC_H_HEADER

#include "gc_init.h"

#include <stdlib.h>

__capability void *
GC_malloc (size_t sz);

__capability void *
GC_malloc_region (struct GC_region * region, size_t sz);


#endif // GC_MALLOC_H_HEADER
