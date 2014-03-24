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
  printf("test: compiled %s\n",
         __TIME__ " " __DATE__);
  collection_test();
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
collection_test (void)
{
  #define NALLOC    0x1000
  #define NSTORE    0x100
  #define NBYTES    0x100
  int i;
  GC_cap_ptr arr[NSTORE];
  GC_init();
  for (i=0; i<NALLOC; i++)
  {
    arr[i%NSTORE] = GC_malloc(NBYTES);
    if (!(void*)arr[i%NSTORE])
      {printf("ERROR: oom %d\n", i);break;}
  }
  GC_debug_print_region_stats(GC_state.thread_local_region);
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
