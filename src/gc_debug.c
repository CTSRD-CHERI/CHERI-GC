#include "gc_debug.h"

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <stdarg.h>
#include <stdio.h>

void
GC_dbgf2 (const char * file, int line, const char * format, ...)
{
  va_list vl;
  printf("[GC debug] %s:%d ", file, line);
  va_start(vl, format);
  vprintf(format, vl);
  va_end(vl);
  putchar('\n');
}

void
GC_errf2 (const char * file, int line, const char * format, ...)
{
  va_list vl;
  printf("[GC error] %s:%d ", file, line);
  va_start(vl, format);
  vprintf(format, vl);
  va_end(vl);
  putchar('\n');
}

void
GC_debug_print_region_stats(struct GC_region region)
{
  printf
  (
    "Region statistics\n"
    "-----------------\n"
    "fromspace :     base=0x%llx, len=0x%llx\n"
    "tospace   :     base=0x%llx, len=0x%llx\n"
    "free      :     base=0x%llx, len=0x%llx\n"
    "scan      :     base=0x%llx, len=0x%llx\n",
    (unsigned long long) cheri_getbase(region.fromspace),
    (unsigned long long) cheri_getlen(region.fromspace),
    (unsigned long long) cheri_getbase(region.tospace),
    (unsigned long long) cheri_getlen(region.tospace),
    (unsigned long long) cheri_getbase(region.free),
    (unsigned long long) cheri_getlen(region.free),
    (unsigned long long) cheri_getbase(region.scan),
    (unsigned long long) cheri_getlen(region.scan)
  );
}

#include <ucontext.h>
#include <string.h>

static void
GC_debug_print_stack_stats_helper (int arg)
{
  // The stack looks like this:
  // ----high memory addresses----
  // function arguments
  // $ra
  // $fp
  // preserved registers
  // local variables
  // $gp
  // ----low memory addresses----
  // $sp and $fp point just beneath $gp during the call.
  // i.e., the stack grows down.
  
  int local;
  
  #define GC_GET_STACK_PTR(ret) \
    __asm __volatile \
    ( \
      "daddiu %0, $sp, 0" : "=r"(ret) : : \
    )
  
  #define GC_GET_FRAME_PTR(ret) \
    __asm __volatile \
    ( \
      "daddiu %0, $fp, 0" : "=r"(ret) : : \
    )
  
  void * stack_ptr = NULL;
  GC_GET_STACK_PTR(stack_ptr);
  GC_dbgf("Stack pointer: 0x%llx\n",
    (unsigned long long) stack_ptr);

  void * frame_ptr = NULL;
  GC_GET_FRAME_PTR(frame_ptr);
  GC_dbgf("Frame pointer: 0x%llx\n",
    (unsigned long long) frame_ptr);
  
  GC_dbgf("Address of a local variable: 0x%llx",
    (unsigned long long) &local);

  GC_dbgf("Address of an argument: 0x%llx",
    (unsigned long long) &arg);
  

  // Doesn't seem to work on FreeBSD...
  ucontext_t ucp;
  memset(&ucp, 0, sizeof ucp);
  if (getcontext(&ucp))
  {
    GC_errf("getcontext");
  }
  else
  {
    GC_dbgf("getcontext stack bottom: 0x%llx\n"
            "getcontext stack size:   0x%llx\n",
            (unsigned long long) ucp.uc_stack.ss_sp,
            (unsigned long long) ucp.uc_stack.ss_size
    );
  }
  
  GC_dbgf("GC_get_stack_bottom(): 0x%llx\n",
    (unsigned long long) GC_state.stack_bottom);

}

void
GC_debug_print_stack_stats (void)
{
  GC_debug_print_stack_stats_helper(0);
}
