#include "gc_malloc.h"
#include "gc_init.h"
#include "gc_debug.h"
#include "gc_low.h"
#include "gc_config.h"
#include "gc_collect.h"

#include <stdlib.h>

__capability void *
GC_malloc (size_t sz)
{
  if (!GC_is_initialized()) GC_init();
  return GC_malloc_region(&GC_state.thread_local_region,
                          sz,
                          GC_COLLECT_ON_ALLOCATION_FAILURE);
}

__capability void *
GC_malloc_region (struct GC_region * region, size_t sz, int collect_on_failure)
{
  if (sz < sizeof(GC_cap_ptr))
  {
    GC_vdbgf("sz 0x%llx to small; allocating at least 0x%llx bytes",
      (GC_ULL) sz,
      (GC_ULL) sizeof(GC_cap_ptr));
    sz = sizeof(GC_cap_ptr);
  }

  if (sz > (size_t) cheri_getlen(region->free))
  {
    GC_vdbgf("sz too big: 0x%llx", (GC_ULL) sz);
    if (collect_on_failure)
    {
      GC_collect_region(region);
      return GC_malloc_region(region, sz, 0);
    }
    else
    {
      // TODO: try to grow the heap.
      GC_errf("out of memory");
      return GC_INVALID_PTR;
    }
  }
  
  // TODO: handle csetlen and cincbase exceptions
  
  __capability void * ret = GC_cheri_setlen(region->free, sz);
  
  // TODO: use cincbase here to preserve permissions, and remove the stuff
  // below.
  region->free = GC_cheri_ptr(
    GC_cheri_getbase(region->free)+sz,
    GC_cheri_getlen (region->free)-sz);

#ifdef GC_GENERATIONAL
#ifdef GC_OY_RUNTIME
  if (GC_state.oy_technique == GC_OY_MANUAL)
    region->free = GC_SET_YOUNG(region->free);
  else if (GC_state.oy_technique == GC_OY_EPHEMERAL)
    region->free = GC_SET_EPHEMERAL(region->free);
#elif GC_OY_DEFAULT == GC_OY_MANUAL
  region->free = GC_SET_YOUNG(region->free);
#elif GC_OY_DEFAULT == GC_OY_EPHEMERAL
  region->free = GC_SET_EPHEMERAL(region->free);
#endif
#endif // GC_GENERATIONAL
  return ret;
  
}
