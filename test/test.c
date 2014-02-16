#include <gc_init.h>

#include <stdio.h>

int
main (int argc, char **argv)
{
  printf("main\n");
  printf("GC_init: %d\n", GC_init());
  return 0;
}
