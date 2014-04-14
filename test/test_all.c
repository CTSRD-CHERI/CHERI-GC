
/*
 *
 * test_all.c: framework for testing my GC, the Boehm GC, and no GC
 *
 * How to use:
 *
 * 1. Add your test to the TESTS macro in the format:
 *    X_MACRO(your_test_name, "your_test_description")
 *
 * 2. Define your test function with DEFINE_TEST(your_test_name) followed by
 *    the body of the test function. The test function accepts no parameters and
 *    should return 0 on success and non-zero on failure.
 * 
 * 3. Run gmake -f Makefile.testall to compile three separate
 *    executables, one for each type of GC, and push them to the ZFS storage via
 *    wenger.
 *
 * 4. Run /mnt/mbv21zfs/testall, /mnt/mbv21zfs/testall-boehm,
 *    /mnt/mbv21zfs/testall-no-gc as appropriate.
 *
 * The things you can use in all tests are:
 *
 * Thing you can use  Type  Description (for my GC, usually)
 * TF_ULL             T     Unsigned long long type
 * tf_cap_t           T     Capability type or nothing. All pointers that might
 *                          need to be capabilities for my GC should be tagged
 *                          with this. The Boehm and no GC case use this as
 *                          appropriate.
 * tf_malloc          F     Allocate memory
 * tf_free            F     Free memory (does nothing on Boehm and my GC)
 * tf_collect         F     Force garbage collection
 * tf_store_cap       M     Use to store a capability into another
 * tf_func_t          T     Use to declare/define a function
 * tf_cheri_ptr       M     Create a capability from a base and length
 * tf_cheri_getbase   M     Get the base of a capability
 * tf_cheri_getlen    M     Get the length of a capability. Currently
 *                          unsupported for Boehm.
 * tf_cheri_gettag    M     Get the tag of a capability. Currently unsupported
 *                          for Boehm.
 * tf_invalid_ptr     M     Corresponds to an instance of an invalid pointer
 * tf_ptr_valid       M     Checks whether a pointer is valid
 * tf_gc_init         F     Initialize a GC, if there is one
 * tf_dump_stats      F     Dump GC stats
 * tf_assert          M     Assert
 * tf_printf          F     Print formatted string
 * tf_time_t          T     Time type for use with tf_time
 * tf_time            F     Get the time (note that tests are timed
 *                          automatically)
 * tf_time_diff       M     Subtract two times
 * tf_time_add        M     Add two times
 * tf_time_pretty     M     Divide time argument by a divisor based on its size
 * tf_time_pretty_unit M    Get the time unit corresponding to the argument when
 *                          divided by tf_time_pretty
 * TF_START_TIMING    M     Start timing something. Give a tf_time_t variable to
 *                          store the time in as parameter.
 * TF_STOP_TIMING     M     Stop timing the variable given previously to
 *                          TF_START_TIMING.
 * TF_STOP_TIMING_PRINT M   TF_STOP_TIMING, but also prints out the time taken.
 * TF_ALIGN_32        M     Round up void* pointer address to next multiple of
 *                          32 bytes
 * TESTF              M     Print formatted string from test function
 * (T=data type, F=function, M=macro)
 *
 * These are mapped automatically by test_all.h to the correct types for each
 * platform.
 *
 *
 */

#include "test_all.h"
#include "test_time.h"

#include <string.h>
#include <time.h>


#ifdef GC_BOEHM
void
__LOCK_MALLOC (void)
{
}
void
__UNLOCK_MALLOC (void)
{
}
#endif // GC_BOEHM

#ifdef GC_NONE
#include <stdlib.h>
tf_func_t tf_cap_t void *
tf_no_gc_malloc (size_t sz)
{
  void * p = malloc(sz);
  if (p)
    return tf_cheri_ptr(p, sz);
  else
    return tf_invalid_ptr;
}
tf_func_t void
tf_no_gc_free (tf_cap_t void * ptr)
{
  free(tf_cheri_getbase(ptr));
}
#endif // GC_NONE

tf_func_t void
tf_dump_stats (void)
{
#if defined(GC_CHERI)
  GC_debug_dump();
#elif defined(GC_BOEHM)
  tf_printf("Boehm heap size: %llu\n", (TF_ULL) GC_get_heap_size());
#endif // GC selector
}

#include <sys/time.h>

tf_func_t tf_time_t
tf_time (void)
{
  struct timeval tv;
  if (gettimeofday(&tv, NULL))
  {
    fprintf(stderr, "error: gettimeofday");
    exit(1);
  }
  return
    ((tf_time_t) tv.tv_sec) * ((tf_time_t) 1000000)
    + ((tf_time_t) tv.tv_usec);
}

typedef struct
{
  tf_cap_t void * ptr;
} P;

typedef struct node_tag
{
  tf_cap_t struct node_tag * next;
  int value;
} node;

