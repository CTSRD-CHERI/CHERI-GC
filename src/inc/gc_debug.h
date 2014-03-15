#ifndef GC_DEBUG_H_HEADER
#define GC_DEBUG_H_HEADER

#include "gc_init.h"

#define GC_dbgf(...)  GC_dbgf2(__FILE__, __LINE__, __VA_ARGS__)
#define GC_errf(...)  GC_errf2(__FILE__, __LINE__, __VA_ARGS__)
#define GC_assert(cond) \
  do { \
    if (!(cond)) GC_errf("assertion failed: `%s'", #cond); \
  } while (0)

void
GC_dbgf2 (const char * file, int line, const char * format, ...);

void
GC_errf2 (const char * file, int line, const char * format, ...);

void
GC_debug_print_region_stats (struct GC_region region);

void
GC_debug_print_stack_stats (void);

void
GC_debug_capdump (const void * start, const void * end);

void
GC_debug_memdump (const void * start, const void * end);

#endif // GC_DEBUG_H_HEADER
