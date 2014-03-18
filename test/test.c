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
void
collection_test2 (void);

int
main (int argc, char **argv)
{
  printf("test: compiled %s\n",
         __TIME__ " " __DATE__);
  collection_test2();
  return 0;
}

struct struct1
{
  __capability void * ptr;
};
void
collection_test2 (void)
{
  GC_init();
  int i;
  __capability struct struct1 * cap;
  int max = 0x12; // ENSURE THAT THIS FORCES OLD_GENERATION_CAP INTO OLD GENERATION
  double last_percentage = 0;
  time_t before, after;
  before = time(NULL);
  __capability struct struct1 * old_generation_cap = GC_malloc(0x100);
  for (i=0; i<max; i++)
  {
    cap = GC_malloc(0x100);
    //printf("old generation free: 0x%llx\n", (GC_ULL) GC_cheri_getlen(GC_state.old_generation.free));
    cap->ptr = GC_cheri_ptr(0x43216789, 0x55881122);
    double percentage = 100.0 * ((double) i) / ((double) max);
    if (percentage > 10+last_percentage)
    {
      printf("%.2f%%\n", percentage);
      last_percentage = percentage;
    }
  }
  after = time(NULL);
  printf("%u sec\n", (unsigned) (after-before));

  // Now old_generation_cap should be in the old generation.
  printf("old_generation_cap (0x%llx) is in old generation?: %d\n",
    (GC_ULL) old_generation_cap,
    GC_IN(old_generation_cap, GC_state.old_generation.tospace));

  // create a young-to-old pointer
  cap->ptr = old_generation_cap;
  // DOESN'T WORK. collector confuses it with forwarding pointer.

  printf("cap: 0x%llx\n", (GC_ULL) cap);
  printf("cap->ptr: 0x%llx\n", (GC_ULL) cap->ptr);
  GC_debug_memdump((void*)cap, ((char*)cap)+sizeof(struct struct1));
  GC_collect(); // do a young generation collection. ENSURE THIS DOESN'T COLLECT THE OLD GENERATION.
  printf("cap: 0x%llx\n", (GC_ULL) cap);
  printf("cap->ptr: 0x%llx\n", (GC_ULL) cap->ptr);
  GC_debug_memdump((void*)cap, ((char*)cap)+sizeof(struct struct1));
  
  GC_debug_print_region_stats(GC_state.thread_local_region);
  GC_debug_print_region_stats(GC_state.old_generation);
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
  
  GC_init();
  GC_ULL semispace_size = GC_cheri_getlen(GC_state.thread_local_region.tospace);
 
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
  
  int num_collections = GC_state.thread_local_region.num_collections;

  printf("Time taken: %llu sec\n"
         "Num collections: %llu\n",
         (GC_ULL) (after - before),
         (GC_ULL) num_collections);
}
