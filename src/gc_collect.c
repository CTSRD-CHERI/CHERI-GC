#include "gc_collect.h"
#include "gc_init.h"
#include "gc_low.h"
#include "gc_debug.h"
#include "gc_config.h"
#include "gc_time.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

void
GC_collect (void)
{
  int local;
  //GC_SAVE_STACK_PTR
  //GC_state.stack_top = GC_MAX_STACK_TOP;
  //GC_assert( GC_state.stack_top < (void*)&file ); // check we haven't overflowed the stack

  GC_state.stack_top = &local;
  
  //GC_CLOBBER_CAP_REGS();
  GC_SAVE_REG_STATE();
  
  //GC_CLEAN_STACK();
  
  if (!GC_is_initialized())
  {
    //GC_init();
    GC_fatalf("GC not initialized, call GC_init from main.");
  }
  GC_collect_region(&GC_state.thread_local_region);
  
  GC_CLEAN_STACK();
  GC_RESTORE_REG_STATE();
}

void
GC_collect_region (struct GC_region * region)
{
  GC_START_TIMING(GC_collect_region_time);
   
  GC_assert( GC_state.stack_top && GC_state.reg_bottom && GC_state.reg_top );

  GC_assert( GC_state.stack_top > GC_MAX_STACK_TOP );

  region->num_collections++; // debugging/stats
  
  void * space_start = GC_cheri_getbase(region->tospace);
  void * space_end = space_start + GC_cheri_getlen(region->tospace);
  GC_debug_begin_marking();
  
  GC_assert(
    GC_IN_OR_ON_BOUNDARY(GC_cheri_getbase(region->free), region->tospace) );
  
  GC_CLEAN_STACK();
  
  size_t freed;
  int promoted = 0;
#ifdef GC_GENERATIONAL
  if (GC_is_young(region))
  {
    GC_vdbgf("region is young, promoting objects...");
    size_t old_used =
      GC_cheri_getbase(region->free) - GC_cheri_getbase(region->tospace);
    
    GC_gen_promote(region);
    
    size_t new_used =
      GC_cheri_getbase(region->free) - GC_cheri_getbase(region->tospace);
    
    // Current policy dictates that GC_gen_promote frees everything.
    GC_assert( new_used == 0 );
    
    freed = old_used - new_used;
    promoted = 1;
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
    
    void * tmp_misaligned = region->fromspace_misaligned;
    region->fromspace_misaligned = region->tospace_misaligned;
    region->tospace_misaligned = tmp_misaligned;
    
    region->free = region->tospace;
    region->scan = (GC_cap_ptr *) GC_cheri_getbase(region->tospace);
    
    GC_copy_region(region, 0);

    size_t new_used =
      GC_cheri_getbase(region->free) - GC_cheri_getbase(region->tospace);
    
    freed = old_used - new_used;
    
    GC_vdbgf(
      "(old) generation  old_used=%llu%s  new_used=%llu%s  freed=%llu%s",
      GC_MEM_PRETTY((GC_ULL) old_used), GC_MEM_PRETTY_UNIT((GC_ULL) old_used),
      GC_MEM_PRETTY((GC_ULL) new_used), GC_MEM_PRETTY_UNIT((GC_ULL) new_used),
      GC_MEM_PRETTY((GC_ULL) freed), GC_MEM_PRETTY_UNIT((GC_ULL) freed));
  }

  GC_debug_end_marking(space_start, space_end);

  GC_STOP_TIMING_PRINT(
    GC_collect_region_time,
    "GC_collect_region %s %llu%s,",
    promoted ? "promoted" : "freed   ",
    GC_MEM_PRETTY((GC_ULL) freed), GC_MEM_PRETTY_UNIT((GC_ULL) freed));
  
}

