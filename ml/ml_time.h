#ifndef ML_TIME_H_HEADER
#define ML_TIME_H_HEADER

#include "common.h"

// For internal use: 1 unit of time is a microsecond.

// For convenience, when timing things:
#define ML_START_TIMING(x) \
  unsigned long long x = ml_time();
#define ML_STOP_TIMING(x,...) \
  do { \
    x = ml_time() - x; \
    static unsigned long long x##_tot = 0; \
    static int x##_num = 0; \
    x##_num++; \
    x##_tot += x; \
    printf("[ML time] "); \
    printf(__VA_ARGS__); \
    printf( \
      " took %llu%s (avg %llu%s)\n", \
      ML_TIME_PRETTY(x), ML_TIME_PRETTY_UNIT(x), \
      ML_TIME_PRETTY(x##_tot/x##_num), ML_TIME_PRETTY_UNIT(x##_tot/x##_num)); \
  } while (0)

#define ML_TIME_PRETTY(x) \
( \
  (x) < 1000 ? (x) : \
  (x) < 10000000 ? ((x)+1000/2) / 1000 : \
  ((x)+1000000/2) / 1000000 \
)
#define ML_TIME_PRETTY_UNIT(x) \
( \
  (x) < 1000 ? "us" : \
  (x) < 10000000 ? "ms" : \
  "s" \
)

unsigned long long
ml_time (void);

#endif // ML_TIME_H_HEADER
