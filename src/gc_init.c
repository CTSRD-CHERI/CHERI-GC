#include "gc_init.h"
#include "gc_debug.h"

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <stdio.h>
#include <stdlib.h>

struct GC_state_struct GC_state = {.initialized = 0};

int
GC_init (void)
{
  if (!GC_state.initialized)
  {
    int rc = GC_init_region(&GC_state.thread_local_region);
    if (rc) return rc;
    
    GC_state.initialized = 1;
    GC_dbgf("initialized");
  }
  return 0;
}

int
GC_is_initialized (void)
{
  return GC_state.initialized;
}


int
GC_init_region (struct GC_region * region)
{
  #define GC_SEMISPACE_SIZE   10000
  void *p = malloc(GC_SEMISPACE_SIZE);
  if (!p)
  {
    GC_errf("malloc tospace");
    return 1;
  }
  region->tospace = cheri_ptr(p, GC_SEMISPACE_SIZE);
  p = malloc(GC_SEMISPACE_SIZE);
  if (!p)
  {
    GC_errf("malloc fromspace");
    return 1;    
  }
  region->fromspace = cheri_ptr(p, GC_SEMISPACE_SIZE);
  region->free = region->fromspace;
  region->scan = cheri_zerocap();
  return 0;
}
