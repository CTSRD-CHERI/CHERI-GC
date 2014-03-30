#include "ml_time.h"
#include "common.h"

#include <sys/time.h>
#include <stdio.h>

unsigned long long
ml_time (void)
{
  struct timeval tv;
  if (gettimeofday(&tv, NULL))
  {
    fprintf(stderr, "error: gettimeofday");
  }
  return
    ((unsigned long long) tv.tv_sec) * ((unsigned long long) 1000000)
    + ((unsigned long long) tv.tv_usec);
}
