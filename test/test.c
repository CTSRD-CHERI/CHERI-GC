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
  GC_CAP void * other;
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
  /*X_MACRO(fill_test, "Fill the heap with 512-byte chunks and ensure integrity after collection") \
  X_MACRO(list_test, "Fill the heap with a list and ensure integrity after collection") \
  */X_MACRO(bintree_test, "Create some binary trees and ensure integrity after collection") \
  /*X_MACRO(regroots_test, "Check register roots") \
  X_MACRO(bitmap_test, "Check bitmap operations") \
  /*X_MACRO(experimental_test, "For experiments") \
  X_MACRO(malloc_time_test, "Tests how long GC_malloc takes without collecting") \
  X_MACRO(malloc_time_test_with_collect, "Tests how long GC_malloc takes with collecting")*/ \

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
  { \
    printf("****FATAL: %s:%d: assertion failed: %s\n", \
      __FILE__, __LINE__, #cond); \
    exit(1); \
  } \
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

#define typ int
const int bufsz = 128*sizeof(typ);
const int maxnbufs = 100000;
static GC_CAP typ * bufs[maxnbufs];
DEFINE_TEST(fill_test)
{
  size_t heapsz =
    GC_state.thread_local_region.max_grow_size_after_collection;
  
#ifdef GC_GENERATIONAL
  heapsz = GC_state.old_generation.max_grow_size_after_collection - heapsz - 1000;
#endif

  TEST_ASSERT( heapsz );
  
  heapsz -= heapsz/10;
  TEST_ASSERT( heapsz > 0 );
  
  size_t nbufs = heapsz / bufsz;
  TEST_ASSERT( maxnbufs > nbufs );
  
  TESTF("heapsz: %llu  nbufs: %llu  bufsz: %llu\n",
    (GC_ULL) heapsz, (GC_ULL) nbufs, (GC_ULL) bufsz);
  
  size_t i;
  for (i=0; i<nbufs; i++)
  {
    bufs[i] = GC_INVALID_PTR();
    GC_STORE_CAP(bufs[i], GC_malloc(bufsz));
    
    TEST_ASSERT( (typ*)bufs[i] );
    TEST_ASSERT( GC_cheri_getlen(bufs[i]) == bufsz );
    
    size_t j;
    for (j=0; j<bufsz/sizeof(typ); j++)
    {
      bufs[i][j] = i;
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
  //TEST_ASSERT( heapsz/100 );
  for (i=0; i<10; i++)
  {
    TESTF("allocating %d bytes...\n", (int) (heapsz/20));
    TEST_ASSERT( (void*) GC_malloc(heapsz/20) );
  }
  GC_collect();
  GC_collect();
  
  TESTF("checking buffers\n");

  for (i=0; i<nbufs; i++)
  {
    TEST_ASSERT( GC_cheri_gettag(bufs[i]) );
    if (!GC_IN((void*)bufs[i], GC_state.thread_local_region.tospace))
    {
#ifdef GC_GENERATIONAL
      if (!GC_IN((void*)bufs[i], GC_state.old_generation.tospace))
#endif // GC_GENERATIONAL
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
  }
  
  for (i=0; i<nbufs; i++)
  {
    TEST_ASSERT( (typ*)bufs[i] );
    TEST_ASSERT( GC_cheri_getlen(bufs[i]) == bufsz );
    if (!GC_IN((void*)bufs[i], GC_state.thread_local_region.tospace))
    {
#ifdef GC_GENERATIONAL
      if (!GC_IN((void*)bufs[i], GC_state.old_generation.tospace))
#endif // GC_GENERATIONAL
      {
        printf("NOTE: bufs[i]=0x%llx, i=%d\n", (GC_ULL)(void*)bufs[i], (int) i);
        GC_debug_print_region_stats(&GC_state.thread_local_region);
        TEST_ASSERT(0);
      }
    }
    
    size_t j;
    for (j=0; j<bufsz/sizeof(typ); j++)
    {      
      if ( bufs[i][j] != (typ) i )
      {
        printf("NOTE: i=%d, j=%d\n", (int) i, (int) j);
        GC_debug_memdump((typ*)bufs[i], ((typ*)bufs[i])+bufsz);
      }
      
      TEST_ASSERT( bufs[i][j] == (typ) i );
    }
  }
  
  return 0;
}

DEFINE_TEST(list_test)
{
  // Don't allow an unlimited heap
  TEST_ASSERT( GC_state.thread_local_region.max_grow_size_before_collection );
  TEST_ASSERT( GC_state.thread_local_region.max_grow_size_after_collection );
#ifdef GC_GENERATIONAL
  TEST_ASSERT( GC_state.old_generation.max_grow_size_before_collection );
  TEST_ASSERT( GC_state.old_generation.max_grow_size_after_collection );
#endif // GC_GENERATIONAL
 
  size_t heapsz =
    GC_state.thread_local_region.max_grow_size_after_collection;
  TEST_ASSERT( heapsz );

#ifdef GC_GENERATIONAL
  heapsz += GC_state.old_generation.max_grow_size_after_collection;
#endif
  
  int i;
  GC_CAP node * head = GC_INVALID_PTR();
  for (i=0; ; i++)
  {
    if (i > heapsz/sizeof(node))
    {
      TESTF("FATAL: too many nodes\n");
      GC_debug_dump();
      return 1;
    }
    
    GC_CAP node * p = GC_INVALID_PTR();
    GC_STORE_CAP(p, GC_malloc(sizeof(node)));
    if (!(void*)p) break;
    
    // dummy, should be lost on collection
    void * tmp = (void*) GC_malloc(500);
    if (tmp) memset(tmp, 0, 500);
    
    p->value = i;
    GC_STORE_CAP(p->next, head);
    GC_STORE_CAP(head, p);
    if (!(i % 1000)) TESTF("allocated %d nodes so far\n", i);
  }
  
  i--;
  
  TESTF("allocated %d nodes (each of size %llu)\n", i, (GC_ULL) sizeof(node));
  
  GC_collect();
  GC_collect();
  GC_collect();
  GC_collect();
  GC_collect();
  
  int j;
  GC_CAP node * p = GC_INVALID_PTR();
  GC_STORE_CAP(p, head);
  for (j=0; j<=i; j++)
  {
    TEST_ASSERT( GC_IN(p, GC_state.thread_local_region.tospace)
#ifdef GC_GENERATIONAL
    || GC_IN(p, GC_state.old_generation.tospace)
#endif // GC_GENERATIONAL
      );
    if (!(void*)p)
    {
      printf("failed at j=%d\n", j);
    }
    TEST_ASSERT( (void*)p );
    if (p->value != (i - j))
    {
      printf("at j=%d, the value is %d (0x%llx)\n", j, p->value, (GC_ULL) p->value);
    }
    TEST_ASSERT( p->value == (i - j) );
    GC_STORE_CAP(p, p->next);
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
  GC_CAP bintree * tree = GC_INVALID_PTR();
  GC_STORE_CAP(tree, GC_malloc(sizeof(bintree)));
  //tree = GC_cheri_ptr(GC_low_malloc(sizeof(bintree)), sizeof(bintree));
  if (!(void*)tree) return GC_INVALID_PTR();
  tree->value = bintree_encode_value(depth, value);
  tree->left = GC_INVALID_PTR();
  tree->right = GC_INVALID_PTR();
  
  GC_malloc(50);
  if (depth > 1)
  {
    GC_malloc(50);
    
    GC_STORE_CAP( tree->left, bintree_create(depth-1, 2*value) );
    if (!(void*)tree->left) return GC_INVALID_PTR();
    TEST_ASSERT( GC_cheri_gettag(tree->left) );
    
    TEST_ASSERT( bintree_check(tree->left, depth-1, 2*value) );
    
    GC_malloc(50);
    
    GC_STORE_CAP( tree->right, bintree_create(depth-1, 2*value+1) );
    if (!(void*)tree->right) return GC_INVALID_PTR();
    TEST_ASSERT( GC_cheri_gettag(tree->right) );

    TEST_ASSERT( bintree_check(tree->right, depth-1, 2*value+1) );
    
    GC_malloc(50);
    TEST_ASSERT( bintree_check(tree->right, depth-1, 2*value+1) );
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
  //printf("Bintree checking at depth=%d, value=%d, tree=0x%llx\n", depth, value, (GC_ULL) (void*)tree);
  if (!GC_cheri_gettag(tree))
  {
    printf("bintree_check: no tag (d=0x%x, v=0x%x)\n", depth, value);
    return 0;
  }
  if (!GC_IN((void*)tree, GC_state.thread_local_region.tospace)
#ifdef GC_GENERATIONAL
      && !GC_IN((void*)tree, GC_state.old_generation.tospace)
  #endif // GC_GENERATIONAL
  ) 
  {
    printf("bintree_check: 0x%llx not in young or old generation (d=0x%x, v=0x%x)\n",
      (GC_ULL)(void*)tree, depth, value);
    return 0;
  }
  if (tree->value != bintree_encode_value(depth, value))
  {
    printf("bintree_check: 0x%llx has bad value 0x%x (expected 0x%x) (d=0x%x, v=0x%x)\n",
      (GC_ULL)(void*)tree, tree->value, bintree_encode_value(depth, value), depth, value);
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
      printf("bintree_check: not NULL left\n");
      return 0;
    }
    if ((void*)tree->right)
    {
      printf("bintree_check: not NULL right\n");
      return 0;
    }
    return 1;
  }
}

ATTR_SENSITIVE static void
bintree_print (GC_CAP bintree * tree, int depth)
{
  TEST_ASSERT((void*)tree);
  int val = tree->value; // compiler crashes if print tree->value directly (why?)
  
  // -O2 optimises the above delay away, causing the compiler to crash
  volatile int x = 0;
  val += x;
  
  printf("[0x%llx\n", (GC_ULL) val);
  if (depth>1)
  {
    TEST_ASSERT( (void*)tree->left );
    TEST_ASSERT( (void*)tree->right );
    TEST_ASSERT(GC_IN(
      (void*)tree->left, GC_state.thread_local_region.tospace)
#ifdef GC_GENERATIONAL
      || GC_IN((void*)tree->left, GC_state.old_generation.tospace)
#endif // GC_GENERATIONAL
    );
    TEST_ASSERT(GC_IN(
      (void*)tree->right, GC_state.thread_local_region.tospace)
#ifdef GC_GENERATIONAL
      || GC_IN((void*)tree->right, GC_state.old_generation.tospace)
#endif // GC_GENERATIONAL
    );
    printf(",L=0x%llx(", (GC_ULL)(void*)tree->left);
    bintree_print( tree->left, depth-1);
    printf("),R=0x%llx(", (GC_ULL)(void*)tree->right);
    bintree_print( tree->right, depth-1);
    printf(")");
  }
  else
  {
    if ((void*)tree->left)
    {
      GC_debug_dump(); // DEBUG
      GC_PRINT_CAP(tree->left);
      printf("tree->left is 0x%llx\n", (GC_ULL) tree->left);
    }
    TEST_ASSERT( !(void*)tree->left );
    TEST_ASSERT( !(void*)tree->right );
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
  
  const int max_trees = 10;
  const int tree_depth = 3;
  
  GC_CAP bintree * trees[max_trees];
  
  GC_minor_free();
  GC_major_collect();
  GC_major_collect();
  
  int i;
  for (i=0; i<max_trees; i++)
  {
    //GC_debug_dump();
    TESTF("allocating tree %d\n", i);
    trees[i] = GC_INVALID_PTR();
    GC_STORE_CAP(trees[i], bintree_create(tree_depth, i));
    TESTF("tree %d is 0x%llx\n", i, (GC_ULL) (void*)trees[i]);
    if (!(void*)trees[i]) break;
    bintree_print(trees[i],tree_depth); printf("\n");
    TEST_ASSERT(bintree_check(trees[i], tree_depth, i));
    TESTF("tree %d allocated\n", i);
    GC_debug_dump();
  }
  
  TESTF("allocated %d trees\n", i);
  bintree_print(trees[9], tree_depth);printf("\n");
  printf("done printing tree 9\n");
  GC_debug_dump();
  GC_collect();
  GC_collect();
  GC_collect();
  GC_collect();
  GC_collect();
  TESTF("collection complete\n");
  bintree_print(trees[9], tree_depth);printf("\n");

  int j;
  for (j=0; j<i; j++)
  {
    bintree_print(trees[j], tree_depth); printf("\n");
    TEST_ASSERT( bintree_check(trees[j], tree_depth, j) );
    TESTF("tree %d ok\n", j);
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
  return 
    !GC_IN((void*)x, GC_state.thread_local_region.tospace)
#ifdef GC_GENERATIONAL
    && !GC_IN((void*)x, GC_state.old_generation.tospace)
#endif // GC_GENERATIONAL
  ;
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
  
  
  // Find member objects
  size_t offset = 0;
  size_t len = 0;
  TEST_ASSERT( GC_bitmap_get_container(&bitmap, 810, &offset, &len) );
  TEST_ASSERT( offset == 0 );
  TEST_ASSERT( len == 1);
  TEST_ASSERT( GC_bitmap_get_container(&bitmap, 53, &offset, &len) );
  TEST_ASSERT( offset == 53 );
  TEST_ASSERT( len == 800);
  TEST_ASSERT( GC_bitmap_get_container(&bitmap, 811, &offset, &len) );
  TEST_ASSERT( offset == 0 );
  TEST_ASSERT( len == 1);
  TEST_ASSERT( !GC_bitmap_get_container(&bitmap, 812, &offset, &len) );
  
  return 0;
}

GC_ULL
getbase_delay (GC_CAP void * x)
{
  printf("getbase_delay: got 0x%llx\n", (GC_ULL)(void*)x);
  return (GC_ULL) (void*)x;
}

DEFINE_TEST(experimental_test)
{
  int tmp = 0xDEADCAFE;
  GC_CAP P * cap = GC_malloc(sizeof(P));
  cap->other = GC_INVALID_PTR();
  cap->ptr = GC_cheri_ptr(&tmp, sizeof tmp);;
  GC_CAP void * x = GC_cheri_ptr((void*)&(cap->ptr), GC_cheri_getlen(cap)-((void*)&(cap->ptr)-(void*)cap));
  GC_PRINT_CAP(cap);
  GC_PRINT_CAP(cap->ptr);
  GC_PRINT_CAP(x); // should be equal to base(cap)+0x20
  GC_collect();
  GC_PRINT_CAP(cap);
  GC_PRINT_CAP(cap->ptr);
  GC_PRINT_CAP(x); // with current collector, won't be equal to base(cap)+0x20
  return 0;
}

DEFINE_TEST(malloc_time_test)
{
  size_t szalloc = 50;
  int nalloc = 1000000;
  
  GC_time_t before = GC_time();
  int i;
  for (i=0; i<nalloc; i++)
  {
    GC_state.thread_local_region.free = GC_state.thread_local_region.tospace;
#ifdef GC_USE_BITMAP
    GC_state.thread_local_region.tospace_bitmap->used = 0;
#endif // GC_USE_BITMAP
    GC_CAP void * x = GC_malloc(szalloc);
  }
  GC_time_t after = GC_time();
  
  GC_time_t diff = after - before;
  
  GC_time_t time_per_alloc = ((double)diff)/(double)nalloc;
  
  printf("Did %d allocs in %llu%s (%llu%s per alloc)\n",
    nalloc, GC_TIME_PRETTY(diff), GC_TIME_PRETTY_UNIT(diff),
    GC_TIME_PRETTY(time_per_alloc), GC_TIME_PRETTY_UNIT(time_per_alloc));

  return 0;
}

DEFINE_TEST(malloc_time_test_with_collect)
{
  size_t szalloc = 50;
  int nalloc = 1000000;
  
  GC_time_t before = GC_time();
  int i;
  for (i=0; i<nalloc; i++)
  {
    GC_CAP void * x = GC_malloc(szalloc);
  }
  GC_time_t after = GC_time();
  
  GC_time_t diff = after - before;
  
  GC_time_t time_per_alloc = ((double)diff)/(double)nalloc;

  GC_debug_print_region_stats(&GC_state.thread_local_region);
  
  printf("Did %d allocs in %llu%s (%llu%s per alloc)\n",
    nalloc, GC_TIME_PRETTY(diff), GC_TIME_PRETTY_UNIT(diff),
    GC_TIME_PRETTY(time_per_alloc), GC_TIME_PRETTY_UNIT(time_per_alloc));
  
  return 0;
}