#ifndef GC_COLLECT_H_HEADER
#define GC_COLLECT_H_HEADER

#include "gc_init.h"
#include "gc_low.h"

void
GC_collect (void);

void
GC_collect_region (struct GC_region * region);

GC_cap_ptr
GC_copy_object (struct GC_region * region,
                GC_cap_ptr cap);

void
GC_copy_roots (struct GC_region * region,
               void * root_start,
               void * root_end);

void
GC_copy_children (struct GC_region * region);

#endif // GC_COLLECT_H_HEADER
