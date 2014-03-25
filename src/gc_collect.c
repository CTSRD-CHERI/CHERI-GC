#include "gc_collect.h"
#include "gc_init.h"
#include "gc_low.h"
#include "gc_debug.h"
#include "gc_config.h"

#include <stdint.h>

void
GC_collect (void)
{
  if (!GC_is_initialized()) GC_init();
  GC_collect_region(&GC_state.thread_local_region);
}

void
GC_collect_region (struct GC_region * region)
{
  region->num_collections++; // debugging/stats

#ifdef GC_GENERATIONAL
  if (GC_is_young(region))
  {
    GC_vdbgf("region is young, promoting objects...");
    GC_gen_promote(region);
  }
  else
#endif // GC_GENERATIONAL
  {
    GC_vdbgf("region is old, collecting proper...");
    size_t old_used =
      GC_cheri_getbase(region->free) - GC_cheri_getbase(region->tospace);
    
    GC_cap_ptr tmp = region->fromspace;
    region->fromspace = region->tospace;
    region->tospace = tmp;
    
    region->free = region->tospace;
    region->scan = (GC_cap_ptr *) GC_cheri_getbase(region->tospace);
    
    GC_copy_region(region, 0);

    size_t new_used =
      GC_cheri_getbase(region->free) - GC_cheri_getbase(region->tospace);
    
    size_t freed = old_used - new_used;
    
    GC_dbgf(
      "(old) generation  old_used=%llu%s  new_used=%llu%s  freed=%llu%s",
      GC_MEM_PRETTY((GC_ULL) old_used), GC_MEM_PRETTY_UNIT((GC_ULL) old_used),
      GC_MEM_PRETTY((GC_ULL) new_used), GC_MEM_PRETTY_UNIT((GC_ULL) new_used),
      GC_MEM_PRETTY((GC_ULL) freed), GC_MEM_PRETTY_UNIT((GC_ULL) freed));
  }
}

void
GC_copy_region (struct GC_region * region,
                int is_generational)
{ 
  GC_PUSH_CAP_REGS(cap_regs);

  int i;
  for (i=0; i<GC_NUM_CAP_REGS; i++)
  {
    if (GC_cheri_gettag(cap_regs[i]))
    {
      GC_vdbgf("cap_reg root [%d]: t=1, b=0x%llx, l=0x%llx",
        i,
        (GC_ULL) GC_cheri_getbase(cap_regs[i]),
        (GC_ULL) GC_cheri_getlen(cap_regs[i])
      );
    }
  }
  
  void * stack_top = NULL;
  GC_GET_STACK_PTR(stack_top);
  
  GC_assert(stack_top <= GC_state.stack_bottom);
 
  GC_copy_roots(
    region, stack_top, GC_state.stack_bottom, is_generational);
  GC_copy_roots(
    region, GC_state.static_bottom, GC_state.static_top, is_generational);
  GC_copy_children(region, is_generational);
  
  GC_RESTORE_CAP_REGS(cap_regs);

  for (i=0; i<GC_NUM_CAP_REGS; i++)
  {
    if (GC_cheri_gettag(cap_regs[i]))
    {
      GC_vdbgf("cap_reg restored root [%d]: t=1, b=0x%llx, l=0x%llx",
        i,
        (GC_ULL) GC_cheri_getbase(cap_regs[i]),
        (GC_ULL) GC_cheri_getlen(cap_regs[i])
      );
    }
  }
  
}

GC_cap_ptr
GC_copy_object (struct GC_region * region,
                GC_cap_ptr cap)
{
  // Copy the object pointed to by cap to region->tospace

  GC_assert(GC_cheri_getlen(cap) >= sizeof(GC_cap_ptr));
  
  GC_debug_just_deallocated(cap);
  
  // If the first word of the object points to a place in region->tospace,
  // then it has already been copied.
  
  // $c2 = *(cap.base + forwarding_address_offset) = *($c0.base + forwarding_address_field_location);
  GC_cap_ptr forwarding_address = GC_FORWARDING_CAP(cap);
  
  unsigned tag = GC_cheri_gettag(forwarding_address); //0;
  if (tag)
  {      
    if (GC_IS_FORWARDING_ADDRESS(forwarding_address))
    {
      GC_assert(GC_IN(GC_cheri_getbase(forwarding_address), region->tospace));
      GC_debug_just_allocated(forwarding_address);
      return GC_STRIP_FORWARDING(forwarding_address);
    }
  }
  
  // No forwarding address present: do the actual copy and set the forwarding
  // address.
  GC_cap_ptr tmp = GC_cap_memcpy(region->free, cap);
  region->free =
    GC_cheri_ptr(
      GC_cheri_getbase(region->free)+GC_cheri_getlen(cap),
      GC_cheri_getlen (region->free)-GC_cheri_getlen(cap));

  // Set the forwarding address of the old object.
  GC_FORWARDING_CAP(cap) = GC_MAKE_FORWARDING_ADDRESS(tmp);
  
  GC_debug_just_allocated(tmp);
  return tmp;
}

