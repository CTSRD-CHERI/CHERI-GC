#include "gc_debug.h"
#include "gc_low.h"

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
    (GC_ULL) GC_cheri_getbase(region.fromspace),
    (GC_ULL) GC_cheri_getlen(region.fromspace),
    (GC_ULL) GC_cheri_getbase(region.tospace),
    (GC_ULL) GC_cheri_getlen(region.tospace),
    (GC_ULL) GC_cheri_getbase(region.free),
    (GC_ULL) GC_cheri_getlen(region.free),
    (GC_ULL) GC_cheri_getbase(region.scan),
    (GC_ULL) GC_cheri_getlen(region.scan)
  );
}

#include <ucontext.h>
#include <string.h>

static void
GC_debug_print_stack_stats_helper (int arg)
{
  
  int local;
  
  void * stack_ptr = NULL;
  GC_GET_STACK_PTR(stack_ptr);
  GC_dbgf("Stack pointer: 0x%llx\n",
    (GC_ULL) stack_ptr);

  void * frame_ptr = NULL;
  GC_GET_FRAME_PTR(frame_ptr);
  GC_dbgf("Frame pointer: 0x%llx\n",
    (GC_ULL) frame_ptr);
  
  GC_dbgf("Address of a local variable: 0x%llx",
    (GC_ULL) &local);

  GC_dbgf("Address of an argument: 0x%llx",
    (GC_ULL) &arg);
  
  GC_dbgf("GC_get_stack_bottom(): 0x%llx\n",
    (GC_ULL) GC_state.stack_bottom);
}

void
GC_debug_print_stack_stats (void)
{
  GC_debug_print_stack_stats_helper(0);
}

void
GC_debug_capdump (const void * start, const void * end)
{
  if ( ((uintptr_t) end) < ((uintptr_t) start) )
  {
    const void * tmp = end;
    end = start;
    start = tmp;
  }
  
  printf("Capability memory dump from 0x%llx to 0x%llx\n",
    (GC_ULL) start,
    (GC_ULL) end);
  GC_ALIGN_32(start, const void *);
  GC_ALIGN_32_LOW(end, const void *);  
  
  const void * p;
  for (p = start;
       ((uintptr_t) p) < ((uintptr_t) end);
       p = (const void *) ( ((uintptr_t) p) + sizeof(GC_cap_ptr) ))
  {
    // Do a CLC to $c1, then get the tag
    GC_RESTORE_CAP_REG(1, p);
    unsigned tag = 0;
    GC_CHERI_CGETTAG(tag, 1);
    if (tag)
    {
      const void * base = NULL;
      size_t length = 0;
      GC_CHERI_CGETBASE(base, 1);
      GC_CHERI_CGETLEN(length, 1);
      
      printf("[0x%llx]  t=%u  b=0x%llx  l=0x%llx\n",
        (GC_ULL) p,
        tag,
        (GC_ULL) base,
        (GC_ULL) length);
    }
    else
    {
      printf("[0x%llx]  t=0\n",
        (GC_ULL) p);
    }
  }
}

void
GC_debug_memdump (const void * start, const void * end)
{
  const char * p,
             * start8 = (const void *) (((uintptr_t) start) & ~7),
             * end8 = (const void *) ((((uintptr_t) end)+8) & ~7);
  char buf[34];
  memset(buf, ' ', sizeof buf);
  buf[33] = '\0';
  size_t i = 0;

  printf("Memory dump from 0x%llx to 0x%llx\n",
    (GC_ULL) start,
    (GC_ULL) end);
  
  for (p = start8; p < end8; p++)
  {
    i = ((uintptr_t) p) % 8;
    int out_bounds = (p < (const char *) start || p > (const char *) end);
    buf[i*3+0] = out_bounds ? '?' : "0123456789ABCDEF"[(*p>>4) & 0xF];
    buf[i*3+1] = out_bounds ? '?': "0123456789ABCDEF"[*p & 0xF];
    buf[i+25] =  out_bounds ? '?' : (*p < 32 || *p > 126) ? '.' : *p;
    if (i == 7)
      printf("0x%llx: %s\n", (GC_ULL) ((uintptr_t) p) - 7, buf);
  }
}
