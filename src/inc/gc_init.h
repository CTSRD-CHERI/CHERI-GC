#ifndef GC_INIT_H_HEADER
#define GC_INIT_H_HEADER

#include <stdlib.h>

struct GC_root
{
  __capability void * cap;
};

struct GC_region
{
  __capability void * tospace, * fromspace, * scan, * free;
  struct GC_root * roots;
  size_t nroots; // the actual number of roots stored
  size_t maxroots; // size of the array
};

struct GC_state_struct
{
  int initialized;
  struct GC_region thread_local_region;
};

extern struct GC_state_struct GC_state;

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
GC_init_region (struct GC_region * region);

#endif // GC_INIT_H_HEADER
