#ifndef GC_DEBUG_H_HEADER
#define GC_DEBUG_H_HEADER

#define GC_dbgf(...)  GC_dbgf2(__FILE__, __LINE__, __VA_ARGS__)
#define GC_errf(...)  GC_errf2(__FILE__, __LINE__, __VA_ARGS__)

void
GC_dbgf2 (const char * file, int line, const char * format, ...);

void
GC_errf2 (const char * file, int line, const char * format, ...);

#include "gc_init.h"

void
GC_debug_print_region_stats (struct GC_region region);

#endif // GC_DEBUG_H_HEADER
