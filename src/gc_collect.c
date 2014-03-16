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
  
  // Push the roots (currently only the capability registers) to the stack

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

// Works, but compiles to an excessively large number of instructions
  /*cap_regs[1] = GC_cheri_getreg(1);
  cap_regs[2] = GC_cheri_getreg(2);
  cap_regs[3] = GC_cheri_getreg(3);
  cap_regs[4] = GC_cheri_getreg(4);
  cap_regs[5] = GC_cheri_getreg(5);
  cap_regs[6] = GC_cheri_getreg(6);
  cap_regs[7] = GC_cheri_getreg(7);
  cap_regs[8] = GC_cheri_getreg(8);
  cap_regs[9] = GC_cheri_getreg(9);
  cap_regs[10] = GC_cheri_getreg(10);
  cap_regs[11] = GC_cheri_getreg(11);
  cap_regs[12] = GC_cheri_getreg(12);
  cap_regs[13] = GC_cheri_getreg(13);
  cap_regs[14] = GC_cheri_getreg(14);
  cap_regs[15] = GC_cheri_getreg(15);
  cap_regs[16] = GC_cheri_getreg(16);
  cap_regs[17] = GC_cheri_getreg(17);
  cap_regs[18] = GC_cheri_getreg(18);
  cap_regs[19] = GC_cheri_getreg(19);
  cap_regs[20] = GC_cheri_getreg(20);
  cap_regs[21] = GC_cheri_getreg(21);
  cap_regs[22] = GC_cheri_getreg(22);
  cap_regs[23] = GC_cheri_getreg(23);
  cap_regs[24] = GC_cheri_getreg(24);
  cap_regs[25] = GC_cheri_getreg(25);
  cap_regs[26] = GC_cheri_getreg(26);*/
  
  int i;
  for (i=0; i<num_regs; i++)
    GC_dbgf("cap_reg root [%d]: t=%d, b=0x%llx, l=0x%llx\n",
      i,
      (int) GC_cheri_gettag(cap_regs[i]),
      (GC_ULL) GC_cheri_getbase(cap_regs[i]),
      (GC_ULL) GC_cheri_getlen(cap_regs[i])
    );

  GC_debug_print_stack_stats();
  
  void * stack_top = NULL;
  GC_GET_STACK_PTR(stack_top);
  
  GC_assert(stack_top <= GC_state.stack_bottom);
  
  GC_dbgf("looking for roots between 0x%llx and 0x%llx\n",
      (GC_ULL) stack_top,
      (GC_ULL) GC_state.stack_bottom);
      
  GC_collect_range(region, stack_top, GC_state.stack_bottom);
}

void
GC_collect_range (struct GC_region * region, const void * root_start,
                  const void * root_end)
{
  GC_assert(root_start <= root_end);
  
  region->scan = region->tospace;
  region->free = region->tospace;
  
  root_start = GC_ALIGN_32(root_start, const void *);
  root_end = GC_ALIGN_32_LOW(root_end, const void *);  
  
  const void * p;
  for (p = root_start;
       ((uintptr_t) p) < ((uintptr_t) root_end);
       p = (const void *) ( ((uintptr_t) p) + sizeof(GC_cap_ptr) ))
  {
    // Do a CLC to $c1, then get the tag
    GC_RESTORE_CAP_REG(1, p);
    unsigned tag = 0;
    GC_CHERI_CGETTAG(tag, 1);
    if (tag)
    {
      GC_cap_ptr cap;
      cap = GC_cheri_getreg(1);
      
      GC_assert(GC_cheri_getlen(cap) >= sizeof(GC_cap_ptr));

      // Copy the object pointed to by cap to region->tospace
      
      // If the first word of the object points to a place in region->tospace,
      // then it has already been copied.
      
      void * base1;
      GC_CHERI_CGETBASE(base1, 1);
      printf("Okay, the base of this thing is 0x%llx and so the forwarding address is 0x%llx\n", (GC_ULL) base1, (GC_ULL) GC_FORWARDING_ADDRESS(cap));
      
      // $c2 = *($c1.base + forwarding_offset) = *($c0.base + forwarding_address);
      GC_RESTORE_CAP_REG(2, GC_FORWARDING_ADDRESS(cap));
      
      printf("Loaded $c1.base+forwarding_offset into $c2.\n");
      
      GC_CHERI_CGETTAG(tag, 2);
      if (tag)
      {
        const void * base;
        GC_CHERI_CGETBASE(base, 2);
        printf("The forwarding address contains a valid capability with base 0x%llx, but is it in the tospace?\n",
            (GC_ULL) base);
        if (GC_IN(base, region->tospace))
        {
          //cap = GC_forwarding_address(cap);
          printf("The forwarding address 0x%llx is inside the tospace!\n",
              (GC_ULL) base);
        }
      }
          //if (forwarded(cap)) new_root_cap = forwarding_address(cap);
          //else {new_root_cap=region->free; move(cap, region->free); region->free += size(cap); forwarding_address(cap) = new_root_cap;}
      return;
      printf("[0x%llx]  t=%u  b=0x%llx  l=0x%llx\n",
        (GC_ULL) p,
        tag,
        (GC_ULL) GC_cheri_getbase(cap),
        (GC_ULL) GC_cheri_getlen(cap));
    }
  }
  
}