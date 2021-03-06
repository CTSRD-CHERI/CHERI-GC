#ifndef GC_COLLECT_H_HEADER
#define GC_COLLECT_H_HEADER

#include "gc_common.h"
#include "gc_init.h"
#include "gc_low.h"
#include "gc_config.h"

// also declared in gc.h
GC_FUNC void
GC_collect (void);

// also declared in gc.h
// In non-generational mode, this has the same effect as GC_collect()
GC_FUNC void
GC_major_collect (void);

// also declared in gc.h
// Free the entire young generation. Does not collect.
GC_FUNC void
GC_minor_free (void);

// force_major_collection is ignored if GC_GENERATIONAL is not set. Otherwise,
// if it is set, a major collection is triggered immediately after the minor
// collection. With more than two generations, the force_major_collection value
// propagates through to older generations.
GC_FUNC void
GC_collect2 (int force_major_collection);

GC_FUNC void
GC_collect_region (struct GC_region * region,
                   int force_major_collection);

// Using capability registers, the stack and the data segment as roots, this
// copies objects from region->fromspace to region->tospace. Assumes
// region->scan and region->free are set accordingly, and modifies them.
GC_FUNC void
GC_copy_region (struct GC_region * region,
                int is_generational);

GC_FUNC GC_cap_ptr
GC_copy_object (struct GC_region * region,
                GC_cap_ptr cap,
                void * parent); // parent for debugging only

GC_FUNC void
GC_copy_roots (struct GC_region * region,
               void * root_start,
               void * root_end,
               int is_generational,
               int is_data_segment);

GC_FUNC void
GC_copy_child (struct GC_region * region,
               GC_cap_ptr * child_addr,
               int is_generational);

GC_FUNC void
GC_copy_children (struct GC_region * region,
                  int is_generational);
                  
#ifdef GC_GENERATIONAL
// Copies the objects from the remembered set and then clears the remembered
// set.
GC_FUNC void
GC_copy_remembered_set (struct GC_region * region);

// Promotes all objects from region->tospace to region->older_region->tospace.
// We conservatively estimate whether the older generation has enough space to
// store all the objects from the young generation, and we collect the older
// generation if there is not enough space.
// If force_major_collection is set, a major collection is triggered immediately
// after the minor collection. With more than two generations, the
// force_major_collection value propagates through to older generations.
GC_FUNC void
GC_gen_promote (struct GC_region * region, int force_major_collection);
#endif // GC_GENERATIONAL

// Replaces all capabilities in region->tospace, on the stack, in global areas
// and in registers that have a base B in the interval
// [old_base, old_base+old_size] with a new capability whose base is
// B-old_base+region->tospace.
// TODO: make it deal with roots when we do old/young stuff (either that, or
// never grow a young region)
GC_FUNC void
GC_region_rebase (struct GC_region * region, void * old_base, size_t old_size);

// Replaces all capabilities in the interval [start, end] that have a base B
// in the interval [old_base, old_base+old_size] with a new capability whose
// base is B-old_base+new_base. NOTE: the region argument is only used for the
// bitmap, where appropriate.
GC_FUNC void
GC_rebase (struct GC_region * region,
           void * start,
           void * end,
           void * old_base,
           size_t old_size,
           void * new_base);
           
// Removes forwarding addresses in the given region
/*GC_FUNC void
GC_clean_forwarding (void * start,
                     void * end);*/

#endif // GC_COLLECT_H_HEADER
