
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

tf_func_t static int
bintree_encode_value (int depth, int value);

tf_func_t static void
bintree_print (tf_cap_t bintree * tree, int depth);

tf_func_t static int
bintree_check (tf_cap_t bintree * tree, int depth, int value);

tf_func_t static tf_cap_t bintree *
bintree_create (int depth, int value);


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
  X_MACRO(pause_time_test, "Measures collector pause time")*/ \
  X_MACRO(pause_time_test2, "Measures collector pause time") \
  /*X_MACRO(bintree_test, "Measures time taken to allocate a binary tree") \
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

static int flag_data_only = 0;
static int flag_input_number = 0;

int
main (int argc, char **argv)
{

#ifdef MEMWATCH
  mwInit();
  mwDoFlush(1);
#endif // MEMWATCH
  
  if (argc < 2)
  {
    tf_printf("need a number argument\n");
    return 1;
  }
  
  flag_input_number = atoi(argv[1]);
  
  if (argc > 2)
  {
    if (!strcmp(argv[2], "data")) flag_data_only = 1;
  }

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

tf_func_t void
pause_time_test_helper (int number_of_allocations, size_t allocation_size,
                        tf_time_t * avg_out, tf_time_t * min_out,
                        tf_time_t * max_out)
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
  if (avg_out) *avg_out = avg;
  if (min_out) *min_out = tmin;
  if (max_out) *max_out = tmax;
}

tf_func_t static void
print_gc_stats2 (const char * msg)
{
  #if defined(GC_CHERI)
#if defined(GC_GENERATIONAL)
  tf_printf("[plotdata] # %s my generational GC (yI=%d yB=%d yA=%d ycur=%d ",
    msg,
    (int) GC_THREAD_LOCAL_HEAP_SIZE,
    (int) GC_state.thread_local_region.max_grow_size_before_collection,
    (int) GC_state.thread_local_region.max_grow_size_after_collection,
    (int) GC_cheri_getlen(GC_state.thread_local_region.tospace)
  );
  tf_printf("oI=%d oB=%d oA=%d ocur=%d)\n",
    (int) GC_OLD_GENERATION_SEMISPACE_SIZE,
    (int) GC_state.old_generation.max_grow_size_before_collection,
    (int) GC_state.old_generation.max_grow_size_after_collection,
    (int) GC_cheri_getlen(GC_state.old_generation.tospace)
  );
#else // GC_GENERATIONAL
  tf_printf(
    "[plotdata] # %s my copying GC (I=%d B=%d A=%d cur=%d)\n",
    msg,
    (int) GC_THREAD_LOCAL_HEAP_SIZE,
    (int) GC_state.thread_local_region.max_grow_size_before_collection,
    (int) GC_state.thread_local_region.max_grow_size_after_collection,
    (int) GC_cheri_getlen(GC_state.thread_local_region.tospace)
  );
#endif // GC_GENERATIONAL
#elif defined(GC_BOEHM)
  tf_printf(
    "[plotdata] # %s Boehm GC (I=%d)\n",
    msg,
    (int) GC_get_heap_size());
#elif defined(GC_NONE)
  tf_printf("[plotdata] # %s no GC\n", msg);
#else
  #error "Define one of GC_CHERI, GC_BOEHM, GC_NONE"
#endif // GC selector
}

DEFINE_TEST(pause_time_test)
{
  int number_of_allocations = 1000;
  
  if (!flag_data_only)
  {
    tf_printf("[plotscript] set terminal png\n"
              "[plotscript] set output \"objects/plot.png\"\n"
              "[plotscript] set title \"Time taken for GC_malloc() averaged over %d allocations\"\n"
              "[plotscript] set xlabel \"Allocation size (B)\"\n"
              "[plotscript] set ylabel \"Pause time (us)\"\n"
              "[plotscript] #set xrange [-2:2]\n"
              "[plotscript] #set yrange [-10:10]\n"
              "[plotscript] set zeroaxis\n"
              "[plotscript] #plot 'objects/plot_data' with yerrorbars\n"
              "[plotscript] plot 'objects/plot_data' with linespoints\n",
              number_of_allocations);
  }
  
  print_gc_stats2("data for");
  
  tf_printf("[plot] # %d allocations per iteration\n", number_of_allocations);
  tf_printf("[plot] # allocation size (B)        pause time (us)\n");
  
  //pause_time_test_helper(1000, 1000, NULL, NULL, NULL); // fill the heap first
  
  int i = flag_input_number;
  //for (i=0; i<=10; i++)
  //for (i=0; i<=200; i++)
  {
    size_t allocation_size = i*1000;
    tf_time_t avg, min, max;
    pause_time_test_helper(number_of_allocations, allocation_size, &avg, &min, &max);
    /*tf_printf("[plot] [avg] %d    %d\n", (int) allocation_size, (int) avg);
    tf_printf("[plot] [min] %d    %d\n", (int) allocation_size, (int) min);
    tf_printf("[plot] [max] %d    %d\n", (int) allocation_size, (int) max);*/
    tf_printf("[plotdata] %d   %d\n", (int) allocation_size, (int) avg);
  }
  
  print_gc_stats2("end of test");
  
  return 0;
}

