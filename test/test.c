#include <gc_init.h>
#include <gc_malloc.h>
#include <gc_debug.h>
#include <gc_collect.h>
#include <gc_low.h>

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

void
collection_test (void);

int
main (int argc, char **argv)
{
  printf("test: compiled %s\n", __TIME__ " " __DATE__);
  collection_test();
  return 0;
}

void
collection_test (void)
{
  GC_ULL num_alloc = 0x800000,
      sz_alloc = 0x600,
      narray = 0x1;
  
  #define MEM_PRETTY(x) \
  ( \
    (x) < 1000 ? (x) : \
    (x) < 1000000 ? (x) / 1000 : \
    (x) < 1000000000 ? (x) / 1000000 : \
    (x) / 1000000000 \
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
  
  printf("Semi-space size         : 0x%llx (%llu) byte (~%llu%s)\n"
         "Number of allocations   : 0x%llx (%llu)      (~%llu%s)\n"
         "Size of each allocation : 0x%llx (%llu) byte (~%llu%s)\n"
         "Total allocation        : 0x%llx (%llu) byte (~%llu%s)\n"
         "Array entries           : 0x%llx (%llu)      (~%llu%s)\n"
         "Retained contents       : 0x%llx (%llu) byte (~%llu%s)\n",
         (GC_ULL) GC_THREAD_LOCAL_SEMISPACE_SIZE,
         (GC_ULL) GC_THREAD_LOCAL_SEMISPACE_SIZE,
         (GC_ULL) MEM_PRETTY(GC_THREAD_LOCAL_SEMISPACE_SIZE),
         MEM_PRETTY_UNIT(GC_THREAD_LOCAL_SEMISPACE_SIZE),
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
  GC_cap_ptr a[narray];
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
  printf("Time taken: %llu sec\n"
         "Num collections: %llu\n",
         (GC_ULL) (after - before),
         (GC_ULL) GC_state.num_collections);
}
