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
collection_test (void);
void
rebase_test (void);
void
grow_test (void);
void
tracking_test (void);
void
remset_test (void);
void
low_realloc_test (void);
void
realloc_preserves_caps_test (void);

int
main (int argc, char **argv)
{
  //remset_test();
  //tracking_test();
  //rebase_test();
  //collection_test();
  //grow_test();
  //low_realloc_test();
  realloc_preserves_caps_test();
  return 0;
}

void
realloc_preserves_caps_test (void)
{
  GC_init();
  struct struct1 * x = GC_low_malloc(1000);
  x->ptr = GC_cheri_ptr((void*) 0x1234, 0x5678);
  printf("x->ptr tag: %d\n", GC_cheri_gettag(x->ptr));
  x = GC_low_realloc(x, 20000);
  printf("x->ptr tag: %d\n", GC_cheri_gettag(x->ptr));
}

void
low_realloc_test (void)
{
  int sz = 10;
  void * ptr = GC_low_malloc(sz);
  memset(ptr, 0x88, sz);
  GC_debug_memdump(ptr, ptr+sz-1);
  ptr = GC_low_realloc(ptr, sz*2);
  GC_debug_memdump(ptr, ptr+sz*2-1);
}

void
remset_test (void)
{
#ifdef GC_GENERATIONAL
#if (GC_OY_STORE_DEFAULT == GC_OY_STORE_REMEMBERED_SET)
  GC_init();
  GC_remembered_set_add(&GC_state.thread_local_region.remset, (void *) 0x123400);
  GC_remembered_set_add(&GC_state.thread_local_region.remset, (void *) 0x567800);
  GC_remembered_set_add(&GC_state.thread_local_region.remset, (void *) 0x999900);
  GC_remembered_set_add(&GC_state.thread_local_region.remset, (void *) 0x888800);
  GC_remembered_set_add(&GC_state.thread_local_region.remset, (void *) 0x777700);
  size_t i;
  for (i=0; i<GC_state.thread_local_region.remset.nroots; i++)
  {
    printf("[%d] 0x%llx\n", (int) i, (GC_ULL) GC_state.thread_local_region.remset.roots[i]);
  }
  printf("size: %d\n", (int) GC_state.thread_local_region.remset.size);
#endif // GC_OY_STORE_DEFAULT
#endif // GC_GENERATIONAL
}

void
tracking_test (void)
{
  GC_init();
  GC_CAP void * obj = GC_malloc(100);
  if (GC_debug_track_allocated(obj, "test object"))
  {
    printf("error: couldn't start tracking object\n");
  }
  collection_test();
}

void
grow_test (void)
{
  GC_init();
  int i;
  for (i=0; i<10000; i++)
  {
    GC_cap_memset(GC_malloc(13), 0xAA);
    printf("%d\n", i);
  }
  printf("exiting\n");
  return;
}

void
rebase_test (void)
{
  GC_cap_ptr cap = GC_cheri_ptr((void*)0x1234, 0x5678);
  GC_cap_ptr cap3 = cap;
  GC_PRINT_CAP(cap);
  GC_cap_ptr cap2[100];
  int i;
  for (i=0; i<100; i++)
    cap2[i] = cap;
  
  void * old_base = (void*) 0x1000;
  size_t old_size = 0xABC;
  void * new_base = (void*) 0x9001;
  
  GC_init();
  void * stack_top = NULL;
  GC_GET_STACK_PTR(stack_top);
  GC_assert(stack_top <= GC_state.stack_bottom);
  GC_rebase(stack_top, GC_state.stack_bottom,
            old_base, old_size, new_base);
  
  GC_PRINT_CAP(cap);
}

void
collection_test (void)
{
  #define NALLOC    100000      // 100,000 5k allocations = 500,000 k = 500 MB
  #define NSTORE    50          // 50*5k = 250k stored
  #define NBYTES    5
  #define LLSTORE   20         // the long-lived store
  #define LLBYTES   500
  int i;
  GC_cap_ptr arr_unaligned[NSTORE+1];
  GC_cap_ptr arrll_unaligned[LLSTORE+1];
  GC_cap_ptr * arr = &arr_unaligned[0];
  GC_ALIGN_32(arr, GC_cap_ptr *);
  GC_cap_ptr * arrll = &arrll_unaligned[0];
  GC_ALIGN_32(arrll, GC_cap_ptr *);
  GC_init();
  time_t start = time(NULL);
    printf("arrll runs from 0x%llx to 0x%llx\n", (GC_ULL) arrll, (GC_ULL) &arrll[LLSTORE-1]);
    printf("arr runs from 0x%llx to 0x%llx\n", (GC_ULL) arr, (GC_ULL) &arr[NSTORE-1]);
  for (i=0; i<LLSTORE; i++)
  {
    arrll[i] = GC_malloc(LLBYTES);
    if (!(void*)arrll[i])
      {printf("ERROR: LL oom %d\n", i);break;}
  }
  for (i=0; i<NALLOC; i++)
  {
    arr[i%NSTORE] = GC_malloc(NBYTES);
    if (!(void*)arr[i%NSTORE])
      {printf("ERROR: oom %d\n", i);break;}
  }
  time_t end = time(NULL);
  GC_debug_print_region_stats(&GC_state.thread_local_region);
#ifdef GC_GENERATIONAL
  GC_debug_print_region_stats(&GC_state.old_generation);
#endif // GC_GENERATIONAL
  printf("total time: %d sec\n", (int) (end - start));
}

/*void
collection_test (void)
{
  int i;
  GC_init();
  
  GC_CHOOSE_OY(
    printf("The OY technique is GC_OY_MANUAL\n"),
    printf("The OY technique is GC_OY_EPHEMERAL\n")
  );
  
  __capability struct struct1 * old_object = GC_malloc(sizeof(struct struct1));
   
  // fill up the young generation right to the top
  __capability void * dummy;
  for (i=0; i<(GC_THREAD_LOCAL_HEAP_SIZE/sizeof(struct struct1))-1; i++)
    dummy = GC_malloc(sizeof(struct struct1));
  
  // you should confirm that the young generation is filled up
  GC_debug_print_region_stats(GC_state.thread_local_region);
  GC_debug_print_region_stats(GC_state.old_generation);
  
  // a young->young pointer
  GC_CHOOSE_OY(
    {GC_STORE_CAP(old_object->ptr, dummy);},  // GC_OY_MANUAL
    {old_object->ptr = dummy;}                // GC_OY_EPHEMERAL
  );
  GC_PRINT_CAP(old_object);
  GC_PRINT_CAP(old_object->ptr);
   
  // force old_object into the old generation
  dummy = GC_malloc(sizeof(struct struct1));
  
  // you should confirm that the object moved
  GC_debug_print_region_stats(GC_state.thread_local_region);
  GC_debug_print_region_stats(GC_state.old_generation);

  // a old->young pointer
  GC_CHOOSE_OY(
    {GC_STORE_CAP(old_object->ptr, dummy);},  // GC_OY_MANUAL
    {old_object->ptr = dummy;}                // GC_OY_EPHEMERAL
  );
  GC_PRINT_CAP(old_object);
  GC_PRINT_CAP(old_object->ptr);
  
}*/
