#ifndef GC_DEBUG_H_HEADER
#define GC_DEBUG_H_HEADER

#include "gc_init.h"
#include "gc_config.h"
#include "gc_low.h"

#ifdef GC_DEBUG
#ifdef GC_VERBOSE_DEBUG
#define GC_vdbgf GC_dbgf
#else // GC_VERBOSE_DEBUG
#define GC_vdbgf(...)
#endif // GC_VERBOSE_DEBUG
#define GC_dbgf(...)  GC_dbgf2(__FILE__, __LINE__, __VA_ARGS__)
#define GC_errf(...)  GC_errf2(__FILE__, __LINE__, __VA_ARGS__)
#define GC_assert(cond) \
  do { \
    if (!(cond)) GC_errf("assertion failed: `%s'", #cond); \
  } while (0)
#else // GC_DEBUG
#define GC_vdbgf(...)
#define GC_dbgf(...)
#define GC_errf(...)
#define GC_assert(cond)
#endif // GC_DEBUG

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

#define GC_PRINT_CAP(cap) GC_debug_print_cap(#cap, (cap))

void GC_debug_print_cap (const char * name, GC_cap_ptr cap);

// disabling these for now because it seems the compiler is buggy when it comes
// to varargs. But printf works fine (so far...).
/*
void
GC_dbgf2 (const char * file, int line, const char * format, ...);

void
GC_errf2 (const char * file, int line, const char * format, ...);
*/
// The workaround:
#include <stdio.h>
#define GC_dbgf2(file,line,...) \
  do { \
    printf("[GC debug] %s:%d ", file, line); \
    printf(__VA_ARGS__); \
    printf("\n"); \
  } while (0)
#define GC_errf2(file,line,...) \
  do { \
    printf("[GC error] %s:%d ", file, line); \
    printf(__VA_ARGS__); \
    printf("\n"); \
  } while (0)

void
GC_debug_print_region_stats (struct GC_region * region);

void
GC_debug_print_stack_stats (void);

void
GC_debug_capdump (const void * start, const void * end);

void
GC_debug_memdump (const void * start, const void * end);


// To assist in checking whether owned objects are valid things on the heap
// (i.e. leak/corruption detection):
#ifdef GC_DEBUG_TRACK_ALLOCATIONS
int
GC_debug_is_allocated (GC_cap_ptr cap);

void
GC_debug_just_allocated (GC_cap_ptr cap);

void
GC_debug_just_deallocated (GC_cap_ptr cap);

void
GC_debug_print_allocated_stats (void);
#else // GC_DEBUG_TRACK_ALLOCATIONS

#define GC_debug_just_allocated(cap)
#define GC_debug_just_deallocated(cap)

#endif // GC_DEBUG_TRACK_ALLOCATIONS

#endif // GC_DEBUG_H_HEADER
