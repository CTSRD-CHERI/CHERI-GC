
/*
 *
 * test_all.c: framework for testing my GC, the Boehm GC, and no GC
 *
 * How to use:
 *
 * 0. Configure the collector(s) as appropriate using gc_config.h and below.
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
 * tf_ull_t           T     Unsigned long long type
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
 * tf_mem_pretty      M     Divide memory argument by a divisor based on its
 *                          size
 * tf_mem_pretty_unit M     Get the memory unit corresponding to the argument
 *                          when divided by tf_mem_pretty
 * tf_num_pretty      M     Divide number argument by a divisor based on its
 *                          size
 * tf_num_pretty_unit M     Get the number unit corresponding to the argument
 *                          when divided by tf_mem_pretty
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


/*
 * Boehm GC configuration
 *
 */
#ifdef GC_BOEHM

#include "../src/inc/gc_config.h"
#define TF_MAX_BOEHM_HEAP_SIZE  GC_BOEHM_MAX_HEAP_SIZE

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
  tf_printf("Boehm heap size: %llu\n", (tf_ull_t) GC_get_heap_size());
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
  /*X_MACRO(malloc_time_test, "Tests how long tf_malloc takes without collecting") \
  X_MACRO(malloc_time_test_with_collect, "Tests how long tf_malloc takes with collecting") \
  X_MACRO(allocate_loads, "Tests how long it takes to allocate a large amount of data") \
  X_MACRO(old_pause_time_test, "Measures pause time of GC_collect ONLY") \
  */X_MACRO(pause_time_test, "Measures collector pause time") \
  /*X_MACRO(experimental_test, "For experiments")*/

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

#ifdef GC_BOEHM
  // Limit the Boehm heap size to match that of our collector.
  GC_set_max_heap_size(TF_MAX_BOEHM_HEAP_SIZE);
#endif // GC_BOEHM

  tf_dump_stats();
#define X_MACRO DO_TEST
  TESTS
#undef X_MACRO

  tf_dump_stats();
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

DEFINE_TEST(allocate_loads)
{
  // this many bytes are allocated in each element of the array
  size_t objsz      = 16384;
  
  // this many bytes are allocated in each element
  size_t nobj       = 50;
  
  // number of allocations to do
  size_t nalloc     = 1000;
  
  // this many bytes are stored at any one time
  size_t total_stored = objsz * nobj;
  
  // this many bytes are allocated in total
  size_t total_allocated = objsz * nalloc;
  
  tf_cap_t void * refs[nobj];
  
  tf_printf(
    "Object size............%llu%s (%llu bytes)\n"
    "Number of objects......%llu%s (%llu)\n"
    "Number of allocations..%llu%s (%llu)\n"
    "Total stored...........%llu%s (%llu bytes)\n"
    "Total allocated........%llu%s (%llu bytes)\n",
    tf_mem_pretty(objsz), tf_mem_pretty_unit(objsz), (tf_ull_t) objsz,
    tf_num_pretty(nobj), tf_num_pretty_unit(nobj), (tf_ull_t) nobj,
    tf_num_pretty(nalloc), tf_num_pretty_unit(nalloc), (tf_ull_t) nalloc,
    tf_mem_pretty(total_stored), tf_mem_pretty_unit(total_stored), (tf_ull_t) total_stored,
    tf_mem_pretty(total_allocated), tf_mem_pretty_unit(total_allocated), (tf_ull_t) total_allocated);
  
  
  tf_time_t before = tf_time();
  
  size_t i;
  for (i=0; i<nalloc; i++)
  {
    size_t index = i % nobj;
    refs[index] = tf_invalid_ptr;
    tf_store_cap(refs[index], tf_malloc(objsz));
    if (!(void*)refs[index])
    {
      tf_printf("i=%d: out of memory?\n", (int)i);
      exit(1);
    }
    // TODO: check integrity of stored data...
  }
  
  tf_time_t after = tf_time();

  tf_time_t diff = after - before;
  
  tf_printf(
    "Took %llu%s (diff=%llu)\n",
    tf_time_pretty(diff), tf_time_pretty_unit(diff), (tf_ull_t) diff);
  
  return 0;
}

DEFINE_TEST(old_pause_time_test)
{
  int j, max=100, sz=512, heapsz=65536;
  tf_printf("Note: make sure the heap size is at least %d bytes big.\n", heapsz);
  tf_time_t tot = 0, tmin = 0, tmax = 0;
  for (j=0; j<max; j++)
  {
    int i;
    for (i=0; i<(heapsz-1)/sz; i++)
    {
      tf_malloc(sz);
    }
    tf_time_t before = tf_time();
    tf_collect();
    tf_time_t after = tf_time();
    tf_time_t diff = after - before;
    if (tot+diff<tot)
    {
      tf_printf("tot overflow\n");
      exit(1);
    }
    tot += diff;
    if (diff > tmax) tmax = diff;
    if ((diff < tmin)||!tmin) tmin = diff;
    tf_printf("[%d] Pause time %llu%s (%llu)\n",
      j, tf_time_pretty(diff), tf_time_pretty_unit(diff), (tf_ull_t) diff);
  }
  tf_time_t avg = tot/max;
  tf_printf(
    "Pause time %llu%s (%llu)\nTotal time %llu%s (%llu))\nMin pause %llu%s (%llu))\nMax pause %llu%s (%llu))\n",
    tf_time_pretty(avg), tf_time_pretty_unit(avg), (tf_ull_t) avg,
    tf_time_pretty(tot), tf_time_pretty_unit(tot), (tf_ull_t) tot,
    tf_time_pretty(tmin), tf_time_pretty_unit(tmin), (tf_ull_t) tmin,
    tf_time_pretty(tmax), tf_time_pretty_unit(tmax), (tf_ull_t) tmax);
  return 0;
}

