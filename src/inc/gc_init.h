#ifndef GC_INIT_H_HEADER
#define GC_INIT_H_HEADER

#include <stdlib.h>

struct GC_region
{
  __capability void * tospace, * fromspace, * free, ** scan;
  struct GC_region * older_region; // only used if this one is young
  int num_collections; // debugging/stats
};

struct GC_state_struct
{
  int initialized;
  int oy_technique;
  struct GC_region thread_local_region;
  struct GC_region old_generation;
  void * stack_bottom, * static_bottom, * static_top;
};


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

extern struct GC_state_struct GC_state;

// For convenience. Contains the data segment area used internally for GC state.
extern __capability struct GC_state_struct * GC_state_cap;

// also declared in gc.h
// Return values:
// 0 : success
// 1 : error
int
GC_init (void);

// Return values:
// 0 : not initialized
// 1 : initialized
int
GC_is_initialized (void);

// Return values:
// 0 : not young
// 1 : young
int
GC_is_young (struct GC_region * region);

// Return values:
// 0 : success
// 1 : error
int
GC_init_old_region (struct GC_region * region, size_t semispace_size);

// Return values:
// 0 : success
// 1 : error
int
GC_init_young_region (struct GC_region * region,
                      struct GC_region * older_region,
                      size_t sz);

// also declared in gc.h
// Return values:
// 0 : success
// 1 : error
int
GC_set_oy_technique (int oy_technique);

#endif // GC_INIT_H_HEADER
