#include <gc_init.h>
#include <gc_malloc.h>
#include <gc_debug.h>
#include <gc_collect.h>

#include <machine/cheri.h>
#include <machine/cheric.h>


#include <stdio.h>

void
allocation_test (void);
void
roots_check_test (void);

int
main (int argc, char **argv)
{
  printf("test: compiled %s\n", __TIME__ " " __DATE__);
  roots_check_test();
  return 0;
}

void
allocation_test (void)
{
  GC_init();
  
  int i;
  for (i=0; i<15; i++)
  {
    __capability void *ptr = GC_malloc(0x1000);
    printf("ptr base: 0x%llx\n", (unsigned long long) ptr);
  }
  
  GC_debug_print_region_stats(GC_state.thread_local_region);
}

void
roots_check_test (void)
{
  allocation_test();
  GC_get_roots(&GC_state.thread_local_region);
}
