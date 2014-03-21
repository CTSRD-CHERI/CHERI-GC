#include "gc_collect.h"
#include "gc_init.h"
#include "gc_low.h"
#include "gc_debug.h"

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

  if (GC_is_young(region))
  {
    GC_dbgf("region is young, promoting objects...");
    GC_gen_promote(region);
  }
  else
  {
    GC_dbgf("region is old, collecting proper...");
    size_t old_size =
      GC_cheri_getbase(region->free) - GC_cheri_getbase(region->tospace);
    
    GC_cap_ptr tmp = region->fromspace;
    region->fromspace = region->tospace;
    region->tospace = tmp;
    
    region->free = region->tospace;
    region->scan = (GC_cap_ptr *) GC_cheri_getbase(region->tospace);
    
    GC_copy_region(region, 0);

    size_t new_size =
      GC_cheri_getbase(region->free) - GC_cheri_getbase(region->tospace);
    
    GC_dbgf("old_size=%d  new_size=%d  freed=%d",
        (int) old_size,
        (int) new_size,
        (int) (old_size - new_size));
  }
}

void
GC_copy_region (struct GC_region * region,
                int is_generational)
{ 
  // This information is from version 1.8 of the CHERI spec:
  // We ignore:
  //  $c0 because it spans the entire address space
  //  $c1 - $c2 because they are caller-save
  //  $c3 - $c10 because they are used to pass arguments
  //  $c11 - $c16 because they are also caller-save
  //  $c27 - $c31 because they are for kernel use 
  // We therefore don't ignore:
  //  $c17 - $c24 because they are callee-save
  //  $c25
  //  $c26 (IDC)

  // This information is from version 1.5 of the CHERI spec:
  // We ignore:
  //  $c0 because it spans the entire address space
  //  $c1 - $c4 because they are used to pass arguments and can be treated as
  //            clobbered
  //  $c5 - $c15, $c15 - $c23 because they are caller-save
  //  $c27 - $c31 because they are for kernel use 
  // We therefore don't ignore:
  //  $c16 - $c23 because they are callee-save
  //  $c24 - $c26 because whether they can be ignored is unknown
  
  // So to be conservative we save $c16 - $c26.
  
  int num_regs = 11;
  GC_cap_ptr cap_regs_misaligned[sizeof(GC_cap_ptr)*num_regs+32];
  GC_cap_ptr * cap_regs = cap_regs_misaligned;
  
  // CSC instruction needs 32-byte aligned destination address.
  cap_regs = GC_ALIGN_32(cap_regs, GC_cap_ptr *);
  
  GC_PUSH_CAP_REG(16, &cap_regs[0]);
  GC_PUSH_CAP_REG(17, &cap_regs[1]);
  GC_PUSH_CAP_REG(18, &cap_regs[2]);
  GC_PUSH_CAP_REG(19, &cap_regs[3]);
  GC_PUSH_CAP_REG(20, &cap_regs[4]);
  GC_PUSH_CAP_REG(21, &cap_regs[5]);
  GC_PUSH_CAP_REG(22, &cap_regs[6]);
  GC_PUSH_CAP_REG(23, &cap_regs[7]);
  GC_PUSH_CAP_REG(24, &cap_regs[8]);
  GC_PUSH_CAP_REG(25, &cap_regs[9]);
  GC_PUSH_CAP_REG(26, &cap_regs[10]);

  int i;
  for (i=0; i<num_regs; i++)
    if (GC_cheri_gettag(cap_regs[i]))
      GC_dbgf("cap_reg root [%d]: t=%d, b=0x%llx, l=0x%llx",
        i,
        (int) GC_cheri_gettag(cap_regs[i]),
        (GC_ULL) GC_cheri_getbase(cap_regs[i]),
        (GC_ULL) GC_cheri_getlen(cap_regs[i])
    );
  
  void * stack_top = NULL;
  GC_GET_STACK_PTR(stack_top);
  
  GC_assert(stack_top <= GC_state.stack_bottom);
 
  GC_copy_roots(
    region, stack_top, GC_state.stack_bottom, is_generational);
  GC_copy_roots(
    region, GC_state.static_bottom, GC_state.static_top, is_generational);
  GC_copy_children(region, is_generational);
  
  GC_RESTORE_CAP_REG(16, &cap_regs[0]);
  GC_RESTORE_CAP_REG(17, &cap_regs[1]);
  GC_RESTORE_CAP_REG(18, &cap_regs[2]);
  GC_RESTORE_CAP_REG(19, &cap_regs[3]);
  GC_RESTORE_CAP_REG(20, &cap_regs[4]);
  GC_RESTORE_CAP_REG(21, &cap_regs[5]);
  GC_RESTORE_CAP_REG(22, &cap_regs[6]);
  GC_RESTORE_CAP_REG(23, &cap_regs[7]);
  GC_RESTORE_CAP_REG(24, &cap_regs[8]);
  GC_RESTORE_CAP_REG(25, &cap_regs[9]);
  GC_RESTORE_CAP_REG(26, &cap_regs[10]);

  for (i=0; i<num_regs; i++)
    if (GC_cheri_gettag(cap_regs[i]))
      GC_dbgf("cap_reg root [%d]: t=%d, b=0x%llx, l=0x%llx",
        i,
        (int) GC_cheri_gettag(cap_regs[i]),
        (GC_ULL) GC_cheri_getbase(cap_regs[i]),
        (GC_ULL) GC_cheri_getlen(cap_regs[i])
    );
}

