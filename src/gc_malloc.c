#include "gc_malloc.h"
#include "gc_init.h"
#include "gc_debug.h"
#include "gc_low.h"
#include "gc_config.h"
#include "gc_collect.h"
#include "gc_time.h"

#include <stdlib.h>

__capability void *
GC_malloc2 (
#ifdef GC_DEBUG
  const char * file, int line,
#endif // GC_DEBUG
size_t sz
)
{
  if (!GC_is_initialized()) GC_init();
  return GC_malloc_region(
#ifdef GC_DEBUG
    file, line,
#endif // GC_DEBUG
    &GC_state.thread_local_region,
    sz,
    GC_COLLECT_ON_ALLOCATION_FAILURE);
}

__capability void *
GC_malloc_region
(
#ifdef GC_DEBUG
  const char * file, int line,
#endif // GC_DEBUG
  struct GC_region * region, size_t sz, int collect_on_failure
)
{
  GC_START_TIMING(GC_malloc_region_time);
  
  // so that internal pointers in structs are properly aligned for the user and
  // the GC.
  size_t orig_sz = sz;
  sz = GC_ALIGN_32(sz, size_t);

  /*if (sz < sizeof(GC_cap_ptr))
  {
    GC_vdbgf("sz 0x%llx to small; allocating at least 0x%llx bytes",
      (GC_ULL) sz,
      (GC_ULL) sizeof(GC_cap_ptr));
    sz = sizeof(GC_cap_ptr);
  }*/
  
  int too_small = sz > (size_t) cheri_getlen(region->free);

#ifdef GC_GROW_YOUNG_HEAP
  if (too_small)
  {
    GC_vdbgf("GC_malloc_region(): growing (young) heap (sz=0x%llx)",
      (GC_ULL) sz);
    too_small = !GC_grow(region, sz);
  }
#endif // GC_GROW_YOUNG_HEAP
  
  if (too_small && collect_on_failure)
  {
    GC_vdbgf("GC_malloc_region(): collecting (young) heap (sz=0x%llx)",
      (GC_ULL) sz);
    
    GC_START_TIMING(GC_malloc_region_collect_time);
    
    GC_collect_region(region);
    
    GC_STOP_TIMING(
      GC_malloc_region_collect_time,
      "GC_malloc_region collection");
#ifdef GC_TIME
    region->time_spent_in_collector = 
      GC_time_add(region->time_spent_in_collector,
                  GC_malloc_region_collect_time);
#endif // GC_TIME
    
    too_small = sz > (size_t) cheri_getlen(region->free);
    GC_vdbgf("GC_malloc_region(): collecting complete. Too small? %d",
      too_small);
  }
  
  if (too_small)
  {
    // TODO: try to allocate directly in old generation if out of options.
    GC_errf("GC_malloc_region(): out of memory (sz=0x%llx)", (GC_ULL) sz);
    GC_debug_print_region_stats(region);
    return GC_INVALID_PTR;
  }
  
  // TODO: handle csetlen and cincbase exceptions
  
  // NOTE: if we return the cap with orig_sz as length, the GC gets confused
  //       about its *actual* length which, if <32 or not 32-bit aligned, is
  //       important. GC_collect assumes that we have actually *allocated*
  //       GC_ALIGN_32(orig_sz) bytes.
  __capability void * ret = GC_cheri_setlen(region->free, orig_sz);

  // TODO: use cincbase here to preserve permissions, and remove the stuff
  // below.
  region->free =
    GC_setbaselen(
      region->free,
      GC_cheri_getbase(region->free)+sz,
      GC_cheri_getlen (region->free)-sz);
  /*region->free = GC_cheri_ptr(
    GC_cheri_getbase(region->free)+sz,
    GC_cheri_getlen (region->free)-sz);*/

#ifdef GC_GENERATIONAL
  GC_SWITCH_WB_TYPE(
    {region->free = GC_SET_YOUNG(region->free);},
    {region->free = GC_SET_EPHEMERAL(region->free);}
  );
#endif // GC_GENERATIONAL
  
  region->num_allocations++;

#ifdef GC_DEBUG
  GC_debug_just_allocated(ret, file, line);
#else // GC_DEBUG
  GC_debug_just_allocated(ret, "(unknown file)", 0);
#endif // GC_DEBUG
    
  //GC_STOP_TIMING(GC_malloc_region_time, "GC_malloc_region(%llu)", (GC_ULL) sz);

  return ret;
  
}