DEFINE_TEST(pause_time_test2)
{
  int allocation_size = flag_input_number * 1000;
  int number_of_allocations = 1000;
  tf_printf("[plotdata] # %d allocations per iteration\n", number_of_allocations);
  tf_printf("[plotdata] # allocation size (B)        total time (us)\n");
  
  if (!flag_data_only)
  {
    tf_printf("[plotscript] set terminal png\n"
              "[plotscript] set output \"objects/plot.png\"\n"
              "[plotscript] set title \"Time taken by GC_malloc() for %d allocations\"\n"
              "[plotscript] set xlabel \"Allocation size (B)\"\n"
              "[plotscript] set ylabel \"Pause time (us)\"\n"
              "[plotscript] #set xrange [-2:2]\n"
              "[plotscript] #set yrange [-10:10]\n"
              "[plotscript] set zeroaxis\n"
              "[plotscript] #plot 'objects/plot_data' with yerrorbars\n"
              "[plotscript] plot 'objects/plot_data' with linespoints\n",
              number_of_allocations);
  }
  
  int i;
  tf_time_t before = tf_time();
  for (i=0; i<number_of_allocations; i++)
  {
    tf_cap_t void * p = tf_malloc(allocation_size);
    tf_free(p);
    if (!tf_ptr_valid(p))
    {
      tf_printf("out of memory\n");
      exit(1);
    }
  }
  tf_time_t after = tf_time();
  
  tf_time_t diff = after - before;
  
  tf_printf("[plotdata] %d %llu\n", allocation_size, (unsigned long long) diff);  
  
  print_gc_stats2("begin test");
  print_gc_stats2("end test");
  return 0;
}