void
GC_copy_region (struct GC_region * region,
                int is_generational)
{ 
  //GC_PUSH_CAP_REGS(cap_regs);
  
  /*int i;
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
  }*/
  
  //GC_vdbgf("The registers lie between 0x%llx and 0x%llx",
    //(GC_ULL) &cap_regs[0], (GC_ULL) &cap_regs[GC_NUM_CAP_REGS-1]);
  
  //void * stack_top = NULL;
  //GC_GET_STACK_PTR(stack_top);

  GC_dbgf("The registers lie between 0x%llx and 0x%llx",
    (GC_ULL) GC_state.reg_bottom, (GC_ULL) GC_state.reg_top);
  GC_dbgf("The stack to scan lies between 0x%llx and 0x%llx",
    (GC_ULL) GC_state.stack_top, (GC_ULL) GC_state.stack_bottom);
  
  GC_assert( GC_state.stack_top && GC_state.reg_bottom && GC_state.reg_top );
  
  GC_assert( GC_state.stack_top <= GC_state.stack_bottom );
    
  GC_copy_roots(
    region, GC_state.stack_top, GC_state.stack_bottom, is_generational, 0);
  GC_copy_roots(
    region, GC_state.reg_bottom, GC_state.reg_top, is_generational, 0);
  GC_copy_roots(
    region, GC_state.static_bottom, GC_state.static_top, is_generational, 1);
  GC_copy_children(region, is_generational);
#ifdef GC_GENERATIONAL
#if (GC_OY_STORE_DEFAULT == GC_OY_STORE_REMEMBERED_SET)
  GC_copy_remembered_set(region);
#endif // GC_OY_STORE_DEFAULT
#endif // GC_GENERATIONAL

  // TODO: ensure no forwarding addresses left in registers.
  /*GC_clean_forwarding(
    GC_cheri_getbase(region->fromspace),
    GC_cheri_getbase(region->fromspace) + GC_cheri_getlen(region->fromspace));*/
  GC_cap_memclr(region->fromspace);
}

GC_cap_ptr
GC_copy_object (struct GC_region * region,
                GC_cap_ptr cap,
                void * parent) // parent for debugging only
{
  // Copy the object pointed to by cap to region->tospace
  
  GC_assert( GC_IS_GC_ALLOCATED(cap) );
  GC_assert( GC_IN(GC_cheri_getbase(cap), region->fromspace) );
  GC_assert( GC_IS_ALIGNED_32(cap) );
  GC_assert( GC_cheri_gettag(cap) );
  GC_assert( !GC_IS_FORWARDING_ADDRESS(cap) );
  
  // Need enough space for forwarding pointer; GC_malloc ensures that this is
  // always true, but the capability may present itself as having a smaller
  // length due to the semantics of GC_malloc.
  size_t user_length = GC_cheri_getlen(cap);
  GC_cap_ptr orig_cap = cap;
  cap = GC_setbaselen(
    cap, GC_cheri_getbase(cap), GC_ALIGN_32(user_length, size_t));

  // If the first word of the object points to a place in region->tospace,
  // then it has already been copied.
  
  // $c2 = *(cap.base + forwarding_address_offset) = *($c0.base + forwarding_address_field_location);
  GC_cap_ptr forwarding_address = GC_FORWARDING_CAP(cap);
  
  unsigned tag = GC_cheri_gettag(forwarding_address);
  if (tag)
  {
    if (GC_IS_FORWARDING_ADDRESS(forwarding_address))
    {
      GC_assert(GC_IN(GC_cheri_getbase(forwarding_address), region->tospace));
      GC_cap_ptr tmp = GC_STRIP_FORWARDING(forwarding_address);
      GC_assert(!GC_IS_FORWARDING_ADDRESS(tmp));
      return tmp;
    }
  }
  
  // No forwarding address present: do the actual copy and set the forwarding
  // address.

  GC_assert( GC_cheri_getlen(region->free) >= GC_cheri_getlen(cap) );
    
  GC_cap_ptr tmp = GC_cap_memcpy(region->free, cap);

  tmp = GC_cheri_setlen(tmp, user_length);
  region->free =
    GC_setbaselen(
      region->free,
      GC_cheri_getbase(region->free)+GC_cheri_getlen(cap),
      GC_cheri_getlen (region->free)-GC_cheri_getlen(cap));

  tmp = GC_SET_GC_ALLOCATED(tmp);
  
#ifndef GC_GENERATIONAL
  // Not true always; consider young-old copy
  GC_assert( GC_cheri_getperm(tmp) == GC_cheri_getperm(cap) );
#endif // GC_GENERATIONAL

#ifdef GC_DEBUG
  // Clobber the old cap with a magic value, for debugging
  GC_cap_memset(cap, GC_MAGIC_JUST_COPIED);
#endif // GC_DEBUG
  
  // Set the forwarding address of the old object.
  GC_FORWARDING_CAP(cap) = GC_MAKE_FORWARDING_ADDRESS(tmp);
 
  GC_debug_just_copied(orig_cap, tmp, parent);
  
  //GC_assert( GC_IS_GC_ALLOCATED(tmp) );
  
  GC_assert(!GC_IS_FORWARDING_ADDRESS(tmp));

  return tmp;
}

