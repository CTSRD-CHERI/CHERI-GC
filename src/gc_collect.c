#include "gc_collect.h"
#include "gc_init.h"
#include "gc_low.h"
#include "gc_debug.h"

#include <machine/cheri.h>
#include <machine/cheric.h>

void
GC_collect (void)
{
  if (!GC_is_initialized()) GC_init();
  GC_collect_region(&GC_state.thread_local_region);
}

void
GC_collect_region (struct GC_region * region)
{
  GC_get_roots(region);
}

int
GC_reallocate_roots (struct GC_region * region)
{
  region->maxroots = (region->maxroots == 0) ? 10 : (region->maxroots*2);
  struct GC_root * roots =
    GC_low_realloc(region->roots, region->maxroots*sizeof(struct GC_root));
  if (roots == NULL)
    return 1;
  region->roots = roots;
  return 0;
}

int
GC_get_roots (struct GC_region * region)
{
  
  region->nroots = 0;
  
  #define GC_PUSH_ROOT(x) \
    do { \
      if ((region->nroots == region->maxroots) && GC_reallocate_roots(region)) \
      { \
        GC_errf("region->roots allocation failed"); \
        return 1; \
      } \
      region->roots[region->nroots++].cap = (x); \
    } while (0)
  
  // TODO:
  // load into r1 &cap_ptr
  // csc $cX, $r1, 0($c0)
  __asm __volatile
  (
    "csc $1, $c1" : :  : "$1"
  );
  
  
  // no, no, no. this needs to all be done in assembly or something so that we
  // can ensure that registers aren't modified in the process.
  
  
  /*__capability void *ptr = cheri_getreg(0);
  if (cheri_gettag(cheri_getreg(1))) GC_PUSH_ROOT(cheri_getreg(1));
  GC_PUSH_ROOT(cheri_getreg(2));
  GC_PUSH_ROOT(cheri_getreg(3));
  GC_PUSH_ROOT(cheri_getreg(4));
  GC_PUSH_ROOT(cheri_getreg(5));
  GC_PUSH_ROOT(cheri_getreg(6));
  GC_PUSH_ROOT(cheri_getreg(7));
  GC_PUSH_ROOT(cheri_getreg(8));
  GC_PUSH_ROOT(cheri_getreg(9));
  GC_PUSH_ROOT(cheri_getreg(10));
  GC_PUSH_ROOT(cheri_getreg(11));
  GC_PUSH_ROOT(cheri_getreg(12));
  GC_PUSH_ROOT(cheri_getreg(13));
  GC_PUSH_ROOT(cheri_getreg(14));
  GC_PUSH_ROOT(cheri_getreg(15));
  GC_PUSH_ROOT(cheri_getreg(16));
  GC_PUSH_ROOT(cheri_getreg(17));
  GC_PUSH_ROOT(cheri_getreg(18));
  GC_PUSH_ROOT(cheri_getreg(19));
  GC_PUSH_ROOT(cheri_getreg(20));
  GC_PUSH_ROOT(cheri_getreg(21));
  GC_PUSH_ROOT(cheri_getreg(22));
  GC_PUSH_ROOT(cheri_getreg(23));*/
  
  int i;
  for (i=0; i<region->nroots; i++)
    GC_dbgf("root [%d]: b=0x%llx, l=0x%llx\n",
      i,
      (unsigned long long) cheri_getbase(region->roots[i].cap),
      (unsigned long long) cheri_getlen(region->roots[i].cap));
  
  #undef GC_PUSH_ROOT
  
  return 0;
}
