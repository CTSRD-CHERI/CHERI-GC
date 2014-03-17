#ifdef GC_CHERI

#include <gc_init.h>
#include <gc_malloc.h>
#include <gc_debug.h>
#include <gc_collect.h>
#include <gc_low.h>

#include <machine/cheri.h>
#include <machine/cheric.h>

#else // GC_CHERI

// Boehm

#define GC_ULL  unsigned long long
typedef void * GC_cap_ptr;
#define __capability
#define GC_ALIGN_32(x,typ)  ((typ)(x))
void __LOCK_MALLOC ()
{
}
void __UNLOCK_MALLOC ()
{
}

#endif // GC_CHERI

#include <stdio.h>
#include <string.h>
#include <time.h>

void
collection_test (void);

int
main (int argc, char **argv)
{
  printf("test: compiled %s using %s\n",
         __TIME__ " " __DATE__,
#ifdef GC_CHERI
         "GC2/CHERI"
#else // GC_CHERI
         "Boehm"
#endif // GC_CHERI
         );
  collection_test();
  return 0;
}

void
collection_test (void)
{
  GC_ULL num_alloc = 1800000,
      sz_alloc = 250,
      narray = 5;
  
  #define MEM_PRETTY(x) \
  ( \
    (x) < 1000 ? (x) : \
    (x) < 1000000 ? ((x)+1000/2) / 1000 : \
    (x) < 1000000000 ? ((x)+1000000/2) / 1000000 : \
    ((x)+1000000000/2) / 1000000000 \
  )
  #define MEM_PRETTY_UNIT(x) \
  ( \
    (x) < 1000 ? "b" : \
    (x) < 1000000 ? "kB" : \
    (x) < 1000000000 ? "MB" : \
    "GB" \
  )
  #define NUM_PRETTY MEM_PRETTY
  #define NUM_PRETTY_UNIT(x) \
  ( \
    (x) < 1000 ? "" : \
    (x) < 1000000 ? "k" : \
    (x) < 1000000000 ? "M" : \
    "G" \
  )
  
#ifdef GC_CHERI
  GC_init();
  GC_ULL semispace_size = GC_cheri_getlen(GC_state.thread_local_region.tospace);
#else // GC_CHERI
  GC_ULL semispace_size = GC_get_heap_size();
#endif // GC_CHERI
 
  printf("Semi-space size         : 0x%llx (%llu) byte (~%llu%s)\n"
         "Number of allocations   : 0x%llx (%llu)      (~%llu%s)\n"
         "Size of each allocation : 0x%llx (%llu) byte (~%llu%s)\n"
         "Total allocation        : 0x%llx (%llu) byte (~%llu%s)\n"
         "Array entries           : 0x%llx (%llu)      (~%llu%s)\n"
         "Retained contents       : 0x%llx (%llu) byte (~%llu%s)\n",
         (GC_ULL) semispace_size,
         (GC_ULL) semispace_size,
         (GC_ULL) MEM_PRETTY(semispace_size),
         MEM_PRETTY_UNIT(semispace_size),
         (GC_ULL) num_alloc,
         (GC_ULL) num_alloc,
         (GC_ULL) NUM_PRETTY(num_alloc),
         NUM_PRETTY_UNIT(num_alloc),
         (GC_ULL) sz_alloc,
         (GC_ULL) sz_alloc,
         (GC_ULL) MEM_PRETTY(sz_alloc),
         MEM_PRETTY_UNIT(sz_alloc),
         (GC_ULL) (num_alloc * sz_alloc),
         (GC_ULL) (num_alloc * sz_alloc),
         (GC_ULL) MEM_PRETTY(num_alloc * sz_alloc),
         MEM_PRETTY_UNIT(num_alloc * sz_alloc),
         (GC_ULL) narray,
         (GC_ULL) narray,
         (GC_ULL) NUM_PRETTY(narray),
         NUM_PRETTY_UNIT(narray),
         (GC_ULL) (narray * sz_alloc),
         (GC_ULL) (narray * sz_alloc),
         (GC_ULL) MEM_PRETTY(narray * sz_alloc),
         MEM_PRETTY_UNIT(narray * sz_alloc)
        );

  int i;
  GC_cap_ptr a_misaligned[narray];
  GC_cap_ptr * a = a_misaligned;
  a = GC_ALIGN_32(a, GC_cap_ptr *);
  time_t before, after;
  before = time(NULL);
  for (i=0; i<num_alloc; i++)
  {
    a[i % narray] = GC_malloc(sz_alloc);
    if (a[i % narray] == NULL)
    {
      printf("Out of memory! i=%d\n", i);
      break;
    }
  }
  after = time(NULL);
  
#ifdef GC_CHERI
  int num_collections = GC_state.num_collections;
#else // GC_CHERI
  int num_collections = 0;
#endif // GC_CHERI
  
  printf("Time taken: %llu sec\n"
         "Num collections: %llu\n",
         (GC_ULL) (after - before),
         (GC_ULL) num_collections);

#ifdef GC_CHERI
#else // GC_CHERI
  // Boehm
  printf("Boehm heap size now: 0x%llx\n", GC_get_heap_size());
#endif // GC_CHERI
  
}
