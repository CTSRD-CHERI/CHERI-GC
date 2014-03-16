#include "gc_init.h"
#include "gc_debug.h"
#include "gc_low.h"

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
    
    GC_state.stack_bottom = GC_get_stack_bottom();
    if (GC_state.stack_bottom == NULL) return 1;
    
    GC_state.static_bottom = GC_get_static_bottom();
    if (GC_state.stack_bottom == NULL) return 1;

    GC_state.static_top = GC_get_static_top();
    if (GC_state.stack_bottom == NULL) return 1;
    
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
  #define GC_SEMISPACE_SIZE   0x10000
  void *p = GC_low_malloc(2*GC_SEMISPACE_SIZE);
  if (p == NULL)
  {
    GC_errf("malloc");
    return 1;
  }
  region->tospace = GC_cheri_ptr(p, GC_SEMISPACE_SIZE);
  region->fromspace = GC_cheri_ptr(p+GC_SEMISPACE_SIZE, GC_SEMISPACE_SIZE);
  region->free = region->fromspace;
  region->scan = GC_cheri_ptr(0, 0);
  return 0;
}
