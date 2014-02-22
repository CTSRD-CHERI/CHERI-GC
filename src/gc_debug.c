#include "gc_debug.h"

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
    "fromspace :     0x%llx\n"
    "tospace   :     0x%llx\n"
    "free      :     0x%llx (0x%llx bytes into fromspace)\n"
    "scan      :     0x%llx (0x%llx bytes into fromspace)\n",
    (GC_ULL) region.fromspace,
    (GC_ULL) region.tospace,
    (GC_ULL) region.free,
    (GC_ULL) ( ((GC_ULL)region.free) > ((GC_ULL)region.fromspace) ? ((GC_ULL)region.free) - ((GC_ULL)region.fromspace) : 0 ),
    (GC_ULL) region.scan,
    (GC_ULL) ( ((GC_ULL)region.scan) > ((GC_ULL)region.fromspace) ? ((GC_ULL)region.scan) - ((GC_ULL)region.fromspace) : 0 )
  );
}
