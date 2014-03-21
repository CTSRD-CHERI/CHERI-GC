#ifndef GC_COLLECT_H_HEADER
#define GC_COLLECT_H_HEADER

#include "gc_init.h"
#include "gc_low.h"

void
GC_collect (void);

void
GC_collect_region (struct GC_region * region);

// Using capability registers, the stack and the data segment as roots, this
// copies objects from region->fromspace to region->tospace. Assumes
// region->scan and region->free are set accordingly, and modifies them.
void
GC_copy_region (struct GC_region * region);

GC_cap_ptr
GC_copy_object (struct GC_region * region,
                GC_cap_ptr cap);

void
GC_copy_roots (struct GC_region * region,
               void * root_start,
               void * root_end);

void
GC_copy_children (struct GC_region * region);

// Promotes all objects from region->tospace to region->older_region->tospace.
// We conservatively estimate whether the older generation has enough space to
// store all the objects from the young generation, and we collect the older
// generation if there is not enough space.
void
GC_gen_promote (struct GC_region * region);

#endif // GC_COLLECT_H_HEADER