typedef struct bintree_tag
{
  tf_cap_t struct bintree_tag * left, * right;
  int value;
} bintree;

// ----------------------------------------------------------------------------
// ------ To create a new test, just add it to the TESTS list and -------------
// ------ define it with DEFINE_TEST. Tests should return 0 on ----------------
// ------ success and non-zero on failure. main() will call all ---------------
// ------ the tests in turn. --------------------------------------------------
// ----------------------------------------------------------------------------

#define TESTS \
  /*X_MACRO(malloc_time_test, "Tests how long tf_malloc takes without collecting")*/ \
  X_MACRO(malloc_time_test_with_collect, "Tests how long tf_malloc takes with collecting")

#define DECLARE_TEST(test,descr) \
tf_func_t int \
test (const char * TEST__test_name);

#define DEFINE_TEST(test) \
tf_func_t int \
test (const char * TEST__test_name)

#define DO_TEST(test,descr) \
do { \
  tf_printf("----------Begin test " #test "----------\n"); \
  tf_printf(#test ": " descr "\n"); \
  tf_time_t before = tf_time(); \
  int e = test(#test); \
  if (e) \
  { \
    tf_printf("#########FATAL: TEST FAILED: " #test " (error code %d)\n", e); \
    exit(1); \
  } \
  else \
  { \
    tf_time_t after = tf_time(); \
    tf_printf("----------Finished test " #test " (took %llu%s)----------\n", \
      tf_time_pretty(tf_time_diff(after, before)), \
      tf_time_pretty_unit(tf_time_diff(after, before))); \
  } \
} while(0); \

#define TESTF(...) \
do { \
  tf_printf("[%s]  ", TEST__test_name); \
  tf_printf(__VA_ARGS__); \
} while (0)

#define tf_assert(cond) \
do { \
  if (!(cond)) \
    tf_printf("****FATAL: %s:%d: assertion failed: %s\n", \
      __FILE__, __LINE__, #cond); \
} while (0)

#define X_MACRO DECLARE_TEST
  TESTS
#undef X_MACRO

int
main (int argc, char **argv)
{

#ifdef MEMWATCH
  mwInit();
  mwDoFlush(1);
#endif // MEMWATCH

  tf_printf("Compiled for "
#if   defined(GC_CHERI)
  "GC_CHERI"
#elif defined(GC_BOEHM)
  "GC_BOEHM"
#elif defined(GC_NONE)
  "GC_NONE"
#else
#error "Define one of GC_CHERI, GC_BOEHM, GC_NONE."
#endif // GC_CHERI, GC_BOEHM, GC_NONE
  " at %s\n", __TIME__  " " __DATE__);
  
  int rc = tf_gc_init();
  if (rc)
  {
    tf_printf("tf_gc_init failed with %d\n", rc);
    return 1;
  }
  tf_dump_stats();
#define X_MACRO DO_TEST
  TESTS
#undef X_MACRO

#ifdef MEMWATCH
  mwTerm();
#endif // MEMWATCH  
  return 0;
}

DEFINE_TEST(malloc_time_test)
{
  size_t szalloc = 50;
  int nalloc = 1000000;
  
  tf_time_t before = tf_time();
  int i;
  for (i=0; i<nalloc; i++)
  {
#ifdef GC_CHERI
    GC_state.thread_local_region.free = GC_state.thread_local_region.tospace;
    GC_state.thread_local_region.tospace_bitmap->used = 0;
#endif // GC_CHERI
    tf_cap_t void * x = tf_malloc(szalloc);
  }
  tf_time_t after = tf_time();
  
  tf_time_t diff = after - before;
  
  tf_time_t time_per_alloc = ((double)diff)/(double)nalloc;
  
  tf_printf("Did %d allocs in %llu%s (%llu%s per alloc)\n",
    nalloc, tf_time_pretty(diff), tf_time_pretty_unit(diff),
    tf_time_pretty(time_per_alloc), tf_time_pretty_unit(time_per_alloc));

  return 0;
}

DEFINE_TEST(malloc_time_test_with_collect)
{
  size_t szalloc = 50;
  int nalloc = 1000000;
  
  tf_time_t before = tf_time();
  int i;
  for (i=0; i<nalloc; i++)
  {
    tf_cap_t void * x = tf_malloc(szalloc);
  }
  tf_time_t after = tf_time();
  
  tf_time_t diff = after - before;
  
  tf_time_t time_per_alloc = ((double)diff)/(double)nalloc;
  
  tf_printf("Did %d allocs in %llu%s (%llu%s per alloc)\n",
    nalloc, tf_time_pretty(diff), tf_time_pretty_unit(diff),
    tf_time_pretty(time_per_alloc), tf_time_pretty_unit(time_per_alloc));
  
  return 0;
}