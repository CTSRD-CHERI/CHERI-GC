#ifndef TEST_TIME_H_HEADER
#define TEST_TIME_H_HEADER

#include "test_all.h"

// For the client: tf_time_t is guaranteed to be small enough to be used as an
// unsigned long long for printing purposes.

// For internal use: 1 unit of time is a microsecond.

// For convenience, when timing things:
#define TF_START_TIMING(x) \
  tf_time_t x = tf_time();
#define TF_STOP_TIMING(x,...) \
  do { \
    x = tf_time_diff(tf_time(), x); \
    static tf_time_t x##_tot = 0; \
    static int x##_num = 0; \
    x##_num++; \
    x##_tot = tf_time_add(x##_tot, x); \
  } while (0)
#define TF_STOP_TIMING_PRINT(x,...) \
  do { \
    x = tf_time_diff(tf_time(), x); \
    static tf_time_t x##_tot = 0; \
    static int x##_num = 0; \
    x##_num++; \
    x##_tot = tf_time_add(x##_tot, x); \
    printf("[GC time] "); \
    printf(__VA_ARGS__); \
    printf( \
      " took %llu%s (avg %llu%s)\n", \
      tf_time_pretty(x), tf_time_pretty_unit(x), \
      tf_time_pretty(x##_tot/x##_num), tf_time_pretty_unit(x##_tot/x##_num)); \
  } while (0)
#endif // tf_time


#define tf_time_t             TF_ULL
#define tf_time_diff(a,b)     ( (a)-(b) )
#define tf_time_add(a,b)      ( (a)+(b) )

#define tf_time_pretty(x) \
((TF_ULL) ( \
  (x) < 1000 ? (x) : \
  (x) < 10000000 ? ((x)+1000/2) / 1000 : \
  ((x)+1000000/2) / 1000000 \
))
#define tf_time_pretty_unit(x) \
( \
  (x) < 1000 ? "us" : \
  (x) < 10000000 ? "ms" : \
  "s" \
)

tf_func tf_time_t
tf_time (void);

#endif // TEST_TIME_H_HEADER
