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
void
collection_test3 (void);
void
collection_test4 (void);
void
collection_test5 (void);
void
collection_test6 (void);

int
main (int argc, char **argv)
{
  printf("test: compiled %s\n",
         __TIME__ " " __DATE__);
  collection_test6();
  return 0;
}

struct struct1
{
  __capability void * ptr;
};

typedef struct node_tag
{
  int value;
  __capability struct node_tag * next;
} node;

void
collection_test6 (void)
{
  int i;
  __capability struct struct1 * old_object = GC_malloc(sizeof(struct struct1));
  
  // force old_object into the old generation
  __capability void * dummy;
  for (i=0; i<10+GC_OLD_GENERATION_SEMISPACE_SIZE/sizeof(struct struct1); i++)
    dummy = GC_malloc(sizeof(struct struct1));
  dummy = GC_cheri_ptr(NULL, 0);
  
  // young_object should get allocated in the young generation
  __capability struct struct1 * young_object = GC_malloc(sizeof(struct struct1));
  
  old_object->ptr = GC_cheri_ptr((void*)0x1234,0x5678);
  GC_PRINT_CAP(old_object);
  GC_PRINT_CAP(young_object);
  GC_PRINT_CAP(old_object->ptr);
}

void
collection_test5 (void)
{
  int i;
  __capability struct struct1 * old_object = GC_malloc(sizeof(struct struct1));
  
  // force old_object into the old generation
  __capability void * dummy;
  for (i=0; i<10+GC_OLD_GENERATION_SEMISPACE_SIZE/sizeof(struct struct1); i++)
    dummy = GC_malloc(sizeof(struct struct1));
  dummy = GC_cheri_ptr(NULL, 0);
  
  // young_object should get allocated in the young generation
  __capability struct struct1 * young_object = GC_malloc(sizeof(struct struct1));

  printf("young object is young? %d\n", (int) GC_IS_YOUNG(young_object));
  GC_STORE_CAP(young_object->ptr, young_object);
  printf("old object is young? %d young_object->ptr is young? %d\n", (int) GC_IS_YOUNG(old_object), (int) GC_IS_YOUNG(young_object->ptr));
  GC_STORE_CAP(young_object->ptr, old_object);
  printf("old object is young? %d young_object->ptr is young? %d\n", (int) GC_IS_YOUNG(old_object), (int) GC_IS_YOUNG(young_object->ptr));
  //young_object->ptr = old_object;
  
  // use this as a root to point to the old object, then make sure that the old
  // object isn't cached in a register or the stack.
  //__capability struct struct1 * young_root = GC_malloc(sizeof(struct struct1));
  //young_root->ptr = old_object;
  
  
  GC_assert( GC_IN(old_object, GC_state.old_generation.tospace) );
  GC_assert( GC_IN(young_object, GC_state.thread_local_region.tospace) );
  //GC_assert( GC_IN(young_root, GC_state.thread_local_region.tospace) );
  
  //GC_debug_print_region_stats(GC_state.thread_local_region);
  //GC_debug_print_region_stats(GC_state.old_generation);
  
  
  // create an old-to-young pointer
  //old_object->ptr = young_object;
  printf("old_object->ptr. tag? %d. young? %d. contained_in_old? %d  0x%llx.\n",
    (int) GC_cheri_gettag(old_object->ptr), (int) GC_IS_YOUNG(old_object->ptr), (int) GC_IS_CONTAINED_IN_OLD(old_object->ptr), (GC_ULL) (GC_cheri_getperm(old_object->ptr)));
  GC_STORE_CAP(old_object->ptr, young_object);
  printf("old_object->ptr. tag? %d. young? %d. contained_in_old? %d  0x%llx.\n",
    (int) GC_cheri_gettag(old_object->ptr), (int) GC_IS_YOUNG(old_object->ptr), (int) GC_IS_CONTAINED_IN_OLD(old_object->ptr), (GC_ULL) (GC_cheri_getperm(old_object->ptr)));
  printf("old_object: 0x%llx\nyoung object: 0x%llx\nold_object->ptr: 0x%llx\nyoung_object->ptr: 0x%llx\n",
    (GC_ULL) old_object, (GC_ULL) young_object, (GC_ULL) old_object->ptr, (GC_ULL) young_object->ptr);
  
  // (try to) ensure the old object isn't cached in a register or the stack
  void * old_object_saved = GC_cheri_getbase(old_object);
  old_object = GC_cheri_ptr(NULL, 0);
  
  GC_collect();
  
  // observe if old_object->ptr has been changed
  old_object = GC_cheri_ptr(old_object_saved, sizeof(struct struct1));
  printf("old_object: 0x%llx\nyoung object: 0x%llx\nold_object->ptr: 0x%llx\nyoung_object->ptr: 0x%llx\n",
    (GC_ULL) old_object, (GC_ULL) young_object, (GC_ULL) old_object->ptr, (GC_ULL) young_object->ptr);
}


#include <signal.h>
void sigsegv_handler (int p)
{
  printf("Getting cause.\n");
  GC_ULL cause = cheri_getcause();
  printf("Done!\n");
  exit(0);
}
void
collection_test4 (void)
{
  //signal(SIGSEGV, sigsegv_handler);
  signal(SIGPROT, sigsegv_handler);
  int val = 12345;
  __capability int * cap = GC_cheri_ptr(&val, sizeof (int));
  cap = GC_cheri_andperm(cap, ~CHERI_PERM_LOAD);
  printf("Dereferencing capability (t=%d).\n", (int) GC_cheri_gettag(cap));
  printf("Value: %d\n", *cap);
}

void
collection_test3 (void)
{
  __capability node * a, * b;
  GC_init();
  a = GC_malloc(sizeof(node));
  b = GC_malloc(sizeof(node));
  a->value = 0x11223344;
  b->value = 0x55667788;
  a->next = b;
  b->next = a; 
  printf("b: 0x%llx\n", (GC_ULL) b);
  printf("a: 0x%llx\n", (GC_ULL) a);
  printf("a->value: 0x%llx\n", (GC_ULL) ((node*)a)->value);
  printf("a->next: 0x%llx\n", (GC_ULL) ((node*)a)->next);
  printf("a->next->value: 0x%llx\n", (GC_ULL) ((node*) (((node*)a)->next)) -> value);
  printf("a->next->next: 0x%llx\n", (GC_ULL) ((node*) (((node*)a)->next)) -> next );
  GC_collect();
  a->value = 0xDEADCAFE;
  b->value = 0xBEEFF00D;
  printf("b: 0x%llx\n", (GC_ULL) b);
  printf("a: 0x%llx\n", (GC_ULL) a);
  printf("a->value: 0x%llx\n", (GC_ULL) ((node*)a)->value);
  printf("a->next: 0x%llx\n", (GC_ULL) ((node*)a)->next);
  printf("a->next->value: 0x%llx\n", (GC_ULL) ((node*) (((node*)a)->next)) -> value);
  printf("a->next->next: 0x%llx\n", (GC_ULL) ((node*) (((node*)a)->next)) -> next );
}

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
    cap->ptr = GC_cheri_ptr((void*)0x43216789, 0x55881122);
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
  // WORKS now that we identify forwarding pointers separately.
  cap->ptr = old_generation_cap;

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
