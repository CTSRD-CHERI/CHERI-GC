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
   
  __capability void * cap_regs_misaligned[sizeof(__capability void *)*26+32];
  GC_cap_ptr * cap_regs = cap_regs_misaligned;
  
  // CSC instruction needs 32-byte aligned destination address.
  cap_regs = GC_ALIGN_32(cap_regs, GC_cap_ptr *);
  
  GC_PUSH_CAP_REG(1, &cap_regs[0]);
  GC_PUSH_CAP_REG(2, &cap_regs[1]);
  GC_PUSH_CAP_REG(3, &cap_regs[2]);
  GC_PUSH_CAP_REG(4, &cap_regs[3]);
  GC_PUSH_CAP_REG(5, &cap_regs[4]);
  GC_PUSH_CAP_REG(6, &cap_regs[5]);
  GC_PUSH_CAP_REG(7, &cap_regs[6]);
  GC_PUSH_CAP_REG(8, &cap_regs[7]);
  GC_PUSH_CAP_REG(9, &cap_regs[8]);
  GC_PUSH_CAP_REG(10, &cap_regs[9]);
  GC_PUSH_CAP_REG(11, &cap_regs[10]);
  GC_PUSH_CAP_REG(12, &cap_regs[11]);
  GC_PUSH_CAP_REG(13, &cap_regs[12]);
  GC_PUSH_CAP_REG(14, &cap_regs[13]);
  GC_PUSH_CAP_REG(15, &cap_regs[14]);
  GC_PUSH_CAP_REG(16, &cap_regs[15]);
  GC_PUSH_CAP_REG(17, &cap_regs[16]);
  GC_PUSH_CAP_REG(18, &cap_regs[17]);
  GC_PUSH_CAP_REG(19, &cap_regs[18]);
  GC_PUSH_CAP_REG(20, &cap_regs[19]);
  GC_PUSH_CAP_REG(21, &cap_regs[20]);
  GC_PUSH_CAP_REG(22, &cap_regs[21]);
  GC_PUSH_CAP_REG(23, &cap_regs[22]);
  GC_PUSH_CAP_REG(24, &cap_regs[23]);
  GC_PUSH_CAP_REG(25, &cap_regs[24]);
  GC_PUSH_CAP_REG(26, &cap_regs[25]);
  
  int i;
  for (i=0; i<26; i++)
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