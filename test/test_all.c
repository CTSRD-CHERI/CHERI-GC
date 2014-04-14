
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
 * 3. Run gmake -f Makefile.CHERI test_all to compile three separate
 *    executables, one for each type of GC, and push them to the ZFS storage via
 *    wenger.
 *
 * 4. Run /mnt/mbv21zfs/test_all, /mnt/mbv21zfs/test_all-boehm,
 *    /mnt/mbv21zfs/test_all-no-gc as appropriate.
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
tf_func_t tf_cap void *
tf_no_gc_malloc (size_t sz)
{
  void * p = malloc(sz);
  if (p)
    return tf_cheri_ptr(p, sz);
  else
    return tf_invalid_ptr;
}
tf_func_t void
tf_no_gc_free (tf_cap void * ptr)
{
  free(tf_cheri_getbase(ptr));
}
#endif // GC_NONE

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
  /*X_MACRO(fill_test, "Fill the heap with 512-byte chunks and ensure integrity after collection") \
  X_MACRO(list_test, "Fill the heap with a list and ensure integrity after collection") \
  X_MACRO(bintree_test, "Create some binary trees and ensure integrity after collection") \
  X_MACRO(regroots_test, "Check register roots") \
  X_MACRO(bitmap_test, "Check bitmap operations") \
  X_MACRO(experimental_test, "For experiments") \
  X_MACRO(malloc_time_test, "Tests how long tf_malloc takes without collecting")*/ \
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
  return 0;
}

DEFINE_TEST(fill_test)
{
#define typ int
  const int bufsz = 128*sizeof(typ);
  const int maxnbufs = 100000;
  tf_cap_t char * bufs[maxnbufs/bufsz];
  
  size_t heapsz =
    GC_state.thread_local_region.max_grow_size_after_collection;
  tf_assert( heapsz );
  
  heapsz -= heapsz/10;
  tf_assert( heapsz > 0 );
  
  size_t nbufs = heapsz / bufsz;
  tf_assert( maxnbufs > nbufs );
  
  TESTF("nbufs: %llu  bufsz: %llu\n", (GC_ULL) nbufs, (GC_ULL) bufsz);
  
  size_t i;
  for (i=0; i<nbufs; i++)
  {
    bufs[i] = tf_invalid_ptr;
    tf_store_cap(bufs[i], tf_malloc(bufsz));
    
    tf_assert( (typ*)bufs[i] );
    tf_assert( GC_cheri_getlen(bufs[i]) == bufsz );
    
    size_t j;
    for (j=0; j<bufsz/sizeof(typ); j++)
    {
      ((typ*)bufs[i])[j] = i;
    }
    
    // dummy, should be lost on collection
    typ * tmp = (typ*) tf_malloc(bufsz*2);
    tf_assert( tmp );
    memset(tmp, 0, bufsz*2);
  }
  
  TESTF("filled %llu buffers\n", (GC_ULL) nbufs);
  
  tf_collect();
  tf_collect();
  tf_collect();
  tf_assert( heapsz/100 );
  for (i=0; i<heapsz/100; i++)
  {
    tf_assert( (void*) tf_malloc(heapsz/10) );
  }
  tf_collect();
  tf_collect();
  
  TESTF("checking buffers\n");

  for (i=0; i<nbufs; i++)
  {
    tf_assert( GC_cheri_gettag(bufs[i]) );
    if (!GC_IN((void*)bufs[i], GC_state.thread_local_region.tospace))
    {
      tf_printf(
        "Warning: bufs[%d] (0x%llx) "
        "is not in the tospace (0x%llx to 0x%llx)\n",
        (int) i, (GC_ULL)(void*)bufs[i],
        (GC_ULL)(void*)GC_state.thread_local_region.tospace,
        (GC_ULL)(((void*)GC_state.thread_local_region.tospace)+
                GC_cheri_getlen(GC_state.thread_local_region.tospace)));
    }
  }
  
  for (i=0; i<nbufs; i++)
  {
    tf_assert( (typ*)bufs[i] );
    tf_assert( GC_cheri_getlen(bufs[i]) == bufsz );
    if (!GC_IN((void*)bufs[i], GC_state.thread_local_region.tospace))
    {
      tf_printf("NOTE: bufs[i]=0x%llx, i=%d\n", (GC_ULL)(void*)bufs[i], (int) i);
      GC_debug_print_region_stats(&GC_state.thread_local_region);
    }
    tf_assert( GC_IN((void*)bufs[i], GC_state.thread_local_region.tospace) );
    
    size_t j;
    for (j=0; j<bufsz/sizeof(typ); j++)
    {      
      if ( ((typ*)bufs[i])[j] != (typ) i )
      {
        tf_printf("NOTE: i=%d, j=%d\n", (int) i, (int) j);
        GC_debug_memdump((typ*)bufs[i], ((typ*)bufs[i])+bufsz);
      }
      
      tf_assert( ((typ*)bufs[i])[j] == (typ) i );
    }
  }
  
  return 0;
}

