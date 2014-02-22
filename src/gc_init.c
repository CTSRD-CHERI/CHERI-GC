#include "gc_init.h"
#include "gc_debug.h"

#include <stdio.h>

struct GC_state_struct GC_state = {.initialized = 0};

int
GC_init (void)
{
  GC_state.initialized = 1;
  GC_dbgf("initialized");
  return 0;
}

int
GC_is_initialized (void)
{
  return GC_state.initialized;
}
