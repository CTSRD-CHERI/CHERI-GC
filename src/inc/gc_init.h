#ifndef GC_INIT_H_HEADER
#define GC_INIT_H_HEADER

#include "gc_common.h"
#include "gc_config.h"
#include "gc_time.h"
#include "gc_remset.h"
#include "gc_bitmap.h"

#include <stdlib.h>

struct GC_region
{
  // The allocation routines (GC_grow, GC_init_*_region) ensure that the tospace
  // and fromspace are 32-bit aligned and have a 32-bit aligned length. The
  // collection routines assume and require this.
  __capability void * tospace, * fromspace, * free, ** scan;
  // The not-32-byte-aligned result from GC_low_malloc(), used by GC_grow().
  void * tospace_misaligned, * fromspace_misaligned;
  int num_collections, num_allocations; // debugging/stats

#ifdef GC_USE_BITMAP
  struct GC_bitmap * tospace_bitmap, * fromspace_bitmap;
#endif // GC_USE_BITMAP
  
#ifdef GC_GENERATIONAL
  struct GC_region * older_region; // only used if this one is young

#if (GC_OY_STORE_DEFAULT == GC_OY_STORE_REMEMBERED_SET)
  struct GC_remembered_set * remset;
#endif // GC_OY_STORE_DEFAULT
  
#endif // GC_GENERATIONAL
#ifdef GC_GROW_HEAP
  size_t max_grow_size_before_collection;
  size_t max_grow_size_after_collection;
#endif // GC_GROW_HEAP
#ifdef GC_TIME
  GC_time_t time_spent_in_collector;
#endif // GC_TIME
};

struct GC_state_struct
{
  int initialized;
  struct GC_region thread_local_region;
  
  // set once on init
  void * stack_bottom,
       * static_bottom, * static_top, // data segment area
       * state_bottom, * state_top; // location of this struct
  
  // set before GC_collect_region() is called and cleared by GC_collect_region()
  void * stack_top, * reg_bottom, * reg_top;
  
#ifdef GC_GENERATIONAL
  struct GC_region old_generation;
#ifdef GC_WB_RUNTIME
  int wb_type; // write barrier type for old-young stores
#endif // GC_WB_RUNTIME
#endif // GC_GENERATIONAL
};

extern struct GC_state_struct GC_state;


#ifdef GC_GENERATIONAL


// GC_OY_STORE_DEFAULT determines the technique we use to store old-young
// pointers.

// GC_OY_STORE_REMEMBERED_SET:
// We remember the pointer to the young object inside the old object (not the
// actual old object itself) by making it act as a single-capability-containing
// root. On copy, we iterate the set and copy everything inside the remembered
// set, too.
#define GC_OY_STORE_REMEMBERED_SET    0

// GC_state.wb_type determines the write barrier technique we use to deal with
// old-to-young pointers.

// GC_WB_MANUAL:
// Programmer manually puts in explicit checks before every capability store by
// using GC_STORE_CAP. Internally, we use a custom permission bit to determine
// whether the capability to store into is old or not.
#define GC_WB_MANUAL       0

// GC_WB_EPHEMERAL:
// We set the `store ephemeral' permission on young objects, but not on old
// objects. We make young objects themselves ephemeral. Then storing a young
// pointer in a young object is fine, as is storing an old pointer in an old
// object. Storing an old pointer in a young object is also okay, because an
// ephemeral capability can store normal capabilities. However, we get a trap
// when we try to store a young pointer in an old object.
#define GC_WB_EPHEMERAL    1

// GC_WB_MMAP:
// We map the entire old heap without the `store capability' permission and get
// a trap whenever any capability is stored in the old heap.
#define GC_WB_MMAP         2

// GC_WB_NOSTORE:
// We remove `store capability' individually on old objects, and get a trap
// whenever any capability is stored in an old object.
#define GC_WB_NOSTORE      3

#ifdef GC_WB_RUNTIME
#define GC_SWITCH_WB_TYPE(manual_statement,ephemeral_statement) \
  do { \
    if (GC_state.wb_type == GC_WB_MANUAL) \
      {manual_statement;} \
    else if (GC_state.wb_type == GC_WB_EPHEMERAL) \
      {ephemeral_statement;} \
  } while (0)

#elif GC_WB_DEFAULT == GC_WB_MANUAL \
#define GC_SWITCH_WB_TYPE(manual_statement,ephemeral_statement) \
  do {manual_statement;} while (0)

#elif GC_WB_DEFAULT == GC_WB_EPHEMERAL \
#define GC_SWITCH_WB_TYPE(manual_statement,ephemeral_statement) \
  do {ephemeral_statement;} while (0)

#endif // GC_WB_RUNTIME etc
  
#endif // GC_GENERATIONAL

// also defined in gc.h
// GC_init(): MUST be called from main(), and main must take argc as an
// argument. Could change this to use GC_get_stack_bottom(), but this is
// more accurate.
// Return values:
// 0 : success
// 1 : error
#define GC_init()   GC_init2(&argc, __FILE__, __LINE__)

GC_FUNC int
GC_init2 (void * arg_for_stack_bottom, const char * file, int line);

// Return values:
// 0 : not initialized
// 1 : initialized
GC_FUNC int
GC_is_initialized (void);

// Return values:
// 0 : not young
// 1 : young
#ifdef GC_GENERATIONAL
GC_FUNC int
GC_is_young (struct GC_region * region);

// Return values:
// 0 : success
// 1 : error
GC_FUNC int
GC_init_old_region (struct GC_region * region,
                    size_t semispace_size,
                    size_t max_grow_size_before_collection,
                    size_t max_grow_size_after_collection);

// Return values:
// 0 : success
// 1 : error
GC_FUNC int
GC_init_young_region (struct GC_region * region,
                      struct GC_region * older_region,
                      size_t sz,
                      size_t max_grow_size_before_collection,
                      size_t max_grow_size_after_collection);

// also declared in gc.h
// Return values:
// 0 : success
// 1 : error
#ifdef GC_WB_RUNTIME
GC_FUNC int
GC_set_wb_type (int wb_type);
#else // GC_WB_RUNTIME
#define GC_set_wb_type(wb_type) 0
#endif // GC_WB_RUNTIME

#else // GC_GENERATIONAL
// Return values:
// 0 : success
// 1 : error
GC_FUNC int
GC_init_region (struct GC_region * region,
                size_t semispace_size,
                size_t max_grow_size_before_collection,
                size_t max_grow_size_after_collection);
#endif // GC_GENERATIONAL

GC_FUNC void
GC_reset_region (struct GC_region * region);

#endif // GC_INIT_H_HEADER