DEFINE_TEST(list_test)
{
  // Don't allow an unlimited heap
  tf_assert( GC_state.thread_local_region.max_grow_size_before_collection );
  tf_assert( GC_state.thread_local_region.max_grow_size_after_collection );
 
  size_t heapsz =
    GC_state.thread_local_region.max_grow_size_after_collection;
  tf_assert( heapsz );
  
  int i;
  tf_cap_t node * head = tf_invalid_ptr;
  for (i=0; ; i++)
  {
    if (i > heapsz/sizeof(node))
    {
      TESTF("FATAL: too many nodes!\n");
      GC_debug_print_region_stats(&GC_state.thread_local_region);
      return 1;
    }
    
    tf_cap_t node * p = tf_invalid_ptr;
    tf_store_cap(p, tf_malloc(sizeof(node)));
    if (!(void*)p) break;
    
    // dummy, should be lost on collection
    void * tmp = (void*) tf_malloc(500);
    if (tmp) memset(tmp, 0, 500);
    
    ((node*)p)->value = i;
    tf_store_cap(((node*)p)->next, head);
    tf_store_cap(head, p);
  }
  
  i--;
  
  TESTF("allocated %d nodes (each of size %llu)\n", i, (GC_ULL) sizeof(node));
  
  tf_collect();
  tf_collect();
  tf_collect();
  tf_collect();
  tf_collect();
  
  int j;
  tf_cap_t node * p = tf_invalid_ptr;
  tf_store_cap(p, head);
  for (j=0; j<=i; j++)
  {
    tf_assert( GC_IN(p, GC_state.thread_local_region.tospace) );
    if (!(void*)p)
    {
      tf_printf("failed at j=%d\n", j);
    }
    tf_assert( (void*)p );
    if (((node*)p)->value != (i - j))
    {
      tf_printf("at j=%d, the value is %d (0x%llx)\n", j, ((node*)p)->value, (GC_ULL) ((node*)p)->value);
    }
    tf_assert( ((node*)p)->value == (i - j) );
    tf_store_cap(p, ((node*)p)->next);
  }
 
  tf_assert( !(void*)p );
  
  TESTF("checked %d nodes\n", i);
  
  return 0;
}

tf_func_t static int
bintree_encode_value (int depth, int value)
{
  return ((depth&0xFFFF) << 16) | (value&0xFFFF);
}

tf_func_t static void
bintree_print (tf_cap_t bintree * tree, int depth);
tf_func_t static int
bintree_check (tf_cap_t bintree * tree, int depth, int value);

