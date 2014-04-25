#include "gc_memcpy.h"
#include "gc_init.h"
#include "gc_low.h"
#include "gc_debug.h"

#include <string.h>

void *
GC_memcpy (void * dest, const void * src, size_t sz)
{
  memcpy(dest, src, sz);
  
  // For code simplicity, require src and dest to be 32-byte aligned (for now).
  // This should be the case at least for any objects allocated by the
  // collector.
  if (!GC_IS_ALIGNED_32(src) || !GC_IS_ALIGNED_32(dest))
  {
    GC_fatalf("GC_memcpy: require src and dest to be 32-byte aligned\n");
  }
  
  GC_CAP void ** dest_child = dest;
  GC_CAP void * const * src_child  = src;
  size_t i;
  for (i = 0; i < sz / sizeof(GC_cap_ptr); i++)
  {
    if (GC_cheri_gettag(src_child[i]))
    {
      GC_STORE_CAP(dest_child[i], src_child[i]);
    }
  }

  return dest;
}

