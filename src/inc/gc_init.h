#ifndef GC_INIT_H_HEADER
#define GC_INIT_H_HEADER

#include <stdlib.h>

struct GC_region
{
  __capability void * tospace, * fromspace, * free;
};

struct GC_state_struct
{
  int initialized;
  struct GC_region thread_local_region;
  void * stack_bottom, * static_bottom, * static_top;
  int num_collections; // debugging/stats
};

extern struct GC_state_struct GC_state;

// For convenience. Contains the data segment area used internally for GC state.
extern __capability struct GC_state_struct * GC_state_cap;

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
// 0 : success
// 1 : error
int
GC_init_region (struct GC_region * region, size_t semispace_size);

#endif // GC_INIT_H_HEADER
