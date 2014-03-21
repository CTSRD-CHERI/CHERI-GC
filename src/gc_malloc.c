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
    GC_dbgf("sz 0x%llx to small; allocating at least 0x%llx bytes",
      (GC_ULL) sz,
      (GC_ULL) sizeof(GC_cap_ptr));
    sz = sizeof(GC_cap_ptr);
  }

  if (sz > (size_t) cheri_getlen(region->free))
  {
    GC_dbgf("sz too big: 0x%llx", (GC_ULL) sz);
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
  
  region->free = GC_cheri_ptr(
    GC_cheri_getbase(region->free)+sz,
    GC_cheri_getlen (region->free)-sz);
  
  return ret;
  
}
