#include "gc_init.h"
#include "gc_debug.h"
#include "gc_low.h"
#include "gc_config.h"

#include <stdio.h>
#include <stdlib.h>

struct GC_state_struct GC_state = {.initialized = 0};
__capability struct GC_state_struct * GC_state_cap;

int
GC_init2 (const char * file, int line)
{
  if (!GC_state.initialized)
  {
    GC_dbgf("GC_init called from %s:%d", file, line);
    
    GC_state_cap = GC_cheri_ptr(&GC_state, sizeof GC_state);

    int rc;
#ifdef GC_GENERATIONAL
    rc = GC_init_old_region(
      &GC_state.old_generation,
      GC_OLD_GENERATION_SEMISPACE_SIZE);
    if (rc) return rc;
    
    rc = GC_init_young_region(
      &GC_state.thread_local_region,
      &GC_state.old_generation,
      GC_THREAD_LOCAL_HEAP_SIZE);
    if (rc) return rc;
#ifdef GC_OY_RUNTIME
    GC_state.oy_technique = GC_OY_DEFAULT;
#endif // GC_OY_RUNTIME
#else // GC_GENERATIONAL
    rc = GC_init_region(
      &GC_state.thread_local_region,
      GC_THREAD_LOCAL_HEAP_SIZE);
    if (rc) return rc;
#endif // GC_GENERATIONAL
    
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
#ifdef GC_GENERATIONAL
GC_init_old_region
#else // GC_GENERATIONAL
GC_init_region
#endif // GC_GENERATIONAL
(struct GC_region * region, size_t semispace_size)
{
  // round up size to the next 32-bit boundary
  semispace_size = GC_ALIGN_32(semispace_size, size_t);
  void * p = GC_low_malloc(2*semispace_size);
  if (p == NULL)
  {
    GC_errf("malloc");
    return 1;
  }
  region->tospace = GC_cheri_ptr(p, semispace_size);
  region->fromspace = GC_cheri_ptr(p+semispace_size, semispace_size);
  region->free = region->tospace;
  region->scan = NULL;
#ifdef GC_GENERATIONAL
  region->older_region = NULL;
#endif // GC_GENERATIONAL
  region->num_collections = 0;
  return 0;
}

#ifdef GC_GENERATIONAL
int
GC_is_young (struct GC_region * region)
{
  return (GC_cheri_getbase(region->older_region) != NULL);
}

int
GC_init_young_region (struct GC_region * region,
                      struct GC_region * older_region,                      
                      size_t sz)
{
  sz = GC_ALIGN_32(sz, size_t);
  void * p = GC_low_malloc(sz);
  if (p == NULL)
  {
    GC_errf("malloc");
    return 1;
  }
  region->tospace = GC_SET_YOUNG(GC_cheri_ptr(p, sz));

  GC_CHOOSE_OY(
    region->tospace = GC_SET_YOUNG(region->tospace),      // GC_OY_MANUAL
    region->tospace = GC_SET_EPHEMERAL(region->tospace)   // GC_OY_EPHEMERAL
  );
  
  region->fromspace = GC_cheri_ptr(NULL, 0);
  region->free = region->tospace;
  region->scan = NULL;
  region->older_region = older_region;
  region->num_collections = 0;
  return 0;
}

int
GC_set_oy_technique (int oy_technique)
{
  if (!GC_is_initialized)
  {
    int rc = GC_init();
    if (rc) return rc;
  }
  GC_state.oy_technique = oy_technique;
  return 0;
}
#endif // GC_GENERATIONAL
