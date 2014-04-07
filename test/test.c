#include <gc_init.h>
#include <gc_malloc.h>
#include <gc_debug.h>
#include <gc_collect.h>
#include <gc_low.h>
#include <gc_config.h>

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#define ATTR_SENSITIVE __attribute__((sensitive))
//#define ATTR_SENSITIVE

typedef struct
{
  GC_CAP void * ptr;
} P;

typedef struct node_tag
{
  int value;
  __capability struct node_tag * next;
} node;

// ----------------------------------------------------------------------------
// ------ To create a new test, just add it to the TESTS list and -------------
// ------ define it with DEFINE_TEST. Tests should return 0 on ----------------
// ------ success and non-zero on failure. main() will call all ---------------
// ------ the tests in turn. --------------------------------------------------
// ----------------------------------------------------------------------------

#define TESTS \
  X_MACRO(fill_test, "Fill the heap with 512-byte chunks and ensure integrity after collection") \
  X_MACRO(list_test, "Fill the heap with a list and ensure integrity after collection") \

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
  int e = test(#test); \
  if (e) \
  { \
    printf("#########FATAL: TEST FAILED: " #test " (error code %d)\n", e); \
    exit(1); \
  } \
  else \
  { \
    printf("----------Finished test " #test "----------\n"); \
  } \
} while(0); \

#define TESTF(...) \
do { \
  printf("[%s]  ", TEST__test_name); \
  printf(__VA_ARGS__); \
} while (0)

#define X_MACRO DECLARE_TEST
  TESTS
#undef X_MACRO

int
main (int argc, char **argv)
{
  GC_assert( !GC_init() );
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
  GC_assert( heapsz );
  
  heapsz -= heapsz/10;
  GC_assert( heapsz > 0 );
  
  size_t nbufs = heapsz / bufsz;
  GC_assert( maxnbufs > nbufs );
  
  TESTF("nbufs: %llu  bufsz: %llu\n", (GC_ULL) nbufs, (GC_ULL) bufsz);
  
  size_t i;
  for (i=0; i<nbufs; i++)
  {
    bufs[i] = GC_INVALID_PTR;
    GC_STORE_CAP(bufs[i], GC_malloc(bufsz));
    
    GC_assert( (typ*)bufs[i] );
    GC_assert( GC_cheri_getlen(bufs[i]) == bufsz );
    
    size_t j;
    for (j=0; j<bufsz/sizeof(typ); j++)
    {
      ((typ*)bufs[i])[j] = i;
    }
    
    // dummy, should be lost on collection
    typ * tmp = (typ*) GC_malloc(bufsz*2);
    GC_assert( tmp );
    memset(tmp, 0, bufsz*2);
  }
  
  TESTF("filled %llu buffers\n", (GC_ULL) nbufs);
  
  GC_collect();
  GC_collect();
  GC_collect();
  GC_assert( heapsz/100 );
  for (i=0; i<heapsz/100; i++)
  {
    GC_assert( (void*) GC_malloc(heapsz/10) );
  }
  GC_collect();
  GC_collect();
  
  TESTF("checking buffers\n");
  
  for (i=0; i<nbufs; i++)
  {
    GC_assert( (typ*)bufs[i] );
    GC_assert( GC_cheri_getlen(bufs[i]) == bufsz );
    size_t j;
    for (j=0; j<bufsz/sizeof(typ); j++)
    {
      GC_assert( GC_IN((void*)bufs[i], GC_state.thread_local_region.tospace) );
      
      if ( ((typ*)bufs[i])[j] != (typ) i )
      {
        printf("NOTE: i=%d, j=%d\n", (int) i, (int) j);
        GC_debug_memdump((typ*)bufs[i], ((typ*)bufs[i])+bufsz);
      }
      
      GC_assert( ((typ*)bufs[i])[j] == (typ) i );
    }
  }
  
  return 0;
}

DEFINE_TEST(list_test)
{
  // Don't allow an unlimited heap
  GC_assert( GC_state.thread_local_region.max_grow_size_before_collection );
  GC_assert( GC_state.thread_local_region.max_grow_size_after_collection );
 
  size_t heapsz =
    GC_state.thread_local_region.max_grow_size_after_collection;
  GC_assert( heapsz );
  
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
    GC_assert( GC_IN(p, GC_state.thread_local_region.tospace) );
    if (!(void*)p)
    {
      printf("failed at j=%d\n", j);
    }
    GC_assert( (void*)p );
    if (((node*)p)->value != (i - j))
    {
      printf("at j=%d, the value is %d (0x%llx)\n", j, ((node*)p)->value, (GC_ULL) ((node*)p)->value);
    }
    GC_assert( ((node*)p)->value == (i - j) );
    GC_STORE_CAP(p, ((node*)p)->next);
  }
 
  GC_assert( !(void*)p );
  
  TESTF("checked %d nodes\n", i);
  
  return 0;
}