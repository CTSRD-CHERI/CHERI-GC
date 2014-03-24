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