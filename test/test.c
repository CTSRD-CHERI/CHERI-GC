#include <gc_init.h>
#include <gc_malloc.h>
#include <gc_debug.h>
#include <gc_collect.h>
#include <gc_low.h>

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <stdio.h>
#include <string.h>

void
allocation_test (void);
void
memdump_test (void);
void
capdump_test (void);
void
collection_test (void);

__capability void * arbitrary_cap;

int
main (int argc, char **argv)
{
  printf("test: compiled %s\n", __TIME__ " " __DATE__);
  arbitrary_cap = GC_cheri_ptr((void*)0x9988, 0x7777);
  collection_test();
  return 0;
}

void
allocation_test (void)
{
  GC_init();
  
  int i;
  for (i=0; i<15; i++)
  {
    __capability void * ptr = GC_malloc(0x1000);
    printf("ptr base: 0x%llx\n", (unsigned long long) ptr);
  }
  
  GC_debug_print_region_stats(GC_state.thread_local_region);
}

void
memdump_test (void)
{
  allocation_test();
  GC_collect_region(&GC_state.thread_local_region);
  const char * buf = "hello!!$thiz2";
  GC_debug_memdump(buf+1, buf+strlen(buf));
  printf("\n\n\n");
  const char * buf2 = "hello!!$thiz";
  GC_debug_memdump(buf2, buf2+strlen(buf2));
  printf("\n\n\n");
  const char * buf3 = "hello!!$thiz234";
  GC_debug_memdump(buf3, buf3+strlen(buf3));
}

void
capdump_test (void)
{
  int a = 0xABCDEF;
  __capability void * b = GC_cheri_ptr((void*)0x1234, 0x5678);
  void * stack_top = NULL;
  GC_init();
  GC_GET_STACK_PTR(stack_top);
  GC_debug_capdump(GC_state.stack_bottom, stack_top);
}

void
collection_test (void)
{
  int a[50];
  memset(a, 0, 32*sizeof (int));
  a[0] = 0x00ABCDEF;
  __capability void * b = GC_cheri_ptr(&a, 0x5678);
  void * stack_top = NULL;
  GC_init();
  GC_GET_STACK_PTR(stack_top);
  GC_cheri_setreg(3, arbitrary_cap);
  GC_PUSH_CAP_REG(3, GC_FORWARDING_ADDRESS((void*) &a));
  GC_collect_range(&GC_state.thread_local_region, stack_top, stack_top+10*32);
}
