#include "gc_time.h"
#include "gc_debug.h"

#include <sys/time.h>

GC_time_t
GC_time (void)
{
  struct timeval tv;
  if (gettimeofday(&tv, NULL))
  {
    GC_errf("gettimeofday");
  }
  return
    ((GC_time_t) tv.tv_sec) * ((GC_time_t) 1000000)
    + ((GC_time_t) tv.tv_usec);
}
