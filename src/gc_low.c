#include "gc_low.h"
#include "gc_debug.h"
#include "gc_init.h"
#include "gc_collect.h"

#include <stdlib.h>
#include <string.h>

struct GC_alloc_entry_t
{
  void * ptr;
  size_t len;
};
#define GC_MAX_ALLOC_ENTRY 500
struct GC_alloc_entry_t GC_alloc_table[GC_MAX_ALLOC_ENTRY];
int GC_alloc_index = 0;
void *
GC_add_to_alloc_list (void * p, size_t sz, void * old_ptr)
{
  if (GC_alloc_index == GC_MAX_ALLOC_ENTRY)
  {
    GC_fatalf("Too many allocation entries!\n");
  }
  if (old_ptr)
  {
    // this is a realloc.
    GC_vdbgf("[GC alloc]: REALLOC: Finding entry for old_ptr=0x%llx\n",
      (GC_ULL) old_ptr);
    int i;
    for (i=0; i<GC_alloc_index; i++)
    {
      if (GC_alloc_table[i].ptr == old_ptr) break;
    }
    if (i<GC_alloc_index)
    {
      GC_vdbgf("[GC alloc]: REALLOC: Found. OLD: p=0x%llx sz=%d NEW: p=0x%llx sz=%d\n",
        (GC_ULL) GC_alloc_table[i].ptr,
        (int) GC_alloc_table[i].len,
        (GC_ULL) p,
        (int) sz);
#ifdef GC_DEBUG
      if (sz > GC_alloc_table[i].len)
        memset(p+GC_alloc_table[i].len,
               GC_MAGIC_JUST_REALLOCATED,
               sz-GC_alloc_table[i].len);
#endif // GC_DEBUG
      GC_alloc_table[i].ptr = p;
      GC_alloc_table[i].len = sz;
    }
    else
    {
      GC_fatalf("[GC alloc]: REALLOC: Could not find entry!\n");
    }
  }
  else if (p)
  {
    if (sz > 0)
    {
      // this is a malloc.
#ifdef GC_DEBUG
      memset(p, GC_MAGIC_JUST_ALLOCATED, sz);
#endif // GC_DEBUG
      GC_vdbgf("[GC alloc]: MALLOC: Just allocated p=0x%llx sz=%d\n",
        (GC_ULL) p, (int) sz);
    }
    else
    {
      sz = -sz;
      GC_vdbgf("[GC alloc]: CALLOC: Just allocated p=0x%llx sz=%d\n",
        (GC_ULL) p, (int) sz);
    }
    GC_alloc_table[GC_alloc_index].ptr = p;
    GC_alloc_table[GC_alloc_index].len = sz;
    GC_alloc_index++;
  }
  return p;
}

void *
GC_low_malloc (size_t sz)
{
  void * p = malloc(sz);
  return GC_add_to_alloc_list(p, sz, NULL);
}

void *
GC_low_calloc (size_t num, size_t sz)
{
  void * p = calloc(num, sz);
  return GC_add_to_alloc_list(p, -num*sz, NULL);
}

void *
GC_low_realloc (void * ptr, size_t sz)
{
  //void * p = realloc(ptr, sz);
  //return GC_add_to_alloc_list(p, sz, ptr);
  
  if (!ptr) return GC_low_malloc(sz);
  // find size:
  int i;
  for (i=0; i<GC_alloc_index; i++)
  {
    if (GC_alloc_table[i].ptr == ptr) break;
  }
  if (i<GC_alloc_index)
  {
    GC_vdbgf("[GC_low_realloc]: REALLOC: Found. OLD: p=0x%llx sz=%d NEW: p=(not known yet) sz=%d\n",
      (GC_ULL) GC_alloc_table[i].ptr,
      (int) GC_alloc_table[i].len,
      (int) sz);
    void * p = malloc(sz);
    if (p)
    {
      // WARNING: all perms set...
      GC_cap_ptr new_cap = GC_cheri_ptr(p, sz);
      GC_cap_ptr old_cap =
        GC_cheri_ptr(GC_alloc_table[i].ptr, GC_alloc_table[i].len);
      GC_cap_memcpy(new_cap, old_cap);
      free(ptr);
    }
    return GC_add_to_alloc_list(p, sz, ptr);
  }
  else
  {
    GC_fatalf("[GC realloc]: REALLOC: Could not find entry!\n");
  }
  return NULL;
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
      //GC_assert(GC_cheri_getperm(dest_child[i]) == GC_cheri_getperm(src_child[i]));
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
  
  GC_vdbgf("NOTE: doing a memclr...\n");
#ifdef GC_DEBUG
  GC_cap_memset(dest, GC_MAGIC_JUST_CLEARED);
#endif // GC_DEBUG

  return dest;
}

