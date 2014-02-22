#include "gc_malloc.h"
#include "gc_init.h"

#include <stdlib.h>

__capability void *
GC_malloc (size_t sz)
{
  if (!GC_is_initialized()) GC_init();
  return NULL;
}
