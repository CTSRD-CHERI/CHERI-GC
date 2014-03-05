#ifndef GC_COLLECT_H_HEADER
#define GC_COLLECT_H_HEADER

#include "gc_init.h"

void
GC_collect (void);

void
GC_collect_region (struct GC_region * region);

void
GC_collect_range (struct GC_region * region, void * start, void * end);

// Return values:
// 0 : success
// 1 : error
int
GC_get_roots (struct GC_region * region);

#endif // GC_COLLECT_H_HEADER