void
GC_copy_roots (struct GC_region * region,
               void * root_start,
               void * root_end,
               int is_generational,
               int is_data_segment)
{
  // TODO: ignore roots in the GC's call stack. There shouldn't be any
  // (unintended) roots in the GC's call stack unless a capability argument is
  // passed in any calls up to this call or local capability variables are
  // declared.
  
  GC_assert(root_start <= root_end);
  
  root_start = GC_ALIGN_32(root_start, void *);
  root_end = GC_ALIGN_32_LOW(root_end, void *);
  
  GC_vdbgf("copying roots in range 0x%llx to 0x%llx",
    (GC_ULL) root_start, (GC_ULL) root_end);
  
  GC_cap_ptr * p;
  for (p = (GC_cap_ptr *) root_start;
       ((uintptr_t) p) < ((uintptr_t) root_end);
       p++)
  {
    if (is_data_segment && GC_IN(p, GC_state_cap))
    {
      continue;
    }
    if (GC_cheri_gettag(*p)
        && GC_IS_GC_ALLOCATED(*p)
        && GC_IN(GC_cheri_getbase(*p), region->fromspace))
    {
      GC_vdbgf("[root] [%d%%] location=0x%llx (%s?), b=0x%llx, l=0x%llx",
      (int) (
        100.0*
          ((double) (((uintptr_t) p)-((uintptr_t) root_start)))/
          ((double) (((uintptr_t) root_end)-((uintptr_t) root_start)))),
        (GC_ULL) p,
        (((GC_ULL) p) > 0x7F00000000) ? "stack/reg" :
        is_data_segment ? ".data" : "unk.",
        (GC_ULL) *p,
        (GC_ULL) GC_cheri_getlen(*p));
      *p = GC_copy_object(region, *p, p);
      
      GC_assert( GC_IN(GC_cheri_getbase(*p), region->tospace) );
      
#ifdef GC_GENERATIONAL
      if (is_generational)
      {
        GC_SWITCH_WB_TYPE(
          {*p = GC_UNSET_YOUNG(*p);},      // GC_WB_MANUAL
          {*p = GC_UNSET_EPHEMERAL(*p);}   // GC_WB_EPHEMERAL
        );
      }
#endif // GC_GENERATIONAL
    }
  }
  
  GC_vdbgf("roots copied");
}

void
GC_copy_child (struct GC_region * region,
               GC_cap_ptr * child_addr,
               int is_generational)
{
  if (GC_cheri_gettag(*child_addr)
    && GC_IS_GC_ALLOCATED(*child_addr)
       // necessary now for remembered set too
    && GC_IN(GC_cheri_getbase(*child_addr), region->fromspace))
  {
    GC_vdbgf("[child] location=0x%llx, b=0x%llx, l=0x%llx",
      (GC_ULL) child_addr,
      (GC_ULL) *child_addr, 
      (GC_ULL) GC_cheri_getlen(*child_addr));
    *child_addr = GC_copy_object(region, *child_addr, child_addr);
#ifdef GC_GENERATIONAL
    if (is_generational)
    {
      GC_SWITCH_WB_TYPE(
        {*child_addr =
          GC_UNSET_YOUNG(
          GC_SET_CONTAINED_IN_OLD(*child_addr));},  // GC_WB_MANUAL
        {*child_addr =
          GC_UNSET_EPHEMERAL(*child_addr);}         // GC_WB_EPHEMERAL
      );
    }
#endif // GC_GENERATIONAL
  }
}

