#ifndef GC_INIT_H_HEADER
#define GC_INIT_H_HEADER

#include <stdlib.h>
#include "gc_config.h"

struct GC_region
{
  // Note: gc_collect assumes tospace and fromspace are 32-bit aligned.
  __capability void * tospace, * fromspace, * free, ** scan;
  int num_collections, num_allocations; // debugging/stats
#ifdef GC_GENERATIONAL
  struct GC_region * older_region; // only used if this one is young
#endif // GC_GENERATIONAL
#ifdef GC_GROW_HEAP
  size_t max_size;
#endif // GC_GROW_HEAP
};

struct GC_state_struct
{
  int initialized;
  struct GC_region thread_local_region;
  void * stack_bottom, * static_bottom, * static_top;
#ifdef GC_GENERATIONAL
  struct GC_region old_generation;
#ifdef GC_OY_RUNTIME
  int oy_technique;
#endif // GC_OY_RUNTIME
#endif // GC_GENERATIONAL
};

extern struct GC_state_struct GC_state;

// For convenience. Contains the data segment area used internally for GC state.
extern __capability struct GC_state_struct * GC_state_cap;

#ifdef GC_GENERATIONAL
// GC_state.oy_technique determines how we deal with old-to-young pointers.

// GC_OY_MANUAL:
// Programmer manually puts in explicit checks before every capability store by
// using GC_STORE_CAP. Internally, we use a custom permission bit to determine
// whether the capability to store into is old or not.
#define GC_OY_MANUAL       0

// GC_OY_EPHEMERAL:
// We set the `store ephemeral' permission on young objects, but not on old
// objects. We make young objects themselves ephemeral. Then storing a young
// pointer in a young object is fine, as is storing an old pointer in an old
// object. Storing an old pointer in a young object is also okay, because an
// ephemeral capability can store normal capabilities. However, we get a trap
// when we try to store a young pointer in an old object.
#define GC_OY_EPHEMERAL    1

// GC_OY_MMAP:
// We map the entire old heap without the `store capability' permission and get
// a trap whenever any capability is stored in the old heap.
#define GC_OY_MMAP         2

// GC_OY_NOSTORE:
// We remove `store capability' individually on old objects, and get a trap
// whenever any capability is stored in an old object.
#define GC_OY_NOSTORE      3

#ifdef GC_OY_RUNTIME
#define GC_CHOOSE_OY(manual_statement,ephemeral_statement) \
  do { \
    if (GC_state.oy_technique == GC_OY_MANUAL) \
      {manual_statement;} \
    else if (GC_state.oy_technique == GC_OY_EPHEMERAL) \
      {ephemeral_statement;} \
  } while (0)

#elif GC_OY_DEFAULT == GC_OY_MANUAL \
#define GC_CHOOSE_OY(manual_statement,ephemeral_statement) \
  do {manual_statement;} while (0)

#elif GC_OY_DEFAULT == GC_OY_EPHEMERAL \
#define GC_CHOOSE_OY(manual_statement,ephemeral_statement) \
  do {ephemeral_statement;} while (0)

#endif // GC_OY_RUNTIME stuff  
  
#endif // GC_GENERATIONAL

// also defined in gc.h
// Return values:
// 0 : success
// 1 : error
#define GC_init()   GC_init2(__FILE__, __LINE__)

int
GC_init2 (const char * file, int line);

// Return values:
// 0 : not initialized
// 1 : initialized
int
GC_is_initialized (void);

// Return values:
// 0 : not young
// 1 : young
#ifdef GC_GENERATIONAL
int
GC_is_young (struct GC_region * region);

// Return values:
// 0 : success
// 1 : error
int
GC_init_old_region (struct GC_region * region,
                    size_t semispace_size,
                    size_t max_size);

// Return values:
// 0 : success
// 1 : error
int
GC_init_young_region (struct GC_region * region,
                      struct GC_region * older_region,
                      size_t sz,
                      size_t max_size);

// also declared in gc.h
// Return values:
// 0 : success
// 1 : error
#ifdef GC_OY_RUNTIME
int
GC_set_oy_technique (int oy_technique);
#else // GC_OY_RUNTIME
#define GC_set_oy_technique(oy_technique) 0
#endif // GC_OY_RUNTIME

#else // GC_GENERATIONAL
// Return values:
// 0 : success
// 1 : error
int
GC_init_region (struct GC_region * region,
                size_t semispace_size,
                size_t max_size);
#endif // GC_GENERATIONAL

#endif // GC_INIT_H_HEADER
