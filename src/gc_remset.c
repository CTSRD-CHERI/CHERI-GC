#include "gc_common.h"
#include "gc_remset.h"
#include "gc_low.h"
#include "gc_debug.h"

GC_FUNC void
GC_remembered_set_init (struct GC_remembered_set * remset)
{
  remset->roots = NULL;
  remset->size = 0;
  remset->nroots = 0;
  
  remset->size = 1024;
  remset->roots = GC_low_malloc(remset->size*sizeof(void*));
  if (!remset->roots)
  {
    GC_fatalf("GC_remembered_set_init(): could not initialize remembered set");
    remset->size = 0;
  }
}

GC_FUNC int
GC_remembered_set_add (struct GC_remembered_set * remset,
                       void * address)
{
  GC_vdbgf("adding to remembered set: 0x%llx", (GC_ULL) address);
  
  GC_assert( address );
  GC_assert( GC_IS_ALIGNED_32(address) );
  
  size_t i;
  for (i=0; i<remset->nroots; i++)
  {
    if (remset->roots[i] == address) return 0;
    else if (!remset->roots[i])
    {
      remset->roots[i] = address;
      return 0;
    }
  }
  
  if (remset->nroots+1 > remset->size)
  {
    size_t new_size = remset->size ? 2*remset->size : 1;
    void ** tmp = GC_low_realloc(remset->roots, new_size*sizeof(void *), 0);
    if (!tmp)
    {
      GC_fatalf("GC_remembered_set_add(): could not grow remembered set");
      return 1;
    }
    remset->roots = tmp;
    remset->size = new_size;
  }
  GC_assert( remset->nroots+1 <= remset->size );
  remset->nroots++;
  remset->roots[remset->nroots-1] = address;
  return 0;
}

GC_FUNC void
GC_remembered_set_clr (struct GC_remembered_set * remset)
{
  GC_vdbgf("cleared remembered set");
  remset->nroots = 0;
}