tf_func_t tf_time_t
pause_time_test_helper (int number_of_allocations, size_t allocation_size)
{
  // so that this function can be used many times in succession without previous
  // objects affecting the results
//#ifdef GC_CHERI
  //GC_reset();
//#endif // GC_CHERI
  // TODO: similar for Boehm
  
  tf_printf(
    "Doing %d %llu%s allocations\n",
    number_of_allocations,
    tf_mem_pretty(allocation_size), tf_mem_pretty_unit(allocation_size));
  
  tf_time_t tot = 0, tmin = 0, tmax = 0;
  
  int i;
  for (i=0; i<number_of_allocations; i++)
  {
    //printf("[%d] allocating...\n", i);
    tf_time_t before = tf_time();
    tf_cap_t void * p = tf_malloc(allocation_size);
    tf_free(p);
    tf_time_t after = tf_time();
    tf_time_t diff = after - before;
    if (!tf_ptr_valid(p))
    {
      tf_printf("out of memory\n");
      exit(1);
    }
    //printf("allocated (%d).\n", (int) diff);
    if ((tot+diff) < tot)
    {
      tf_printf("tot overflow\n");
      exit(1);
    }
    tot += diff;
    if (diff > tmax) tmax = diff;
    if ((diff < tmin) || !tmin) tmin = diff;
  }
  tf_time_t avg = tot/number_of_allocations;
  tf_printf(
    "Average pause time %llu%s (%llu)\nTotal pause time %llu%s (%llu))\nMin pause %llu%s (%llu))\nMax pause %llu%s (%llu))\n",
    tf_time_pretty(avg), tf_time_pretty_unit(avg), (tf_ull_t) avg,
    tf_time_pretty(tot), tf_time_pretty_unit(tot), (tf_ull_t) tot,
    tf_time_pretty(tmin), tf_time_pretty_unit(tmin), (tf_ull_t) tmin,
    tf_time_pretty(tmax), tf_time_pretty_unit(tmax), (tf_ull_t) tmax);
  return avg;
}

DEFINE_TEST(pause_time_test)
{
#if defined(GC_CHERI)
#if defined(GC_GENERATIONAL)
  tf_printf(
    "[plot] # data for my generational GC (yI=%d yB=%d yA=%d oI=%d oB=%d oA=%d)\n",
    (int) GC_THREAD_LOCAL_HEAP_SIZE,
    (int) GC_THREAD_LOCAL_HEAP_MAX_SIZE_BEFORE_COLLECTION,
    (int) GC_THREAD_LOCAL_HEAP_MAX_SIZE,
    (int) GC_OLD_GENERATION_SEMISPACE_SIZE,
    (int) GC_OLD_GENERATION_SEMISPACE_MAX_SIZE_BEFORE_COLLECTION,
    (int) GC_OLD_GENERATION_SEMISPACE_MAX_SIZE);
#else // GC_GENERATIONAL
  tf_printf(
    "[plot] # data for my copying GC (I=%d B=%d A=%d)\n",
    (int) GC_THREAD_LOCAL_HEAP_SIZE,
    (int) GC_THREAD_LOCAL_HEAP_MAX_SIZE_BEFORE_COLLECTION,
    (int) GC_THREAD_LOCAL_HEAP_MAX_SIZE);
#endif // GC_GENERATIONAL
#elif defined(GC_BOEHM)
  tf_printf(
    "[plot] # data for my Boehm GC (I=%d)\n",
    (int) GC_get_heap_size());
#elif defined(GC_NONE)
  tf_printf("[plot] # data for no GC\n");
#else
  #error "Define one of GC_CHERI, GC_BOEHM, GC_NONE"
#endif // GC selector

  int number_of_allocations = 1000;
  tf_printf("[plot] # %d allocations per iteration\n", number_of_allocations);
  tf_printf("[plot] # allocation size (B)        avg pause time (us)\n");
  
  pause_time_test_helper(1000, 1000); // fill the heap first
  
  int i;
  //for (i=0; i<=10; i++)
  for (i=10; i>=0; i--)
  {
    size_t allocation_size = i*1000;
    tf_time_t avg = pause_time_test_helper(number_of_allocations, allocation_size);
    tf_printf("[plot] %d    %d\n", (int) allocation_size, (int) avg);
  }
  return 0;
}

DEFINE_TEST(experimental_test)
{
  P tmp;
  tmp.ptr = tf_cheri_ptr((void*)0x1234, 0x5678);
  
  tf_cap_t P * tmp2 = tf_cheri_ptr(&tmp, sizeof(P));
  printf("tmp2 is 0x%llx\n", (tf_ull_t)(void*)tmp2);
  printf("tmp2->ptr is 0x%llx\n", (tf_ull_t)(void*)(tmp2->ptr));
  return 0;
}