tf_func_t static tf_cap_t bintree *
bintree_create (int depth, int value)
{
  tf_cap_t bintree * tree = tf_invalid_ptr;
  tf_store_cap(tree, tf_malloc(sizeof(bintree)));
  //tree = tf_cheri_ptr(GC_low_malloc(sizeof(bintree)), sizeof(bintree));
  if (!(void*)tree) return tf_invalid_ptr;
  ((bintree*)tree)->value = bintree_encode_value(depth, value);
  ((bintree*)tree)->left = tf_invalid_ptr;
  ((bintree*)tree)->right = tf_invalid_ptr;
  
  tf_printf("The tree root is at 0x%llx\n", (GC_ULL) &tree);
  tf_malloc(50);
  if (depth > 1)
  {
    tf_malloc(50);
    
    tf_store_cap( ((bintree*)tree)->left, bintree_create(depth-1, 2*value) );
    if (!(void*)((bintree*)tree)->left) return tf_invalid_ptr;
    tf_assert( GC_cheri_gettag(((bintree*)tree)->left) );
    
    tf_assert( bintree_check(((bintree*)tree)->left, depth-1, 2*value) );
    
    tf_malloc(50);
    
    tf_store_cap( ((bintree*)tree)->right, bintree_create(depth-1, 2*value+1) );
    if (!(void*)((bintree*)tree)->right) return tf_invalid_ptr;
    tf_assert( GC_cheri_gettag(((bintree*)tree)->right) );

    tf_assert( bintree_check(((bintree*)tree)->right, depth-1, 2*value+1) );
    
    tf_malloc(50);
    tf_assert( bintree_check(((bintree*)tree)->right, depth-1, 2*value+1) );
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
  tf_printf("Bintree checking at depth=%d, value=%d, tree=0x%llx\n", depth, value, (GC_ULL) (void*)tree);
  if (!GC_cheri_gettag(tree) ||
      !GC_IN((void*)tree, GC_state.thread_local_region.tospace) ||
      !(void*)tree ||
      ((bintree*)tree)->value != bintree_encode_value(depth, value))
    return 0;
  if (depth>1)
    return
      bintree_check(((bintree*)tree)->left, depth-1, 2*value) &&
      bintree_check(((bintree*)tree)->right, depth-1, 2*value+1);
  else
    return !(void*)((bintree*)tree)->left &&
           !(void*)((bintree*)tree)->right;
}

tf_func_t static void
bintree_print (tf_cap_t bintree * tree, int depth)
{
  tf_assert((void*)tree);
  tf_printf("[0x%llx\n", (GC_ULL) ((bintree*)tree)->value);
  if (depth>1)
  {
    tf_assert( (void*)((bintree*)tree)->left );
    tf_assert( (void*)((bintree*)tree)->right );
    tf_assert(GC_IN(
      (void*)((bintree*)tree)->left, GC_state.thread_local_region.tospace 
    ));
    tf_assert(GC_IN(
      (void*)((bintree*)tree)->right, GC_state.thread_local_region.tospace 
    ));
    tf_printf(",L(");
    bintree_print( ((bintree*)tree)->left, depth-1);
    tf_printf("),R(");
    bintree_print( ((bintree*)tree)->right, depth-1);
    tf_printf(")");
  }
  else
  {
    tf_assert( !(void*)((bintree*)tree)->left );
    tf_assert( !(void*)((bintree*)tree)->right );
  }
  tf_printf("]");
}

DEFINE_TEST(bintree_test)
{
  // Don't allow an unlimited heap
  tf_assert( GC_state.thread_local_region.max_grow_size_before_collection );
  tf_assert( GC_state.thread_local_region.max_grow_size_after_collection );
 
  size_t heapsz =
    GC_state.thread_local_region.max_grow_size_after_collection;
  tf_assert( heapsz );
  
  const int max_trees = 1;
  const int tree_depth = 3;
  
  tf_cap_t bintree * trees[max_trees];
  
  int i;
  for (i=0; i<max_trees; i++)
  {
    TESTF("allocated %d trees so far\n", i);
    GC_debug_print_region_stats(&GC_state.thread_local_region);
    trees[i] = bintree_create(tree_depth, i);
    if (!(void*)trees[i]) break;
  }
  bintree_print(trees[0],tree_depth);
  
  TESTF("allocated %d trees\n", i);
  
  tf_collect();
  tf_collect();
  tf_collect();
  tf_collect();
  tf_collect();

  int j;
  for (j=0; j<i; j++)
  {
    tf_assert( bintree_check(trees[j], tree_depth, j) );
  }
  
  TESTF("checked %d trees\n", j);
  
  return 0;
}

DEFINE_TEST(regroots_test)
{
  tf_cap_t void * x = tf_malloc(50);
  tf_printf("before collecting, x.base = 0x%llx\n", (GC_ULL)(void*)x);
  GC_PRINT_CAP(GC_state.thread_local_region.tospace);
  tf_collect();
  tf_printf("after collecting, x.base = 0x%llx\n", (GC_ULL)(void*)x);
  GC_PRINT_CAP(GC_state.thread_local_region.tospace);
  return !GC_IN((void*)x, GC_state.thread_local_region.tospace);
}

DEFINE_TEST(bitmap_test)
{
  struct GC_bitmap bitmap;
  size_t sz = 811;
  size_t i;
  
  tf_assert( !tf_gc_init_bitmap(&bitmap, sz) );
  tf_assert( bitmap.size == sz );
  tf_assert( bitmap.used == 0 );
  
  for (i=0; i<bitmap.size; i++)
  {
    int bit = GC_BITMAP_GET(&bitmap, i);
    tf_assert( !bit );
    tf_printf("%d", bit);
  }
  tf_printf("\n");
  
  GC_bitmap_allocate(&bitmap, 23);
  GC_bitmap_allocate(&bitmap, 42);
  GC_bitmap_allocate(&bitmap, 19);
  GC_bitmap_allocate(&bitmap, 7);

  for (i=0; i<bitmap.size; i++)
  {
    int bit = GC_BITMAP_GET(&bitmap, i);
    if ((i == 0) || (i == 23) || (i==23+42) || (i==23+42+19))
      tf_assert( bit );
    else
      tf_assert( !bit );
    tf_printf("%d", bit);
  }
  tf_printf("\n");
  
  tf_assert( !GC_bitmap_find(&bitmap, 0, 10) );
  tf_assert( !GC_bitmap_find(&bitmap, 0, 0) );
  tf_assert( !GC_bitmap_find(&bitmap, 0, 1) );
  tf_assert( !GC_bitmap_find(&bitmap, 0, 22) );
  tf_assert( !GC_bitmap_find(&bitmap, 0, 24) );
  tf_assert( !GC_bitmap_find(&bitmap, 0, 0) );
  tf_assert( GC_bitmap_find(&bitmap, 0, 23) );
  tf_assert( !GC_bitmap_find(&bitmap, 23, 23) );
  tf_assert( !GC_bitmap_find(&bitmap, 23, 0) );
  tf_assert( !GC_bitmap_find(&bitmap, 23, 41) );
  tf_assert( !GC_bitmap_find(&bitmap, 23, 1) );
  tf_assert( !GC_bitmap_find(&bitmap, 23, 43) );
  tf_assert( GC_bitmap_find(&bitmap, 23, 42) );
  tf_assert( GC_bitmap_find(&bitmap, 23+42, 19) );
  tf_assert( !GC_bitmap_find(&bitmap, 23+42+19, 6) );
  tf_assert( !GC_bitmap_find(&bitmap, 23+42+19, 99) );
  tf_assert( !GC_bitmap_find(&bitmap, 23+42+19, 8) );
  tf_assert( !GC_bitmap_find(&bitmap, 23+42+19, 0) );
  tf_assert( !GC_bitmap_find(&bitmap, 23+42+19, 1) );
  tf_assert( !GC_bitmap_find(&bitmap, 23, 19999999) );
  tf_assert( GC_bitmap_find(&bitmap, 23+42+19, 7) );
  
  
  // clear
  GC_bitmap_clr(&bitmap);
  tf_assert( bitmap.used == 0 );
  for (i=0; i<bitmap.size; i++)
  {
    int bit = GC_BITMAP_GET(&bitmap, i);
    tf_assert( !bit );
  }
  
  // try to resize the bitmap
  GC_bitmap_allocate(&bitmap, 800);
  GC_bitmap_allocate(&bitmap, 5);
  GC_bitmap_allocate(&bitmap, 5);
  GC_bitmap_allocate(&bitmap, 1);
  for (i=0; i<bitmap.size; i++)
  {
    int bit = GC_BITMAP_GET(&bitmap, i);
    tf_printf("%d", bit);
  }
  tf_printf("\n");
  GC_grow_bitmap(&bitmap, 812);
  
  tf_assert( GC_bitmap_find(&bitmap, 810, 1) );
  tf_assert( !GC_bitmap_find(&bitmap, 811, 1) );
  tf_assert( !GC_bitmap_find(&bitmap, 812, 1) );
  tf_assert( !GC_bitmap_find(&bitmap, 813, 1) );
  tf_assert( !GC_bitmap_find(&bitmap, 811, 0) );
  tf_assert( !GC_bitmap_find(&bitmap, 812, 0) );
  tf_assert( !GC_bitmap_find(&bitmap, 810, 0) );
  
  GC_bitmap_allocate(&bitmap, 1);
  for (i=0; i<bitmap.size; i++)
  {
    int bit = GC_BITMAP_GET(&bitmap, i);
    tf_printf("%d", bit);
  }
  tf_printf("\n");
  tf_assert( GC_bitmap_find(&bitmap, 810, 1) );
  tf_assert( GC_bitmap_find(&bitmap, 811, 1) );
  tf_assert( !GC_bitmap_find(&bitmap, 812, 1) );
  tf_assert( !GC_bitmap_find(&bitmap, 813, 1) );
  tf_assert( !GC_bitmap_find(&bitmap, 811, 0) );
  tf_assert( !GC_bitmap_find(&bitmap, 812, 0) );
  tf_assert( !GC_bitmap_find(&bitmap, 810, 0) );
  
  return 0;
}

DEFINE_TEST(experimental_test)
{
#ifdef GC_USE_BITMAP
  tf_cap_t void * x = tf_malloc(50);
  GC_debug_print_bitmap(GC_state.thread_local_region.tospace_bitmap);
  x = tf_malloc(100);
  GC_debug_print_bitmap(GC_state.thread_local_region.tospace_bitmap);
  tf_collect();
  GC_debug_print_bitmap(GC_state.thread_local_region.tospace_bitmap);
#endif // GC_USE_BITMAP
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
    GC_state.thread_local_region.free = GC_state.thread_local_region.tospace;
    GC_state.thread_local_region.tospace_bitmap->used = 0;
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

  GC_debug_print_region_stats(&GC_state.thread_local_region);
  
  tf_printf("Did %d allocs in %llu%s (%llu%s per alloc)\n",
    nalloc, tf_time_pretty(diff), tf_time_pretty_unit(diff),
    tf_time_pretty(time_per_alloc), tf_time_pretty_unit(time_per_alloc));
  
  return 0;
}