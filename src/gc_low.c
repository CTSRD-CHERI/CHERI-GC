#include "gc_low.h"
#include "gc_debug.h"
#include "gc_init.h"
#include "gc_collect.h"

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
    
    GC_vdbgf("found static bottom: 0x%llx", (GC_ULL) GC_state.static_bottom);
    
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

GC_cap_ptr
GC_cap_memclr (GC_cap_ptr dest)
{
  GC_assert( NULL != GC_cheri_getbase(dest) );

  GC_cap_ptr * start =
    GC_ALIGN_32(GC_cheri_getbase(dest), GC_cap_ptr *);
  GC_cap_ptr * end =
    GC_ALIGN_32_LOW(GC_cheri_getbase(dest) + GC_cheri_getlen(dest), GC_cap_ptr *);
  
  GC_cap_ptr * p;
  for (p = start; p < end; p++)
  {
    *p = GC_INVALID_PTR;
  }
  
  return dest;
}

#ifdef GC_GENERATIONAL
__capability void * __capability *
GC_handle_oy_store (__capability void * __capability * x, GC_cap_ptr y)
{
  GC_dbgf("old-young store : *(0x%llx) := 0x%llx", (GC_ULL) x, (GC_ULL) y);
  
  // Make a trivial root out of x so that the object it points to gets copied
  // and its value gets updated.
  GC_cap_ptr root = GC_cheri_ptr(x, sizeof(GC_cap_ptr));
  
  //GC_fatalf("unhandled for now, quitting.");
  
#if (GC_OY_STORE_DEFAULT == GC_OY_STORE_REMEMBERED_SET)
  GC_remembered_set_add(&GC_state.thread_local_region.remset, root);
#endif // GC_OY_STORE_DEFAULT
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
  // WARNING: we always round *up* to the nearest multiple of 32 bytes to avoid
  // alignment issues.
  
  GC_START_TIMING(GC_grow_time);
    
  hint = GC_ALIGN_32(hint, size_t);
   
  size_t cur_size = GC_cheri_getlen(region->tospace);
    
  if (cur_size == region->max_size)
  {
    GC_vdbgf("GC_grow(): region already max size (%llu%s)",
      GC_MEM_PRETTY((GC_ULL) region->max_size),
      GC_MEM_PRETTY_UNIT((GC_ULL) region->max_size));
    return 0;
  }
  
  GC_assert( cur_size < region->max_size );
  
  void * tospace_base = GC_cheri_getbase(region->tospace);
  size_t new_size = GC_ALIGN_32(
    GC_MIN(GC_MAX(2*cur_size, (cur_size+hint)), region->max_size), size_t);

  GC_vdbgf("GC_grow(): hint=%llu%s, current=%llu%s, trying=%llu%s, max=%llu%s",
    GC_MEM_PRETTY((GC_ULL) hint), GC_MEM_PRETTY_UNIT((GC_ULL) hint),
    GC_MEM_PRETTY((GC_ULL) cur_size), GC_MEM_PRETTY_UNIT((GC_ULL) cur_size),
    GC_MEM_PRETTY((GC_ULL) new_size), GC_MEM_PRETTY_UNIT((GC_ULL) new_size),
    GC_MEM_PRETTY((GC_ULL) region->max_size),
    GC_MEM_PRETTY_UNIT((GC_ULL) region->max_size));
  
  // This is now non-trivial.
  // The reallocation could move the chunk of memory allocated to the tospace,
  // making anything pointing to things inside it invalid.
  
  int fromspace_exists = 
#ifdef GC_GENERATIONAL
    !GC_is_young(region) &&
#endif // GC_GENERATIONAL
    GC_cheri_gettag(region->fromspace);
  
  GC_fatalf("NO! This can't work because region->fromspcae was not malloc'd! (32-bit align...)");
  void * tmp;
  if (fromspace_exists)
  {
//#error NO! This can't work because region->fromspcae was not malloc'd! (32-bit align...)
    void * fromspace_base = GC_cheri_getbase(region->fromspace);
    tmp = GC_low_realloc(fromspace_base, new_size);
    if (!tmp && (new_size > (cur_size+hint)))
    {
      // doubling failed, try just allocating the size requested.
      new_size = (cur_size+hint);
      tmp = GC_low_realloc(fromspace_base, new_size);
    }
    if (!tmp) return 0;
    region->fromspace = GC_setbaselen(region->fromspace, tmp, new_size);
  }
  
  tmp = GC_low_realloc(tospace_base, new_size);
  if (!tmp && (new_size > (cur_size+hint)))
  {
    // doubling failed, try just allocating the requested size.
    new_size = (cur_size+hint);
    // shorten the fromspace, if it was allocated above.
    if (fromspace_exists)
    {
      tmp = GC_low_realloc(GC_cheri_getbase(region->fromspace), new_size);
      if (!tmp)
      {
        // undo our changes
        region->fromspace = GC_cheri_setlen(region->fromspace, cur_size);
        return 0;
      }
      region->fromspace = GC_cheri_setlen(region->fromspace, new_size);
    }
    tmp = GC_low_realloc(tospace_base, new_size);
  }
  if (!tmp)
  {
    if (fromspace_exists)
    {
      // undo our changes
      region->fromspace = GC_cheri_setlen(region->fromspace, cur_size);
    }
    return 0;
  }
  region->tospace = GC_setbaselen(region->tospace, tmp, new_size);
  region->free = GC_setbaselen(
    region->free,
    GC_cheri_getbase(region->free),
    new_size - cur_size + GC_cheri_getlen(region->free));
  
  GC_vdbgf("GC_grow(): actually grew to %llu%s (from %llu%s)",
    GC_MEM_PRETTY((GC_ULL) new_size), GC_MEM_PRETTY_UNIT((GC_ULL) new_size),
    GC_MEM_PRETTY((GC_ULL) cur_size), GC_MEM_PRETTY_UNIT((GC_ULL) cur_size));
  
  if (GC_cheri_getbase(region->tospace) != tospace_base)
  {
    // Do the re-addressing.
    // If we're about to collect anyway, we could do this on-the-fly in the
    // collection routines, but for code simplicity and flexibility we don't
    // bother with that and just do the scan now.
    GC_vdbgf("GC_grow(): region needs rebasing");
    GC_region_rebase(region, tospace_base, cur_size);
    GC_debug_rebase_allocation_entries(
      tospace_base, cur_size, GC_cheri_getbase(region->tospace));
  }
  
  GC_STOP_TIMING(GC_grow_time, "GC_grow %llu%s -> %llu%s",
    GC_MEM_PRETTY((GC_ULL) cur_size), GC_MEM_PRETTY_UNIT((GC_ULL) cur_size),
    GC_MEM_PRETTY((GC_ULL) new_size), GC_MEM_PRETTY_UNIT((GC_ULL) new_size));
  
  return new_size >= (cur_size+hint);
}
#endif // GC_GROW_HEAP