#ifdef GC_GENERATIONAL
__capability void * __capability *
GC_handle_oy_store (__capability void * __capability * x, GC_cap_ptr y)
{
  GC_dbgf("old-young store : *(0x%llx) := 0x%llx", (GC_ULL) x, (GC_ULL) y);
  
  //GC_fatalf("unhandled for now, quitting.");
  
#if (GC_OY_STORE_DEFAULT == GC_OY_STORE_REMEMBERED_SET)
  GC_remembered_set_add(GC_state.thread_local_region.remset, (void *) x);
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
GC_grow (struct GC_region * region, size_t hint, size_t max_size)
{
  // We double the heap size if we can, allocate up to hint bytes if we have to,
  // and saturate at the max.
  // We want min(max(double, hint), max_size).
  // WARNING: we always round *up* to the nearest multiple of 32 bytes to avoid
  // alignment issues.
  
  GC_START_TIMING(GC_grow_time);
    
  void * old_tospace_base_misaligned = region->tospace_misaligned,
       * old_fromspace_base_misaligned = region->fromspace_misaligned,
       * new_tospace_base_misaligned = NULL,
       * new_fromspace_base_misaligned = NULL;
  
  hint = GC_ALIGN_32(hint, size_t);
    
  size_t old_size = GC_cheri_getlen(region->tospace);
  
  if (old_size >= max_size)
  {
    GC_vdbgf("GC_grow(): region already max size (%llu%s)",
      GC_MEM_PRETTY((GC_ULL) max_size),
      GC_MEM_PRETTY_UNIT((GC_ULL) max_size));
    return 0;
  }
  
  GC_assert( !max_size || (old_size < max_size) );
  
  size_t new_size =
    max_size ?
    GC_ALIGN_32(GC_MIN(GC_MAX(2*old_size, (old_size+hint)), max_size), size_t) :
    GC_ALIGN_32(GC_MAX(2*old_size, (old_size+hint)), size_t);
  
  GC_dbgf("GC_grow(): hint=%llu%s, current=%llu%s, trying=%llu%s, max=%llu%s",
    GC_MEM_PRETTY((GC_ULL) hint), GC_MEM_PRETTY_UNIT((GC_ULL) hint),
    GC_MEM_PRETTY((GC_ULL) old_size), GC_MEM_PRETTY_UNIT((GC_ULL) old_size),
    GC_MEM_PRETTY((GC_ULL) new_size), GC_MEM_PRETTY_UNIT((GC_ULL) new_size),
    GC_MEM_PRETTY((GC_ULL) max_size),
    GC_MEM_PRETTY_UNIT((GC_ULL) max_size));
    
  // This is now non-trivial.
  // The reallocation could move the chunk of memory allocated to the tospace,
  // making anything pointing to things inside it invalid.
  
  int fromspace_exists = 
#ifdef GC_GENERATIONAL
    !GC_is_young(region) &&
#endif // GC_GENERATIONAL
    GC_cheri_gettag(region->fromspace);

  if (fromspace_exists)
  {
    // try doubling first
    new_fromspace_base_misaligned =
      GC_low_realloc(old_fromspace_base_misaligned, new_size+32);
    if (new_fromspace_base_misaligned == NULL)
    {
      GC_dbgf("GC_grow(): growing the fromspace (first attempt) failed\n");
      if (new_size > (old_size+hint))
      {
        // doubling failed; try allocating only what is required
        new_size = old_size+hint;
        new_fromspace_base_misaligned =
          GC_low_realloc(old_fromspace_base_misaligned, new_size+32);
        if (new_fromspace_base_misaligned == NULL)
        {
          GC_dbgf("GC_grow(): growing the fromspace (second attempt) failed\n");
          return 0;
        }
      }
    }
  }
  
  new_tospace_base_misaligned =
    GC_low_realloc(old_tospace_base_misaligned, new_size+32);
  if (new_tospace_base_misaligned == NULL)
  {
    GC_dbgf("GC_grow(): growing the tospace (first attempt) failed\n");
    if (new_size > (old_size+hint))
    {
      new_size = old_size+hint;
      // doubling failed; try allocating only what is required, but first shrink
      // down the fromspace, if possible
      void * tmp =
        GC_low_realloc(new_fromspace_base_misaligned, new_size+32);
      if (tmp == NULL)
      {
        GC_dbgf("GC_grow(): warning: shrinking the fromspace failed\n");
      }
      else
      {
        new_fromspace_base_misaligned = tmp;
      }
      new_tospace_base_misaligned = 
        GC_low_realloc(old_tospace_base_misaligned, new_size+32);
      if (new_tospace_base_misaligned == NULL)
      {
        GC_dbgf("GC_grow(): growing the tospace (second attempt) failed\n");
        return 0;
      }
    }
  }
  
  region->fromspace_misaligned = new_fromspace_base_misaligned;
  region->fromspace = GC_setbaselen(
    region->fromspace,
    GC_ALIGN_32(new_fromspace_base_misaligned, void *),
    new_size);
  region->tospace_misaligned = new_tospace_base_misaligned;
  region->tospace = GC_setbaselen(
    region->tospace,
    GC_ALIGN_32(new_tospace_base_misaligned, void *),
    new_size);

  region->free = GC_setbaselen(
    region->free,
    GC_cheri_getbase(region->free),
    GC_cheri_getlen(region->free) + new_size - old_size);

  GC_vdbgf("GC_grow(): actually grew to %llu%s (from %llu%s)",
    GC_MEM_PRETTY((GC_ULL) new_size), GC_MEM_PRETTY_UNIT((GC_ULL) new_size),
    GC_MEM_PRETTY((GC_ULL) old_size), GC_MEM_PRETTY_UNIT((GC_ULL) old_size));
  
  if (new_tospace_base_misaligned != old_tospace_base_misaligned)
  {
    // Do the re-addressing.
    // If we're about to collect anyway, we could do this on-the-fly in the
    // collection routines, but for code simplicity and flexibility we don't
    // bother with that and just do the scan now.
    GC_vdbgf("GC_grow(): region needs rebasing");

    GC_region_rebase(
      region,
      GC_ALIGN_32(old_tospace_base_misaligned, void *),
      old_size);
    GC_debug_rebase_allocation_entries(
      GC_ALIGN_32(old_tospace_base_misaligned, void *),
      old_size,
      GC_ALIGN_32(new_tospace_base_misaligned, void *));
  }

  GC_assert(
    GC_IN_OR_ON_BOUNDARY(GC_cheri_getbase(region->free), region->tospace) );
  
  GC_vdbgf(
    "GC_grow():\n"
    "old tospace base    :     0x%llx\n"
    "old tospace end     :     0x%llx\n"
    "new tospace base    :     0x%llx\n"
    "new tospace end     :     0x%llx\n"
    "old fromspace base  :     0x%llx\n"
    "old fromspace end   :     0x%llx\n"
    "new fromspace base  :     0x%llx\n"
    "new fromspace end   :     0x%llx\n",
    (GC_ULL) GC_ALIGN_32(old_tospace_base_misaligned, void *),
    (GC_ULL) (GC_ALIGN_32(old_tospace_base_misaligned, void *) + old_size),
    (GC_ULL) GC_cheri_getbase(region->tospace),
    (GC_ULL) (GC_cheri_getbase(region->tospace)+GC_cheri_getlen(region->tospace)),
    (GC_ULL) GC_ALIGN_32(old_fromspace_base_misaligned, void *),
    (GC_ULL) (GC_ALIGN_32(old_fromspace_base_misaligned, void *) + old_size),
    (GC_ULL) GC_cheri_getbase(region->fromspace),
    (GC_ULL) (GC_cheri_getbase(region->fromspace)+GC_cheri_getlen(region->fromspace)));
  
  GC_STOP_TIMING(GC_grow_time, "GC_grow %llu%s -> %llu%s",
    GC_MEM_PRETTY((GC_ULL) old_size), GC_MEM_PRETTY_UNIT((GC_ULL) old_size),
    GC_MEM_PRETTY((GC_ULL) new_size), GC_MEM_PRETTY_UNIT((GC_ULL) new_size));
  
  return new_size >= (old_size+hint);
}

/*
int
GC_grow_OLD (struct GC_region * region, size_t hint)
{
  // We double the heap size if we can, allocate up to hint bytes if we have to,
  // and saturate at the max.
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
  
  void * old_tospace_base_misaligned = region->tospace_misaligned;
  void * old_tospace_base = GC_cheri_getbase(region->tospace);
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
  
  void * tmp;
  if (fromspace_exists)
  {
    void * fromspace_base_misaligned = region->fromspace_misaligned;
    tmp = GC_low_realloc(fromspace_base_misaligned, new_size+32);
    printf("FROMSPACE ATTEMPT #1: fromspace_base_misaligned=0x%llx, tmp=0x%llx\n", (GC_ULL) fromspace_base_misaligned, (GC_ULL) tmp);
    if (!tmp && (new_size > (cur_size+hint)))
    {
      // doubling failed, try just allocating the size requested.
      new_size = (cur_size+hint);
      tmp = GC_low_realloc(fromspace_base_misaligned, new_size+32);
      printf("FROMSPACE ATTEMPT #2: fromspace_base_misaligned=0x%llx, tmp=0x%llx\n", (GC_ULL) fromspace_base_misaligned, (GC_ULL) tmp);
    }
    if (!tmp) return 0;
    region->fromspace_misaligned = tmp;
    region->fromspace = GC_setbaselen(
      region->fromspace, GC_ALIGN_32(tmp, void *), new_size);
  }
  
  tmp = GC_low_realloc(old_tospace_base_misaligned, new_size+32);
  printf("TOSPACE ATTEMPT #1: old_tospace_base_misaligned=0x%llx, tmp=0x%llx\n", (GC_ULL) old_tospace_base_misaligned, (GC_ULL) tmp);
  if (!tmp && (new_size > (cur_size+hint)))
  {
    // doubling failed, try just allocating the requested size.
    new_size = (cur_size+hint);
    // shorten the fromspace, if it was allocated above.
    if (fromspace_exists)
    {
      tmp = GC_low_realloc(region->fromspace_misaligned, new_size+32);
      printf("TOSPACE ATTEMPT #1 FAILED, FROMSPACE ATTEMPT AT SHRINKING: old_tospace_base_misaligned=0x%llx, tmp=0x%llx, region->fromspace_misaligned=0x%llx\n", (GC_ULL) old_tospace_base_misaligned, (GC_ULL) tmp, (GC_ULL) region->fromspace_misaligned);
      if (!tmp)
      {
        // undo our changes: keep the original fromspace size
        region->fromspace = GC_cheri_setlen(region->fromspace, cur_size);
        return 0;
      }
      region->fromspace_misaligned = tmp;
      region->fromspace = GC_setbaselen(
        region->fromspace, GC_ALIGN_32(tmp, void *), new_size);
    }
    tmp = GC_low_realloc(old_tospace_base_misaligned, new_size+32);
    printf("TOSPACE ATTEMPT #2: old_tospace_base_misaligned=0x%llx, tmp=0x%llx\n", (GC_ULL) old_tospace_base_misaligned, (GC_ULL) tmp);
  }
  if (!tmp)
  {
    if (fromspace_exists)
    {
      // undo our changes: keep the original fromspace size
      region->fromspace = GC_cheri_setlen(region->fromspace, cur_size);
    }
    return 0;
  }
  region->tospace_misaligned = tmp;
  region->tospace = GC_setbaselen(region->tospace,
    GC_ALIGN_32(tmp, void *), new_size);
  region->free = GC_setbaselen(
    region->free,
    GC_cheri_getbase(region->free),
    new_size - cur_size + GC_cheri_getlen(region->free));
  
  GC_vdbgf("GC_grow(): actually grew to %llu%s (from %llu%s)",
    GC_MEM_PRETTY((GC_ULL) new_size), GC_MEM_PRETTY_UNIT((GC_ULL) new_size),
    GC_MEM_PRETTY((GC_ULL) cur_size), GC_MEM_PRETTY_UNIT((GC_ULL) cur_size));
  
  if (GC_cheri_getbase(region->tospace) != old_tospace_base)
  {
    // Do the re-addressing.
    // If we're about to collect anyway, we could do this on-the-fly in the
    // collection routines, but for code simplicity and flexibility we don't
    // bother with that and just do the scan now.
    GC_vdbgf("GC_grow(): region needs rebasing");
    GC_region_rebase(region, old_tospace_base, cur_size);
    GC_debug_rebase_allocation_entries(
      old_tospace_base, cur_size, GC_cheri_getbase(region->tospace));
  }
  
  GC_STOP_TIMING(GC_grow_time, "GC_grow %llu%s -> %llu%s",
    GC_MEM_PRETTY((GC_ULL) cur_size), GC_MEM_PRETTY_UNIT((GC_ULL) cur_size),
    GC_MEM_PRETTY((GC_ULL) new_size), GC_MEM_PRETTY_UNIT((GC_ULL) new_size));
  
  return new_size >= (cur_size+hint);
}*/

#endif // GC_GROW_HEAP
