#ifndef GC_DEBUG_H_HEADER
#define GC_DEBUG_H_HEADER

#include "gc_init.h"
#include "gc_config.h"

#ifdef GC_DEBUG
#define GC_dbgf(...)  GC_dbgf2(__FILE__, __LINE__, __VA_ARGS__)
#define GC_errf(...)  GC_errf2(__FILE__, __LINE__, __VA_ARGS__)
#define GC_assert(cond) \
  do { \
    if (!(cond)) GC_errf("assertion failed: `%s'", #cond); \
  } while (0)
#else
#define GC_dbgf(...)
#define GC_errf(...)
#define GC_assert(cond)
#endif

#define GC_MEM_PRETTY(x) \
( \
  (x) < 1000 ? (x) : \
  (x) < 1000000 ? ((x)+1000/2) / 1000 : \
  (x) < 1000000000 ? ((x)+1000000/2) / 1000000 : \
  ((x)+1000000000/2) / 1000000000 \
)
#define GC_MEM_PRETTY_UNIT(x) \
( \
  (x) < 1000 ? "B" : \
  (x) < 1000000 ? "kB" : \
  (x) < 1000000000 ? "MB" : \
  "GB" \
)
#define GC_NUM_PRETTY GC_MEM_PRETTY
#define GC_NUM_PRETTY_UNIT(x) \
( \
  (x) < 1000 ? "" : \
  (x) < 1000000 ? "k" : \
  (x) < 1000000000 ? "M" : \
  "G" \
)


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