void
GC_copy_roots (struct GC_region * region,
               void * root_start,
               void * root_end,
               int is_generational)
{
  // TODO: ignore roots in the GC's call stack. There shouldn't be any
  // (unintended) roots in the GC's call stack unless a capability argument is
  // passed in any calls up to this call or local capability variables are
  // declared.
  
  GC_assert(root_start <= root_end);
  
  root_start = GC_ALIGN_32(root_start, void *);
  root_end = GC_ALIGN_32_LOW(root_end, void *);
  
  GC_cap_ptr * p;
  for (p = (GC_cap_ptr *) root_start;
       ((uintptr_t) p) < ((uintptr_t) root_end);
       p++)
  {
    if (GC_IN(p, GC_state_cap))
    {
      continue;
    }
    if (GC_cheri_gettag(*p) && GC_IN(GC_cheri_getbase(*p), region->fromspace))
    {
      GC_vdbgf("[root] location=0x%llx (%s?), b=0x%llx, l=0x%llx\n",
        (GC_ULL) p,
        ((GC_ULL) p) & 0x7F00000000 ? "stack/reg" : ".data",
        (GC_ULL) *p, 
        (GC_ULL) GC_cheri_getlen(*p));
      *p = GC_copy_object(region, *p);
#ifdef GC_GENERATIONAL
      if (is_generational)
      {
        GC_CHOOSE_OY(
          {*p = GC_UNSET_YOUNG(*p);},      // GC_OY_MANUAL
          {*p = GC_UNSET_EPHEMERAL(*p);}   // GC_OY_EPHEMERAL
        );
      }
#endif // GC_GENERATIONAL
    }
  }
}

void
GC_copy_children (struct GC_region * region,
                  int is_generational)
{
  for (;
       ((uintptr_t) region->scan)
         < ((uintptr_t) GC_cheri_getbase(region->free));
       region->scan++)
  {
    if (GC_cheri_gettag(*region->scan)
        && GC_IN(GC_cheri_getbase(*region->scan), region->fromspace))
    {
      GC_vdbgf("[child] location=0x%llx (%s?), b=0x%llx, l=0x%llx\n",
        (GC_ULL) region->scan,
        ((GC_ULL) region->scan) & 0x7F00000000 ? "stack/reg" : ".data",
        (GC_ULL) *region->scan, 
        (GC_ULL) GC_cheri_getlen(*region->scan));
      *region->scan = GC_copy_object(region, *region->scan);
#ifdef GC_GENERATIONAL
      if (is_generational)
      {
        GC_CHOOSE_OY(
          {*region->scan =
            GC_UNSET_YOUNG(
            GC_SET_CONTAINED_IN_OLD(*region->scan));}, // GC_OY_MANUAL
          {*region->scan =
            GC_UNSET_EPHEMERAL(*region->scan);}         // GC_OY_EPHEMERAL
        );
      }
#endif // GC_GENERATIONAL
    }
  }
}

#ifdef GC_GENERATIONAL
void
GC_gen_promote (struct GC_region * region)
{
  // Conservative estimate. Usually requires the old generation to have at least
  // as much space as the entire young generation (because GC_gen_promote is
  // usually called when the young generation is almost full).
  int too_small =
    GC_cheri_getlen(region->older_region->free)
      < (GC_cheri_getbase(region->free) - GC_cheri_getbase(region->tospace));
#ifdef GC_GROW_OLD_HEAP
  if (too_small)
  {
    GC_dbgf("old generation too small, trying to grow");
    too_small =
      !GC_grow(region->older_region,
          (GC_cheri_getbase(region->free) - GC_cheri_getbase(region->tospace)));
  }
#endif // GC_GROW_OLD_HEAP
  if (too_small)
  {
    GC_dbgf("old generation too small, triggering major collection");
    GC_collect_region(region->older_region);
    too_small =
      GC_cheri_getlen(region->older_region->free)
        < (GC_cheri_getbase(region->free) - GC_cheri_getbase(region->tospace));
  }
  
  if (too_small)
  {
    GC_errf("old generation out of memory");
    return;
  }
  
  GC_cap_ptr old_from_space = region->older_region->fromspace;
  region->older_region->fromspace = region->tospace;
  
  // ensure we only scan the young region's children when copying
  region->older_region->scan = (GC_cap_ptr *) region->older_region->free;
  
  GC_copy_region(region->older_region, 1);
  
  region->older_region->fromspace = old_from_space;
  region->free = region->tospace;
}
#endif // GC_GENERATIONAL

