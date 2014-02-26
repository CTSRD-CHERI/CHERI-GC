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
  #define GC_ULL    unsigned long long
  printf
  (
    "Region statistics\n"
    "-----------------\n"
    "fromspace :     base=0x%llx, len=0x%llx\n"
    "tospace   :     base=0x%llx, len=0x%llx\n"
    "free      :     base=0x%llx, len=0x%llx\n"
    "scan      :     base=0x%llx, len=0x%llx\n",
    (GC_ULL) cheri_getbase(region.fromspace),
    (GC_ULL) cheri_getlen(region.fromspace),
    (GC_ULL) cheri_getbase(region.tospace),
    (GC_ULL) cheri_getlen(region.tospace),
    (GC_ULL) cheri_getbase(region.free),
    (GC_ULL) cheri_getlen(region.free),
    (GC_ULL) cheri_getbase(region.scan),
    (GC_ULL) cheri_getlen(region.scan)
  );
}
