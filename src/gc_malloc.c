#include "gc_malloc.h"
#include "gc_init.h"
#include "gc_debug.h"
#include "gc_low.h"

#include <stdlib.h>

__capability void *
GC_malloc (size_t sz)
{
  if (!GC_is_initialized()) GC_init();
  return GC_malloc_region(&GC_state.thread_local_region, sz);
}

__capability void *
GC_malloc_region (struct GC_region * region, size_t sz)
{  
  if (sz > (size_t) cheri_getlen(region->free))
  {
    GC_errf("sz too big: 0x%llx", (GC_ULL) sz);
    return GC_cheri_ptr(0, 0);
  }
  
  if (sz < sizeof(GC_cap_ptr))
  {
    GC_dbgf("sz 0x%llx to small; allocating at least 0x%llx bytes",
      (GC_ULL) sz,
      (GC_ULL) sizeof(GC_cap_ptr));
    sz = sizeof(GC_cap_ptr);
  }
  
  // TODO: handle csetlen and cincbase exceptions
  
  __capability void * ret = GC_cheri_setlen(region->free, sz);
  
  region->free = GC_cheri_ptr(
    GC_cheri_getbase(region->free)+sz,
    GC_cheri_getlen (region->free)-sz);
  
  /*cheri_setreg(1, region->free);
  CHERI_CINCBASE(1, 1, sz);
  region->free = cheri_getreg(1);*/
  
  return ret;
  
}