void
GC_region_rebase (struct GC_region * region, void * old_base, size_t old_size)
{
  // NOTE: could use the relevant permission bit for checking whether a capability
  // is within the previous YOUNG REGION or the previous OLD REGION or whatever.
  // Pass the permission bit as a hint to this function...
  // TODO: make it deal with roots when we do old/young stuff (either that, or
  // never grow a young region)
  
  void * new_base = GC_cheri_getbase(region->tospace);
  size_t new_size = GC_cheri_getlen(region->tospace);
  
  GC_vdbgf(
    "rebasing region\n"
    "old_base = 0x%llx\n"
    "old_size = %llu%s\n"
    "old_end  = 0x%llx\n"
    "new_base = 0x%llx\n"
    "new_size = %llu%s\n"
    "new_end  = 0x%llx\n",
    (GC_ULL) old_base,
    GC_MEM_PRETTY((GC_ULL) old_size), GC_MEM_PRETTY_UNIT((GC_ULL) old_size),
    (GC_ULL) (old_base+old_size),
    (GC_ULL) new_base,
    GC_MEM_PRETTY((GC_ULL) new_size), GC_MEM_PRETTY_UNIT((GC_ULL) new_size),
    (GC_ULL) (new_base+new_size)
  );
  
  GC_PUSH_CAP_REGS(cap_regs);
  
  // TODO: make sure this stays on the stack!
  GC_cap_ptr free_ptr_on_the_stack = region->free;
  
  void * stack_top = NULL;
  GC_GET_STACK_PTR(stack_top);
  
  GC_assert(stack_top <= GC_state.stack_bottom);
 
  GC_rebase(stack_top, GC_state.stack_bottom,
            old_base, old_size, new_base);
  GC_rebase(GC_state.static_bottom, GC_state.static_top,
            old_base, old_size, new_base);
  GC_rebase(new_base, new_base+new_size,
            old_base, old_size, new_base);
  
  region->free = free_ptr_on_the_stack;
  // fails due to aliasing?
  /*
  region->free =
    GC_setbase(
      region->free,
      GC_cheri_getbase(region->free) - old_base + new_base);
  */
  
  GC_RESTORE_CAP_REGS(cap_regs);
}

void
GC_rebase (void * start,
           void * end,
           void * old_base,
           size_t old_size,
           void * new_base)
{
  GC_assert(start <= end);
  
  start = GC_ALIGN_32(start, void *);
  end = GC_ALIGN_32_LOW(end, void *);
  
  // Rebasing is complicated by the fact that the interval [old_base, old_size]
  // might overlap with the interval [new_base, new_size]. Due to aliasing, we
  // need to make sure that we don't change the base twice.
  // To ensure this, we set the forwarding address marker on each capability
  // after we change the base the first time, and then we unset all of them.
  
  GC_cap_ptr * p;
  for (p = (GC_cap_ptr *) start; ((uintptr_t) p) < ((uintptr_t) end); p++)
  {
    if (GC_cheri_gettag(*p) && !GC_IS_FORWARDING_ADDRESS(*p))
    {
      void * base = GC_cheri_getbase(*p);
      if ((base >= old_base) && (base < (old_base+old_size)))
      {
        *p = GC_setbase(*p, base-old_base+new_base);
        *p = GC_MAKE_FORWARDING_ADDRESS(*p);
      }
    }
  }
  for (p = (GC_cap_ptr *) start; ((uintptr_t) p) < ((uintptr_t) end); p++)
  {    
    if (GC_cheri_gettag(*p) && GC_IS_FORWARDING_ADDRESS(*p))
    {
      *p = GC_STRIP_FORWARDING(*p);
    }
  }
}
