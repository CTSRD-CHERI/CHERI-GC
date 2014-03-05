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
  GC_ALIGN_32(cap_regs, GC_cap_ptr *); // CSC instruction needs 32-byte aligned
                                       // destination address.
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
GC_collect_range (struct GC_region * region, void * start, void * end)
{
  
}