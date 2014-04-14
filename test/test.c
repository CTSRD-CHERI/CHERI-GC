#include <gc_init.h>
#include <gc_malloc.h>
#include <gc_debug.h>
#include <gc_collect.h>
#include <gc_low.h>
#include <gc_config.h>
#include <gc_bitmap.h>

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

//#define ATTR_SENSITIVE __attribute__((sensitive))
#define ATTR_SENSITIVE

typedef struct
{
  GC_CAP void * ptr;
} P;

typedef struct node_tag
{
  GC_CAP struct node_tag * next;
  int value;
} node;

typedef struct bintree_tag
{
  GC_CAP struct bintree_tag * left, * right;
  int value;
} bintree;

// ----------------------------------------------------------------------------
// ------ To create a new test, just add it to the TESTS list and -------------
// ------ define it with DEFINE_TEST. Tests should return 0 on ----------------
// ------ success and non-zero on failure. main() will call all ---------------
// ------ the tests in turn. --------------------------------------------------
// ----------------------------------------------------------------------------

#define TESTS \
  X_MACRO(fill_test, "Fill the heap with 512-byte chunks and ensure integrity after collection") \
  X_MACRO(list_test, "Fill the heap with a list and ensure integrity after collection") \
  X_MACRO(bintree_test, "Create some binary trees and ensure integrity after collection") \
  X_MACRO(regroots_test, "Check register roots") \
  X_MACRO(bitmap_test, "Check bitmap operations") \
  X_MACRO(experimental_test, "For experiments") \

#define DECLARE_TEST(test,descr) \
ATTR_SENSITIVE int \
test (const char * TEST__test_name);

#define DEFINE_TEST(test) \
ATTR_SENSITIVE int \
test (const char * TEST__test_name)

