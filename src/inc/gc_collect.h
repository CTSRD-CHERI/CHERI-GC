#ifndef GC_COLLECT_H_HEADER
#define GC_COLLECT_H_HEADER

#include "gc_init.h"
#include "gc_low.h"
#include "gc_config.h"

void
GC_collect (void);

void
GC_collect_region (struct GC_region * region);

// Using capability registers, the stack and the data segment as roots, this
// copies objects from region->fromspace to region->tospace. Assumes
// region->scan and region->free are set accordingly, and modifies them.
void
GC_copy_region (struct GC_region * region,
                int is_generational);

GC_cap_ptr
GC_copy_object (struct GC_region * region,
                GC_cap_ptr cap);

void
GC_copy_roots (struct GC_region * region,
               void * root_start,
               void * root_end,
               int is_generational);

void
GC_copy_children (struct GC_region * region,
                  int is_generational);

#ifdef GC_GENERATIONAL
// Promotes all objects from region->tospace to region->older_region->tospace.
// We conservatively estimate whether the older generation has enough space to
// store all the objects from the young generation, and we collect the older
// generation if there is not enough space.
void
GC_gen_promote (struct GC_region * region);
#endif // GC_GENERATIONAL

// Replaces all capabilities in region->tospace, on the stack, in global areas
// and in registers that have a base B in the interval
// [old_base, old_base+old_size] with a new capability whose base is
// B-old_base+region->tospace.
// TODO: make it deal with roots when we do old/young stuff (either that, or
// never grow a young region)
void
GC_region_rebase (struct GC_region * region, void * old_base, size_t old_size);

#endif // GC_COLLECT_H_HEADER
