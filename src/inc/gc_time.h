#ifndef GC_TIME_H_HEADER
#define GC_TIME_H_HEADER

#include "gc_low.h"

// For the client: GC_time_t is guaranteed to be small enough to be used as an
// unsigned long long for printing purposes.

// For internal use: 1 unit of time is a microsecond.


// For convenience, when timing things:
#ifdef GC_TIME
#define GC_START_TIMING(x) \
  GC_time_t x = GC_time();
#define GC_STOP_TIMING(x,...) \
  do { \
    x = GC_time_diff(GC_time(), x); \
    static GC_time_t x##_tot = 0; \
    static int x##_num = 0; \
    x##_num++; \
    x##_tot = GC_time_add(x##_tot, x); \
    printf("[GC time] "); \
    printf(__VA_ARGS__); \
    printf( \
      " took %llu%s (avg %llu%s)\n", \
      GC_TIME_PRETTY(x), GC_TIME_PRETTY_UNIT(x), \
      GC_TIME_PRETTY(x##_tot/x##_num), GC_TIME_PRETTY_UNIT(x##_tot/x##_num)); \
  } while (0)
#else // GC_TIME
#define GC_START_TIMING(x)
#define GC_STOP_TIMING(x,...)
#endif // GC_TIME


#define GC_time_t             GC_ULL
#define GC_time_diff(a,b)     ( (a)-(b) )
#define GC_time_add(a,b)      ( (a)+(b) )

#define GC_TIME_PRETTY(x) \
( \
  (x) < 1000 ? (x) : \
  (x) < 1000000 ? ((x)+1000/2) / 1000 : \
  ((x)+1000000/2) / 1000000 \
)
#define GC_TIME_PRETTY_UNIT(x) \
( \
  (x) < 1000 ? "us" : \
  (x) < 1000000 ? "ms" : \
  "s" \
)

GC_time_t
GC_time (void);

#endif // GC_TIME_H_HEADER
