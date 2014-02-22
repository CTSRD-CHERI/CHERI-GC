#include "gc_malloc.h"
#include "gc_init.h"

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <stdlib.h>

__capability void *
GC_malloc (size_t sz)
{
  if (!GC_is_initialized()) GC_init();
  
  // TODO: handle csetlen and cincbase exceptions
  
  __capability void * ret = cheri_setlen(GC_state.thread_local_region.free, sz);
  
  cheri_setreg(1, GC_state.thread_local_region.free);
  CHERI_CINCBASE(1, 1, sz);
  GC_state.thread_local_region.free = cheri_getreg(1);
  return ret;
}
