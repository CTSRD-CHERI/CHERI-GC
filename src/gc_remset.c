#include "gc_remset.h"
#include "gc_low.h"
#include "gc_debug.h"

void
GC_remembered_set_init (struct GC_remembered_set * remset)
{
  remset->roots = NULL;
  remset->size = 0;
  remset->nroots = 0;
}

int
GC_remembered_set_add (struct GC_remembered_set * remset,
                       void * address)
{
  GC_dbgf("adding to remembered set: 0x%llx", (GC_ULL) address);
  
  GC_assert( address );
  GC_assert( GC_cheri_gettag(*address) );
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
    void ** tmp = GC_low_realloc(remset->roots, new_size*sizeof(void *));
    if (!tmp)
    {
      GC_fatalf("GC_remembered_set_add(): could not grow remembered set");
      return 1;
    }
    remset->roots = tmp;
    remset->size = new_size;
  }
  remset->nroots++;
  remset->roots[remset->nroots-1] = obj;
  return 0;
}

void
GC_remembered_set_clr (struct GC_remembered_set * remset)
{
  remset->nroots = 0;
}