GC_cap_ptr
GC_copy_object (struct GC_region * region,
                GC_cap_ptr cap)
{
  // Copy the object pointed to by cap to region->tospace

  GC_assert(GC_cheri_getlen(cap) >= sizeof(GC_cap_ptr));
  
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
      GC_dbgf("[root] location=0x%llx (%s?), b=0x%llx, l=0x%llx\n",
        (GC_ULL) p,
        ((GC_ULL) p) & 0x7F00000000 ? "stack/reg" : ".data",
        (GC_ULL) *p, 
        (GC_ULL) GC_cheri_getlen(*p));
      *p = GC_copy_object(region, *p);
      if (is_generational)
        *p = GC_SET_OLD(*p);
    }
  }
}

void
GC_copy_children (struct GC_region * region, int is_generational)
{
  for (;
       ((uintptr_t) region->scan)
         < ((uintptr_t) GC_cheri_getbase(region->free));
       region->scan++)
  {
    if (GC_cheri_gettag(*region->scan)
        && GC_IN(GC_cheri_getbase(*region->scan), region->fromspace))
    {
      GC_dbgf("[child] location=0x%llx (%s?), b=0x%llx, l=0x%llx\n",
        (GC_ULL) region->scan,
        ((GC_ULL) region->scan) & 0x7F00000000 ? "stack/reg" : ".data",
        (GC_ULL) *region->scan, 
        (GC_ULL) GC_cheri_getlen(*region->scan));
      *region->scan = GC_copy_object(region, *region->scan);
      if (is_generational)
        *region->scan = GC_SET_CONTAINED_IN_OLD(*region->scan);
    }
  }
}

void
GC_gen_promote (struct GC_region * region)
{
  // Conservative estimate. Usually requires the old generation to have at least
  // as much space as the entire young generation (because GC_gen_promote is
  // usually called when the young generation is almost full).
  if (GC_cheri_getlen(region->older_region->free)
      < (GC_cheri_getbase(region->free) - GC_cheri_getbase(region->tospace)))
  {
    GC_dbgf("old generation too small, triggering major collection");
    GC_collect_region(region->older_region);
  }
  if (GC_cheri_getlen(region->older_region->free)
      < (GC_cheri_getbase(region->free) - GC_cheri_getbase(region->tospace)))
  {
    // TODO: grow old generation
    GC_errf("out of memory");
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
