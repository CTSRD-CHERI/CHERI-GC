#include "gc_low.h"

#include <stdlib.h>

void *
GC_low_malloc (size_t sz)
{
  return malloc(sz);
}

void *
GC_low_calloc (size_t num, size_t sz)
{
  return calloc(num, sz);
}

void *
GC_low_realloc (void * ptr, size_t sz)
{
  return realloc(ptr, sz);
}
