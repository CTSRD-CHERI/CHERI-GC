#include "gc_low.h"
#include "gc_debug.h"
#include "gc_init.h"

#include <stdlib.h>
#include <string.h>

void *
GC_low_malloc (size_t sz)
{
  return malloc(sz);
}

void *
GC_low_calloc (size_t num, size_t sz)
{
  return calloc(num, sz);
}

void *
GC_low_realloc (void * ptr, size_t sz)
{
  return realloc(ptr, sz);
}

#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>

void *
GC_get_stack_bottom (void)
{
  // This uses a FreeBSD undocumented sysctl call, but the Boehm collector does
  // it too.
  int name[2] = {CTL_KERN, KERN_USRSTACK};
  void * p = NULL;
  size_t lenp = sizeof p;
  if (sysctl(name, (sizeof name)/(sizeof name[0]), &p, &lenp, NULL, 0))
  {
    GC_errf("could not get stack bottom");
    return NULL;
  }
  return p;
}

extern char end; // defined by FreeBSD

// To determine bottom of data section, keep decrementing until we segfault.
// Technically this is undefined behaviour and non-portable.
// We break out of infinite segfaulting by using setjmp/longjmp.
#include <signal.h>
#include <setjmp.h>
static jmp_buf GC_tmp_jmp_buf;
static void (*oldfunc)(int);

static void
GC_sigsegv_handler (int parameter)
{
  longjmp(GC_tmp_jmp_buf, 1);
}

void *
GC_get_static_bottom (void)
{
  if (!GC_is_initialized())
  {
    int val;
    val = setjmp(GC_tmp_jmp_buf);
    if (val == 0)
    {
      GC_state.static_bottom = NULL;
      void * p = GC_ALIGN_32_LOW(GC_get_static_top(), void *);
      
      oldfunc = signal(SIGSEGV, GC_sigsegv_handler);
      if (oldfunc == SIG_ERR)
      {
        GC_errf("could not get static area bottom");
        return NULL;
      }
      
      while (p)
      {
        GC_state.static_bottom = p;
        p = (void *) ( ((uintptr_t) p) - sizeof(GC_cap_ptr) );
        __asm __volatile
        (
          "daddiu $1, %0, 0" : : "r"(p) : "$1"
        );
        __asm __volatile
        (
          "ld $1, 0($1)" : : : "$1"
        );
      }
    }
    
    GC_dbgf("found static bottom: 0x%llx",
      (GC_ULL) GC_state.static_bottom);
    
    oldfunc = signal(SIGSEGV, oldfunc);
    if (oldfunc == SIG_ERR)
    {
      GC_errf("could not get static area bottom");
      return NULL;
    }
  }
  return GC_state.static_bottom;
}

void *
GC_get_static_top (void)
{
  return &end;
}

GC_cap_ptr
GC_cap_memcpy (GC_cap_ptr dest, GC_cap_ptr src)
{
  void * vpdest  = GC_cheri_getbase(dest),
       * vpsrc   = GC_cheri_getbase(src);
  size_t destlen = GC_cheri_getlen(dest),
         srclen  = GC_cheri_getlen(src);
  
  GC_assert( destlen >= srclen );
  GC_assert( NULL != vpdest );
  GC_assert( NULL != vpsrc );
  
  memcpy(vpdest, vpsrc, srclen);

  // Set the tag bits on all capabilities stored in the copied object.
  // We need some way of identifying tag bits better for this part: this is
  // slow.
  GC_cap_ptr * src_child  = GC_ALIGN_32(vpsrc, GC_cap_ptr *);
  GC_cap_ptr * dest_child = GC_ALIGN_32(vpdest, GC_cap_ptr *);
  size_t i;
  for (i = 0; i < srclen / sizeof(GC_cap_ptr); i++)
  {
    if (GC_cheri_gettag(src_child[i]))
    {
      dest_child[i] = src_child[i];
    }
  }

  return GC_cheri_setlen(dest, srclen);
}

GC_cap_ptr
GC_cap_memset (GC_cap_ptr dest, int value)
{
  GC_assert( NULL != GC_cheri_getbase(dest) );
  memset(GC_cheri_getbase(dest), value, GC_cheri_getlen(dest));
  return dest;
}

#ifdef GC_GENERATIONAL
GC_cap_ptr *
GC_handle_oy_store (GC_cap_ptr * x, GC_cap_ptr y)
{
  GC_dbgf("old-young store : *(0x%llx) := 0x%llx\n", (GC_ULL) x, (GC_ULL) y);
  GC_CHOOSE_OY(
    *x = GC_SET_CONTAINED_IN_OLD(y),    // GC_OY_MANUAL
    GC_NOOP                             // GC_OY_EPHEMERAL
  );
  return x;
}
#endif // GC_GENERATIONAL

GC_cap_ptr
GC_orperm (GC_cap_ptr cap, GC_ULL perm)
{  
  return GC_cheri_andperm(
          GC_cheri_ptr(GC_cheri_getbase(cap), GC_cheri_getlen(cap)),
          GC_cheri_getperm(cap) | perm);
}

#ifdef GC_GROW_HEAP
int
GC_grow (struct GC_region * region, size_t hint)
{
  // We try to double the heap size if we can, but allocate up to hint bytes if
  // we have to, and saturate.
  // We want min(max(double, hint), region->max_size).
  // WARNING: we always round *up* to the nearest multiple of 32 bits to avoid
  // alignment issues.
  
#ifdef GC_GENERATIONAL
  // only allocate 2x as much if we need the fromspace.
  size_t GC_mult = GC_is_young(region) ? 1 : 2;
#else // GC_GENERATIONAL
#define GC_mult 2
#endif // GC_GENERATIONAL

  size_t cur_size = GC_cheri_getlen(region->tospace);
  size_t new_size = GC_ALIGN_32(
    GC_MIN(GC_MAX(2*GC_mult*cur_size, GC_mult*(cur_size+hint)),
           GC_mult*region->max_size), size_t);

  GC_dbgf("GC_grow(): hint=%llu, current=%llu, trying=%llu, max=%llu\n",
    (GC_ULL) hint, (GC_ULL) cur_size, (GC_ULL) new_size,
    (GC_ULL) region->max_size);
  
  void * tmp = GC_low_realloc(GC_cheri_getbase(region->tospace), new_size);
  if (!tmp)
  {
    if (new_size > GC_mult*(cur_size+hint))
    {
      // doubling failed, try just allocating the size requested
      new_size = GC_mult*(cur_size+hint);
      tmp = GC_low_realloc(GC_cheri_getbase(region->tospace), new_size);
    }
  }
  if (!tmp) return 0;
  
  // NO! This is now non-trivial. We need to do a `self-copy' or `re-base' (for
  // the tospace), where we examine the roots etc and do a copy. Better is to
  // ask the OS for an immediately adjacent page so that we can keep the whole
  // thing contiguous.
  printf("GC_grow(): Warning, there's a bug here. Exiting.\n");
  exit(0);

  //region->tospace = tmp;
#ifdef GC_GENERATIONAL
  if (GC_mult == 2)
#endif // GC_GENERATIONAL
  {
    //region->fromspace = tmp + new_size;
  }
  
  return new_size >= hint;
}
#endif // GC_GROW_HEAP