DEFINE_TEST(bintree_test)
{
  int depth = flag_input_number;
  
#ifdef GC_CHERI
  int shift = depth >= 15 ? (depth-15) : 0;
  int init_heap_sz = 2000000;
  size_t ycur = GC_ALIGN_32(GC_THREAD_LOCAL_HEAP_SIZE, size_t);
  size_t ocur = GC_ALIGN_32(GC_OLD_GENERATION_SEMISPACE_SIZE, size_t);
  GC_state.thread_local_region.max_grow_size_before_collection =
    GC_ALIGN_32(4*((init_heap_sz*2)<<shift), size_t); // young gen max before collect
  GC_state.thread_local_region.max_grow_size_after_collection =
    GC_ALIGN_32(4*((init_heap_sz*2)<<shift), size_t); // young gen max after collect
#ifdef GC_GENERATIONAL
  GC_state.old_generation.max_grow_size_before_collection =
    GC_ALIGN_32(4*4*(init_heap_sz<<shift), size_t); // old gen max before collect
  GC_state.old_generation.max_grow_size_after_collection =
    GC_ALIGN_32(4*4*(init_heap_sz<<shift), size_t); // old gen max after collect
#endif // GC_GENERATIONAL
  // GC_region_rebase requires the stack and registers saved, which normally
  // happens inside the collector. This hack allows us to use GC_grow outside of
  // the collector...
  char buf_unaligned[32];
  void * buf_aligned = GC_ALIGN_32(buf_unaligned, void*);
  GC_state.stack_top = buf_aligned;
  GC_state.reg_bottom = buf_aligned;
  GC_state.reg_top = buf_aligned;
  
  if (!GC_grow(&GC_state.thread_local_region,
               GC_state.thread_local_region.max_grow_size_before_collection-ycur,
               GC_state.thread_local_region.max_grow_size_before_collection))
  {
    tf_printf("error: GC_grow young generation failed\n");
    exit(1);
  }

#ifdef GC_GENERATIONAL  
  if (!GC_grow(&GC_state.old_generation,
               GC_state.old_generation.max_grow_size_before_collection-ocur,
               GC_state.old_generation.max_grow_size_before_collection))
  {
    tf_printf("error: GC_grow old generation failed\n");
    exit(1);
  }
#endif // GC_GENERATIONAL
  
#endif // GC_CHERI

/*
  BOEHM STATS:
  heap size     bintree depth
  2,052,096 for depth 15 (9s)
  3,657,728 for depth 16 (18s),
  8,679,424 for depth 17 (36s),
  15,437,824 for depth 18 (72s)
  27,451,392 for depth 19 (154s)
  52,617,216 for depth 20 (312s)
*/  
  
  print_gc_stats2("begin test");
  
  int i;
  tf_time_t before = tf_time();
  tf_cap_t bintree * tree = bintree_create(depth,0);
  tf_time_t after = tf_time();
  
  tf_time_t diff = after - before;
  
  tf_printf("[plotdata] %d %llu\n", depth, (unsigned long long) diff);
  
  tf_printf("checking binary tree...\n");
  if (!bintree_check(tree, depth, 0))
  {
    tf_printf("bintree check failed, exiting.\n");
    fprintf(stderr, "error: bintree check failed, exiting.\n");
    exit(1);
  }
  tf_printf("check completed ok\n");
  //bintree_print(tree, depth); tf_printf("\n");
  
  print_gc_stats2("end test");
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


tf_func_t static int
bintree_encode_value (int depth, int value)
{
  return ((depth&0xFFFF) << 16) | (value&0xFFFF);
}

tf_func_t static tf_cap_t bintree *
bintree_create (int depth, int value)
{
  tf_cap_t bintree * tree = tf_invalid_ptr;
  tf_store_cap(tree, tf_malloc(sizeof(bintree)));
  if (!(void*)tree) return tf_invalid_ptr;
  tree->value = bintree_encode_value(depth, value);
  tree->left = tf_invalid_ptr;
  tree->right = tf_invalid_ptr;
  
  tf_malloc(50);
  if (depth > 1)
  {
    tf_malloc(50);
    
    tf_store_cap( tree->left, bintree_create(depth-1, 2*value) );
    if (!(void*)tree->left) return tf_invalid_ptr;
    tf_assert( tf_cheri_gettag(tree->left) );
    
    tf_assert( bintree_check(tree->left, depth-1, 2*value) );
    
    tf_malloc(50);
    
    tf_store_cap( tree->right, bintree_create(depth-1, 2*value+1) );
    if (!(void*)tree->right) return tf_invalid_ptr;
    tf_assert( tf_cheri_gettag(tree->right) );

    tf_assert( bintree_check(tree->right, depth-1, 2*value+1) );
    
    tf_malloc(50);
    tf_assert( bintree_check(tree->right, depth-1, 2*value+1) );
  }
  tf_malloc(50);
  if (!bintree_check(tree, depth, value))
  {
    tf_printf("At depth %d, value %d, bintree_check failed\n", depth, value);
  }
  tf_assert( bintree_check(tree, depth, value) );
  
  return tree;
}

tf_func_t static int
bintree_check (tf_cap_t bintree * tree, int depth, int value)
{
  if (!tf_cheri_gettag(tree))
  {
    tf_printf("bintree_check: no tag (d=0x%x, v=0x%x)\n", depth, value);
    return 0;
  }
#ifdef GC_CHERI
  if (!GC_IN((void*)tree, GC_state.thread_local_region.tospace)
#ifdef GC_GENERATIONAL
      && !GC_IN((void*)tree, GC_state.old_generation.tospace)
#endif // GC_GENERATIONAL
  ) 
  {
    tf_printf("bintree_check: 0x%llx not in young or old generation (d=0x%x, v=0x%x)\n",
      (tf_ull_t)(void*)tree, depth, value);
    return 0;
  }
#endif // GC_CHERI
  if (tree->value != bintree_encode_value(depth, value))
  {
    tf_printf("bintree_check: 0x%llx has bad value 0x%x (expected 0x%x) (d=0x%x, v=0x%x)\n",
      (tf_ull_t)(void*)tree, tree->value, bintree_encode_value(depth, value), depth, value);
    return 0;
  }
  if (depth>1)
    return
      bintree_check(tree->left, depth-1, 2*value) &&
      bintree_check(tree->right, depth-1, 2*value+1);
  else
  {
    if ((void*)tree->left)
    {
      tf_printf("bintree_check: not NULL left\n");
      return 0;
    }
    if ((void*)tree->right)
    {
      tf_printf("bintree_check: not NULL right\n");
      return 0;
    }
    return 1;
  }
}

tf_func_t static void
bintree_print (tf_cap_t bintree * tree, int depth)
{
  tf_assert((void*)tree);
  int val = tree->value; // compiler crashes if print tree->value directly (why?)
  
  // -O2 optimises the above delay away, causing the compiler to crash
  volatile int x = 0;
  val += x;
  
  tf_printf("[0x%llx\n", (tf_ull_t) val);
  if (depth>1)
  {
    tf_assert( (void*)tree->left );
    tf_assert( (void*)tree->right );
#ifdef GC_CHERI
    tf_assert(GC_IN(
      (void*)tree->left, GC_state.thread_local_region.tospace)
#ifdef GC_GENERATIONAL
      || GC_IN((void*)tree->left, GC_state.old_generation.tospace)
#endif // GC_GENERATIONAL
    );
    tf_assert(GC_IN(
      (void*)tree->right, GC_state.thread_local_region.tospace)
#ifdef GC_GENERATIONAL
      || GC_IN((void*)tree->right, GC_state.old_generation.tospace)
#endif // GC_GENERATIONAL
    );
#endif // GC_CHERI
    tf_printf(",L=0x%llx(", (tf_ull_t)(void*)tree->left);
    bintree_print( tree->left, depth-1);
    tf_printf("),R=0x%llx(", (tf_ull_t)(void*)tree->right);
    bintree_print( tree->right, depth-1);
    tf_printf(")");
  }
  else
  {
    tf_assert( !(void*)tree->left );
    tf_assert( !(void*)tree->right );
  }
  tf_printf("]");
}