#define DO_TEST(test,descr) \
do { \
  printf("----------Begin test " #test "----------\n"); \
  printf(#test ": " descr "\n"); \
  GC_time_t before = GC_time(); \
  int e = test(#test); \
  if (e) \
  { \
    printf("#########FATAL: TEST FAILED: " #test " (error code %d)\n", e); \
    exit(1); \
  } \
  else \
  { \
    GC_time_t after = GC_time(); \
    printf("----------Finished test " #test " (took %llu%s)----------\n", \
      GC_TIME_PRETTY(GC_time_diff(after, before)), \
      GC_TIME_PRETTY_UNIT(GC_time_diff(after, before))); \
  } \
} while(0); \

#define TESTF(...) \
do { \
  printf("[%s]  ", TEST__test_name); \
  printf(__VA_ARGS__); \
} while (0)

#define TEST_ASSERT(cond) \
do { \
  if (!(cond)) \
    printf("****FATAL: %s:%d: assertion failed: %s\n", \
      __FILE__, __LINE__, #cond); \
} while (0)

#define X_MACRO DECLARE_TEST
  TESTS
#undef X_MACRO

int
main (int argc, char **argv)
{
  int rc = GC_init();
  if (rc)
  {
    printf("GC_init failed with %d\n", rc);
    return 1;
  }
  GC_debug_dump();
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
  GC_CAP char * bufs[maxnbufs/bufsz];
  
  size_t heapsz =
    GC_state.thread_local_region.max_grow_size_after_collection;
  TEST_ASSERT( heapsz );
  
  heapsz -= heapsz/10;
  TEST_ASSERT( heapsz > 0 );
  
  size_t nbufs = heapsz / bufsz;
  TEST_ASSERT( maxnbufs > nbufs );
  
  TESTF("nbufs: %llu  bufsz: %llu\n", (GC_ULL) nbufs, (GC_ULL) bufsz);
  
  size_t i;
  for (i=0; i<nbufs; i++)
  {
    bufs[i] = GC_INVALID_PTR;
    GC_STORE_CAP(bufs[i], GC_malloc(bufsz));
    
    TEST_ASSERT( (typ*)bufs[i] );
    TEST_ASSERT( GC_cheri_getlen(bufs[i]) == bufsz );
    
    size_t j;
    for (j=0; j<bufsz/sizeof(typ); j++)
    {
      ((typ*)bufs[i])[j] = i;
    }
    
    // dummy, should be lost on collection
    typ * tmp = (typ*) GC_malloc(bufsz*2);
    TEST_ASSERT( tmp );
    memset(tmp, 0, bufsz*2);
  }
  
  TESTF("filled %llu buffers\n", (GC_ULL) nbufs);
  
  GC_collect();
  GC_collect();
  GC_collect();
  TEST_ASSERT( heapsz/100 );
  for (i=0; i<heapsz/100; i++)
  {
    TEST_ASSERT( (void*) GC_malloc(heapsz/10) );
  }
  GC_collect();
  GC_collect();
  
  TESTF("checking buffers\n");

  for (i=0; i<nbufs; i++)
  {
    TEST_ASSERT( GC_cheri_gettag(bufs[i]) );
    if (!GC_IN((void*)bufs[i], GC_state.thread_local_region.tospace))
    {
      printf(
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
    TEST_ASSERT( (typ*)bufs[i] );
    TEST_ASSERT( GC_cheri_getlen(bufs[i]) == bufsz );
    if (!GC_IN((void*)bufs[i], GC_state.thread_local_region.tospace))
    {
      printf("NOTE: bufs[i]=0x%llx, i=%d\n", (GC_ULL)(void*)bufs[i], (int) i);
      GC_debug_print_region_stats(&GC_state.thread_local_region);
    }
    TEST_ASSERT( GC_IN((void*)bufs[i], GC_state.thread_local_region.tospace) );
    
    size_t j;
    for (j=0; j<bufsz/sizeof(typ); j++)
    {      
      if ( ((typ*)bufs[i])[j] != (typ) i )
      {
        printf("NOTE: i=%d, j=%d\n", (int) i, (int) j);
        GC_debug_memdump((typ*)bufs[i], ((typ*)bufs[i])+bufsz);
      }
      
      TEST_ASSERT( ((typ*)bufs[i])[j] == (typ) i );
    }
  }
  
  return 0;
}

DEFINE_TEST(list_test)
{
  // Don't allow an unlimited heap
  TEST_ASSERT( GC_state.thread_local_region.max_grow_size_before_collection );
  TEST_ASSERT( GC_state.thread_local_region.max_grow_size_after_collection );
 
  size_t heapsz =
    GC_state.thread_local_region.max_grow_size_after_collection;
  TEST_ASSERT( heapsz );
  
  int i;
  GC_CAP node * head = GC_INVALID_PTR;
  for (i=0; ; i++)
  {
    if (i > heapsz/sizeof(node))
    {
      TESTF("FATAL: too many nodes!\n");
      GC_debug_print_region_stats(&GC_state.thread_local_region);
      return 1;
    }
    
    GC_CAP node * p = GC_INVALID_PTR;
    GC_STORE_CAP(p, GC_malloc(sizeof(node)));
    if (!(void*)p) break;
    
    // dummy, should be lost on collection
    void * tmp = (void*) GC_malloc(500);
    if (tmp) memset(tmp, 0, 500);
    
    ((node*)p)->value = i;
    GC_STORE_CAP(((node*)p)->next, head);
    GC_STORE_CAP(head, p);
  }
  
  i--;
  
  TESTF("allocated %d nodes (each of size %llu)\n", i, (GC_ULL) sizeof(node));
  
  GC_collect();
  GC_collect();
  GC_collect();
  GC_collect();
  GC_collect();
  
  int j;
  GC_CAP node * p = GC_INVALID_PTR;
  GC_STORE_CAP(p, head);
  for (j=0; j<=i; j++)
  {
    TEST_ASSERT( GC_IN(p, GC_state.thread_local_region.tospace) );
    if (!(void*)p)
    {
      printf("failed at j=%d\n", j);
    }
    TEST_ASSERT( (void*)p );
    if (((node*)p)->value != (i - j))
    {
      printf("at j=%d, the value is %d (0x%llx)\n", j, ((node*)p)->value, (GC_ULL) ((node*)p)->value);
    }
    TEST_ASSERT( ((node*)p)->value == (i - j) );
    GC_STORE_CAP(p, ((node*)p)->next);
  }
 
  TEST_ASSERT( !(void*)p );
  
  TESTF("checked %d nodes\n", i);
  
  return 0;
}

ATTR_SENSITIVE static int
bintree_encode_value (int depth, int value)
{
  return ((depth&0xFFFF) << 16) | (value&0xFFFF);
}

ATTR_SENSITIVE static void
bintree_print (GC_CAP bintree * tree, int depth);
ATTR_SENSITIVE static int
bintree_check (GC_CAP bintree * tree, int depth, int value);

ATTR_SENSITIVE static GC_CAP bintree *
bintree_create (int depth, int value)
{
  GC_CAP bintree * tree = GC_INVALID_PTR;
  GC_STORE_CAP(tree, GC_malloc(sizeof(bintree)));
  //tree = GC_cheri_ptr(GC_low_malloc(sizeof(bintree)), sizeof(bintree));
  if (!(void*)tree) return GC_INVALID_PTR;
  ((bintree*)tree)->value = bintree_encode_value(depth, value);
  ((bintree*)tree)->left = GC_INVALID_PTR;
  ((bintree*)tree)->right = GC_INVALID_PTR;
  
  printf("The tree root is at 0x%llx\n", (GC_ULL) &tree);
  GC_malloc(50);
  if (depth > 1)
  {
    GC_malloc(50);
    
    GC_STORE_CAP( ((bintree*)tree)->left, bintree_create(depth-1, 2*value) );
    if (!(void*)((bintree*)tree)->left) return GC_INVALID_PTR;
    TEST_ASSERT( GC_cheri_gettag(((bintree*)tree)->left) );
    
    TEST_ASSERT( bintree_check(((bintree*)tree)->left, depth-1, 2*value) );
    
    GC_malloc(50);
    
    GC_STORE_CAP( ((bintree*)tree)->right, bintree_create(depth-1, 2*value+1) );
    if (!(void*)((bintree*)tree)->right) return GC_INVALID_PTR;
    TEST_ASSERT( GC_cheri_gettag(((bintree*)tree)->right) );

    TEST_ASSERT( bintree_check(((bintree*)tree)->right, depth-1, 2*value+1) );
    
    GC_malloc(50);
    TEST_ASSERT( bintree_check(((bintree*)tree)->right, depth-1, 2*value+1) );
  }
  GC_malloc(50);
  if (!bintree_check(tree, depth, value))
  {
    printf("At depth %d, value %d, bintree_check failed\n", depth, value);
  }
  TEST_ASSERT( bintree_check(tree, depth, value) );
  return tree;
}

ATTR_SENSITIVE static int
bintree_check (GC_CAP bintree * tree, int depth, int value)
{
  printf("Bintree checking at depth=%d, value=%d, tree=0x%llx\n", depth, value, (GC_ULL) (void*)tree);
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

ATTR_SENSITIVE static void
bintree_print (GC_CAP bintree * tree, int depth)
{
  TEST_ASSERT((void*)tree);
  printf("[0x%llx\n", (GC_ULL) ((bintree*)tree)->value);
  if (depth>1)
  {
    TEST_ASSERT( (void*)((bintree*)tree)->left );
    TEST_ASSERT( (void*)((bintree*)tree)->right );
    TEST_ASSERT(GC_IN(
      (void*)((bintree*)tree)->left, GC_state.thread_local_region.tospace 
    ));
    TEST_ASSERT(GC_IN(
      (void*)((bintree*)tree)->right, GC_state.thread_local_region.tospace 
    ));
    printf(",L(");
    bintree_print( ((bintree*)tree)->left, depth-1);
    printf("),R(");
    bintree_print( ((bintree*)tree)->right, depth-1);
    printf(")");
  }
  else
  {
    TEST_ASSERT( !(void*)((bintree*)tree)->left );
    TEST_ASSERT( !(void*)((bintree*)tree)->right );
  }
  printf("]");
}

DEFINE_TEST(bintree_test)
{
  // Don't allow an unlimited heap
  TEST_ASSERT( GC_state.thread_local_region.max_grow_size_before_collection );
  TEST_ASSERT( GC_state.thread_local_region.max_grow_size_after_collection );
 
  size_t heapsz =
    GC_state.thread_local_region.max_grow_size_after_collection;
  TEST_ASSERT( heapsz );
  
  const int max_trees = 1;
  const int tree_depth = 3;
  
  GC_CAP bintree * trees[max_trees];
  
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
  
  GC_collect();
  GC_collect();
  GC_collect();
  GC_collect();
  GC_collect();

  int j;
  for (j=0; j<i; j++)
  {
    TEST_ASSERT( bintree_check(trees[j], tree_depth, j) );
  }
  
  TESTF("checked %d trees\n", j);
  
  return 0;
}

DEFINE_TEST(regroots_test)
{
  GC_CAP void * x = GC_malloc(50);
  printf("before collecting, x.base = 0x%llx\n", (GC_ULL)(void*)x);
  GC_PRINT_CAP(GC_state.thread_local_region.tospace);
  GC_collect();
  printf("after collecting, x.base = 0x%llx\n", (GC_ULL)(void*)x);
  GC_PRINT_CAP(GC_state.thread_local_region.tospace);
  return !GC_IN((void*)x, GC_state.thread_local_region.tospace);
}

DEFINE_TEST(bitmap_test)
{
  struct GC_bitmap bitmap;
  size_t sz = 811;
  size_t i;
  
  TEST_ASSERT( !GC_init_bitmap(&bitmap, sz) );
  TEST_ASSERT( bitmap.size == sz );
  TEST_ASSERT( bitmap.used == 0 );
  
  for (i=0; i<bitmap.size; i++)
  {
    int bit = GC_BITMAP_GET(&bitmap, i);
    TEST_ASSERT( !bit );
    printf("%d", bit);
  }
  printf("\n");
  
  GC_bitmap_allocate(&bitmap, 23);
  GC_bitmap_allocate(&bitmap, 42);
  GC_bitmap_allocate(&bitmap, 19);
  GC_bitmap_allocate(&bitmap, 7);

  for (i=0; i<bitmap.size; i++)
  {
    int bit = GC_BITMAP_GET(&bitmap, i);
    if ((i == 0) || (i == 23) || (i==23+42) || (i==23+42+19))
      TEST_ASSERT( bit );
    else
      TEST_ASSERT( !bit );
    printf("%d", bit);
  }
  printf("\n");
  
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 0, 10) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 0, 0) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 0, 1) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 0, 22) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 0, 24) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 0, 0) );
  TEST_ASSERT( GC_bitmap_find(&bitmap, 0, 23) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 23, 23) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 23, 0) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 23, 41) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 23, 1) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 23, 43) );
  TEST_ASSERT( GC_bitmap_find(&bitmap, 23, 42) );
  TEST_ASSERT( GC_bitmap_find(&bitmap, 23+42, 19) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 23+42+19, 6) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 23+42+19, 99) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 23+42+19, 8) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 23+42+19, 0) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 23+42+19, 1) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 23, 19999999) );
  TEST_ASSERT( GC_bitmap_find(&bitmap, 23+42+19, 7) );
  
  
  // clear
  GC_bitmap_clr(&bitmap);
  TEST_ASSERT( bitmap.used == 0 );
  for (i=0; i<bitmap.size; i++)
  {
    int bit = GC_BITMAP_GET(&bitmap, i);
    TEST_ASSERT( !bit );
  }
  
  // try to resize the bitmap
  GC_bitmap_allocate(&bitmap, 800);
  GC_bitmap_allocate(&bitmap, 5);
  GC_bitmap_allocate(&bitmap, 5);
  GC_bitmap_allocate(&bitmap, 1);
  for (i=0; i<bitmap.size; i++)
  {
    int bit = GC_BITMAP_GET(&bitmap, i);
    printf("%d", bit);
  }
  printf("\n");
  GC_grow_bitmap(&bitmap, 812);
  
  TEST_ASSERT( GC_bitmap_find(&bitmap, 810, 1) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 811, 1) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 812, 1) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 813, 1) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 811, 0) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 812, 0) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 810, 0) );
  
  GC_bitmap_allocate(&bitmap, 1);
  for (i=0; i<bitmap.size; i++)
  {
    int bit = GC_BITMAP_GET(&bitmap, i);
    printf("%d", bit);
  }
  printf("\n");
  TEST_ASSERT( GC_bitmap_find(&bitmap, 810, 1) );
  TEST_ASSERT( GC_bitmap_find(&bitmap, 811, 1) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 812, 1) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 813, 1) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 811, 0) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 812, 0) );
  TEST_ASSERT( !GC_bitmap_find(&bitmap, 810, 0) );
  
  return 0;
}

DEFINE_TEST(experimental_test)
{
#ifdef GC_USE_BITMAP
  GC_CAP void * x = GC_malloc(50);
  GC_debug_print_bitmap(GC_state.thread_local_region.tospace_bitmap);
  x = GC_malloc(100);
  GC_debug_print_bitmap(GC_state.thread_local_region.tospace_bitmap);
  GC_collect();
  GC_debug_print_bitmap(GC_state.thread_local_region.tospace_bitmap);
#endif // GC_USE_BITMAP
  return 0;
}
