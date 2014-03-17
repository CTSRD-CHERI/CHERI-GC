#include "gc_low.h"
#include "gc_debug.h"

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
  return GC_cheri_setlen(dest, srclen);
}

GC_cap_ptr
GC_cap_memset (GC_cap_ptr dest, int value)
{
  GC_assert( NULL != GC_cheri_getbase(dest) );
  memset(GC_cheri_getbase(dest), value, GC_cheri_getlen(dest));
  return dest;
}