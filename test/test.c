#include <gc_init.h>
#include <gc_malloc.h>
#include <gc_debug.h>

#include <stdio.h>

void allocation_test (void);

int
main (int argc, char **argv)
{
  printf("test: compiled %s\n", __TIME__ " " __DATE__);
  allocation_test();
  printf("GC_init: %d\n", GC_init());
  return 0;
}


void
allocation_test (void)
{
  GC_init();
  __capability void *ptr = GC_malloc(100);
  printf("ptr base: 0x%llx\n", (unsigned long long) ptr);
  GC_debug_print_region_stats(GC_state.thread_local_region);
}