void
GC_copy_children (struct GC_region * region,
                  int is_generational)
{
  GC_vdbgf("copying children");
  
  GC_assert( GC_IS_ALIGNED_32(region->scan) );
  
  for (;
       ((uintptr_t) region->scan)
         < ((uintptr_t) GC_cheri_getbase(region->free));
       region->scan++)
  {
    GC_copy_child(region, region->scan, is_generational);
  }
}

#ifdef GC_GENERATIONAL
#if (GC_OY_STORE_DEFAULT == GC_OY_STORE_REMEMBERED_SET)
void
GC_copy_remembered_set (struct GC_region * region)
{
  GC_vdbgf("copying %d roots from remset", (int) region->remset.nroots);
  size_t i;
  for (i=0; i<region->remset.nroots; i++)
  {
    GC_cap_ptr * root = (GC_cap_ptr *) region->remset.roots[i];
    GC_assert( root );
    GC_dbgf("[%d] Processing remembered root 0x%llx",
      (int) i, (GC_ULL) root);
    GC_copy_child(region, root, 1);
  }
  GC_remembered_set_clr(&region->remset);
}
#endif // GC_OY_STORE_DEFAULT

void
GC_gen_promote (struct GC_region * region)
{
  
  // Conservative estimate. Usually requires the old generation to have at least
  // as much space as the entire young generation (because GC_gen_promote is
  // usually called when the young generation is almost full).
  ptrdiff_t expected_sz =
    GC_cheri_getbase(region->free) - GC_cheri_getbase(region->tospace);
  int too_small = GC_cheri_getlen(region->older_region->free) < expected_sz;
  
#ifdef GC_GROW_OLD_HEAP
  // Grow the heap if we have to.
  if (too_small)
  {
    // UNSAFE. It's only safe to grow the heap if all pointers to stuff inside
    // it are on the stack, in registers, in global areas or in the tospace
    // itself. This ignores young-old pointers!
    //GC_fatalf("UNSAFE growth here.");
    //GC_vdbgf("old generation too small, trying to grow");
    //too_small = !GC_grow(region->older_region, expected_sz,
    //                   region->max_grow_size_before_collection);
  }
#endif // GC_GROW_OLD_HEAP
  if (too_small)
  {
    GC_errf("old generation out of memory");
    return;
  }
  
  GC_vdbgf("old generation ok, copying up to %llu%s",
    GC_MEM_PRETTY((GC_ULL) expected_sz),
    GC_MEM_PRETTY_UNIT((GC_ULL) expected_sz));
  
  GC_cap_ptr old_from_space = region->older_region->fromspace;
  region->older_region->fromspace = region->tospace;
  
  // ensure we only scan the young region's children when copying
  // NOTE: this alignment is OK to go downwards because we can assume that the
  // minimum address region->free can have is 32-byte aligned.
  region->older_region->scan =
    GC_ALIGN_32_LOW((GC_cap_ptr *) region->older_region->free, GC_cap_ptr *);
  
  void * oldfree = GC_cheri_getbase(region->older_region->free);
  
  GC_copy_region(region->older_region, 1);
  
  region->older_region->fromspace = old_from_space;
  region->free = region->tospace;
  
  void * newfree = GC_cheri_getbase(region->older_region->free);
  size_t freelen = GC_cheri_getlen(region->older_region->free);
  size_t usedlen = GC_cheri_getlen(region->older_region->tospace) - freelen;
  ptrdiff_t szdiff = newfree - oldfree;
  
  GC_vdbgf("copied %llu%s (%llu bytes less than expected) into the old generation",
    GC_MEM_PRETTY((GC_ULL) szdiff), GC_MEM_PRETTY_UNIT((GC_ULL) szdiff),
    (GC_ULL) (expected_sz - szdiff));
  
  int residency = 
    (int) (100.0 * (1.0-((double) freelen) /
      ((double) GC_cheri_getlen(region->older_region->tospace))));
  
  GC_vdbgf(
    "total heap residency after promote: %llu kbytes (%llu%s, %d%%)\n",
    (GC_ULL) (usedlen / 1000),
    GC_MEM_PRETTY((GC_ULL) usedlen), GC_MEM_PRETTY_UNIT((GC_ULL) usedlen),
    residency
  );
  
  // Collect (and possibly grow) the heap if we've hit the high watermark. This
  // should usually be set so that if the collection and/or growth succeeds, the
  // next promotion cannot possibly fail. That is, we should always have at
  // least enough space to promote the young generation.
  too_small = GC_cheri_getlen(region->older_region->free) <
              ((size_t) ((1.0-GC_OLD_GENERATION_HIGH_WATERMARK) *
                (double)GC_cheri_getlen(region->older_region->tospace)));
  if (too_small)
  {
    GC_vdbgf(
      "old generation hit high watermark (residency=%d%%, watermark=%d%%),"
      " triggering major collection",
      residency,
      (int) (100.0 * GC_OLD_GENERATION_HIGH_WATERMARK)
    );
    GC_collect_region(region->older_region);
    
    residency = 
        (int) (100.0 * (1.0 -
          ((double) GC_cheri_getlen(region->older_region->free)) /
          ((double) GC_cheri_getlen(region->older_region->tospace))));
    GC_vdbgf("after collection heap residency=%d%%", residency);

    too_small = GC_cheri_getlen(region->older_region->free) <
                ((size_t) ((1.0-GC_OLD_GENERATION_HIGH_WATERMARK) *
                  (double)GC_cheri_getlen(region->older_region->tospace)));
    
    if (too_small)
    {
#ifdef GC_GROW_OLD_HEAP
      // always grow the heap, even if collection succeeded in bringing the size
      // down, so that it hopefully doesn't happen again.
      GC_dbgf("trying to grow old generation");
      too_small = !GC_grow(region->older_region,
                           GC_cheri_getlen(region->older_region->free),
                           region->max_grow_size_after_collection);
      residency = 
          (int) (100.0 * (1.0 -
            ((double) GC_cheri_getlen(region->older_region->free)) /
            ((double) GC_cheri_getlen(region->older_region->tospace))));
      GC_dbgf("after growth heap residency=%d%%", residency);
#endif // GC_GROW_OLD_HEAP
    }
  }

  if (too_small)
  {
    GC_dbgf("warning: old generation heap residency above high watermark, at"
            " %d%%. (D)OOM approaches.", residency);
  }

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
  
  GC_START_TIMING(GC_region_rebase_time);
  
  // Make sure to clean the stack after this. Currently GC_malloc does it.
  // TODO: check if the stack really does need cleaning after this.
  
  GC_dbgf("rebasing region\n");
  
  GC_dbgf(
    "\n"
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
  
  //GC_PUSH_CAP_REGS(cap_regs);
  
  // Now unneeded, because GC_IS_GC_ALLOCATED(region->free) is not true.
  // TODO: make sure this stays on the stack!  
  //GC_cap_ptr free_ptr_on_the_stack = region->free;
  GC_assert( !GC_IS_GC_ALLOCATED(region->free) );
  
  //void * stack_top = NULL;
  //GC_GET_STACK_PTR(stack_top);
  
  GC_dbgf("The registers lie between 0x%llx and 0x%llx",
    (GC_ULL) GC_state.reg_bottom, (GC_ULL) GC_state.reg_top);
  GC_dbgf("The stack to scan lies between 0x%llx and 0x%llx",
    (GC_ULL) GC_state.stack_top, (GC_ULL) GC_state.stack_bottom);
  
  GC_assert( GC_state.stack_top && GC_state.reg_bottom && GC_state.reg_top );

  GC_assert( GC_state.stack_top <= GC_state.stack_bottom );

  GC_PRINT_CAP(region->free);
  GC_PRINT_CAP(region->tospace);

  GC_assert(
    (GC_cheri_getbase(region->free) >= old_base) &&
    (GC_cheri_getbase(region->free) <= (old_base+old_size))
  );
  
  // These areas must not overlap.
  GC_rebase(GC_state.stack_top, GC_state.stack_bottom,
            old_base, old_size, new_base);
  GC_rebase(GC_state.reg_bottom, GC_state.reg_top,
            old_base, old_size, new_base);
  GC_rebase(GC_state.static_bottom, GC_state.static_top,
            old_base, old_size, new_base);
  GC_rebase(new_base, new_base+new_size,
            old_base, old_size, new_base);
  
  /*
  GC_clean_forwarding(stack_top, GC_state.stack_bottom);
  GC_clean_forwarding(GC_state.static_bottom, GC_state.static_top);
  GC_clean_forwarding(new_base, new_base+new_size);
  */
  
#ifdef GC_GENERATIONAL
#if (GC_OY_STORE_DEFAULT == GC_OY_STORE_REMEMBERED_SET)
  size_t i;
  for (i=0; i<region->remset.nroots; i++)
  {
    GC_cap_ptr * root = (GC_cap_ptr *) region->remset.roots[i];
    GC_assert( root );
    GC_dbgf("[%d] Rebasing remembered root 0x%llx",
      (int) i, (GC_ULL) root);
    if (GC_cheri_gettag(*root))
    {
      void * base = GC_cheri_getbase(*root);
      if ((base >= old_base) && (base <= (old_base+old_size)))
      {
        *root = GC_setbase(*root, (base-old_base)+new_base);
      }
    }
  }
#endif // GC_OY_STORE_DEFAULT
#endif // GC_GENERATIONAL

  //region->free = free_ptr_on_the_stack;
  
  // is this OK?
  region->scan = ((void*)region->scan)-old_base+new_base;
  
  GC_assert(
    (GC_cheri_getbase(region->free) >= old_base) &&
    (GC_cheri_getbase(region->free) <= (old_base+old_size))
  );
  
  // used to fail due to aliasing, but now ok due to GC_IS_GC_ALLOCATED
  region->free =
    GC_setbase(
      region->free,
      GC_cheri_getbase(region->free) - old_base + new_base);
  
  GC_assert(
    GC_IN_OR_ON_BOUNDARY(GC_cheri_getbase(region->free), region->tospace) );
  
  //GC_RESTORE_CAP_REGS(cap_regs);
  
  GC_STOP_TIMING_PRINT(GC_region_rebase_time, "region rebase");
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

  // Pointers to old_base+old_size are *always* re-based. We assume that
  // previously, any pointer to old_base+old_size was used solely to mark the
  // end of the region. This should almost always be true. The free pointer
  // regularly points there. Let's just hope nothing's actually allocated there
  // of any use.
  
  GC_cap_ptr * p;
  for (p = (GC_cap_ptr *) start; ((uintptr_t) p) < ((uintptr_t) end); p++)
  {
    if (GC_cheri_gettag(*p)
        && GC_IS_GC_ALLOCATED(*p)
       )//&& !GC_IS_FORWARDING_ADDRESS(*p))
    {
      if (GC_IN(p, GC_state_cap))
      {
        continue;
      }
      void * base = GC_cheri_getbase(*p);
      if ((base >= old_base) && (base <= (old_base+old_size)))
      {
        if (base == (old_base+old_size))
          GC_vdbgf("Warning: on the old_base+old_size edge case 0x%llx.",
                   (GC_ULL) base);
        *p = GC_setbase(*p, (base-old_base)+new_base);
        
        //GC_assert( GC_IS_GC_ALLOCATED(*p) );
        
        //*p = GC_MAKE_FORWARDING_ADDRESS(*p);
      }
    }
  }
  
  //GC_clean_forwarding(start, end);
}

void GC_clean_forwarding (void * start,
                          void * end)
{
  start = GC_ALIGN_32(start, void *);
  end = GC_ALIGN_32_LOW(end, void *);
  
  GC_cap_ptr * p;
  for (p = (GC_cap_ptr *) start; p < (GC_cap_ptr *) end; p++)
  {    
    if (GC_IN(p, GC_state_cap))
    {
      continue;
    }
    if (GC_cheri_gettag(*p) && GC_IS_FORWARDING_ADDRESS(*p))
    {
      *p = GC_STRIP_FORWARDING(*p);
    }
  }
}
