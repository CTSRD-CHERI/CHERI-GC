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
  X_MACRO(fill_test, "Fill the heap with 128-byte chunks and ensure integrity on collection")

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
  const int bufsz = 128;
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
    bufs[i] = GC_malloc(bufsz);
    GC_assert( (char*)bufs[i] );
    GC_assert( GC_cheri_getlen(bufs[i]) == bufsz );
    memset((char*)bufs[i], i, bufsz);
    
    // dummy, should be lost on collection
    GC_assert( (char*) GC_malloc(bufsz*10) );
  }
  
  TESTF("filled %llu buffers\n", (GC_ULL) nbufs);
  
  GC_collect();
  GC_collect();
  GC_collect();
  GC_assert( heapsz/100 );
  for (i=0; i<heapsz/100; i++)
  {
    GC_assert( (char*) GC_malloc(heapsz/10) );
  }
  GC_collect();
  GC_collect();
  GC_collect();
  
  TESTF("checking buffers\n");
  
  for (i=0; i<nbufs; i++)
  {
    GC_assert( (char*)bufs[i] );
    GC_assert( GC_cheri_getlen(bufs[i]) == bufsz );
    size_t j;
    for (j=0; j<bufsz; j++)
    {
      size_t val = ((char*)bufs[i])[j];
      if ( val != i )
      {
        printf("NOTE: i=%d, j=%d\n", (int) i, (int) j);
        GC_debug_memdump((char*)bufs[i], ((char*)bufs[i])+bufsz);
      }
      
      GC_assert( val == i );
    }
  }
  
  return 0;
}
