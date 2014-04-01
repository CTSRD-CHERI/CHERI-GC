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
#define GC_assert(cond) \
  do { \
    if (!(cond)) GC_fatalf("assertion failed: `%s'", #cond); \
  } while (0)
#else // GC_DEBUG
#define GC_vdbgf(...)
#define GC_dbgf(...)
#define GC_assert(cond)
#endif // GC_DEBUG

#define GC_errf(...)  GC_errf2(__FILE__, __LINE__, __VA_ARGS__)
#define GC_fatalf(...)  GC_fatalf2(__FILE__, __LINE__, __VA_ARGS__)

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

#ifdef GC_GENERATIONAL
#define GC_CHECK_ADDRESS(addr) \
( \
  GC_IN((addr), GC_state.thread_local_region.tospace) || \
  GC_IN((addr), GC_state.old_generation.tospace) \
)
#else // GC_GENERATIONAL
#define GC_CHECK_ADDRESS(addr) \
  GC_IN((addr), GC_state.thread_local_region.tospace)
#endif // GC_GENERATIONAL


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
#define GC_fatalf2(file,line,...) \
  do { \
    printf("***GC FATAL %s:%d ", file, line); \
    printf(__VA_ARGS__); \
    printf("\n"); \
    exit(1); \
  } while (0)
void
GC_debug_print_region_stats (struct GC_region * region);

void
GC_debug_print_stack_stats (void);

void
GC_debug_capdump (const void * start, const void * end);

void
GC_debug_memdump (const void * start, const void * end);

void
GC_debug_check_tospace (void);

void
GC_debug_check_area (void * start, void * end);

void
GC_debug_check_roots (void);

#ifdef GC_DEBUG_TRACK_ALLOCATIONS
// To assist in tracking objects.

typedef struct
{
  void * base;
  size_t len;
  int valid;
  int tracking;
  char * tracking_name;
  int marked;
  int rebased; // so we don't do double rebasing
  char * file; int line; // user program location where the object was allocated
} GC_debug_value; // isn't an actual capability to avoid clashes with GC
typedef struct
{
  GC_debug_value * arr;
  size_t sz;
} GC_debug_arr;
struct
{
  GC_debug_arr * tbl;
  size_t sz;
} GC_debug_tbl;

// For the user. Prints stuff each time an object moves, etc.
// tracking_name is used to identify the capability on print-outs. Can be NULL.
// Returns 0 on success, non-zero otherwise.
int
GC_debug_track_allocated (GC_cap_ptr cap, const char * tracking_name);

GC_debug_value *
GC_debug_find_allocated (GC_cap_ptr cap);

GC_debug_value *
GC_debug_find_invalid (GC_cap_ptr cap);

void
GC_debug_just_allocated (GC_cap_ptr cap, const char * file, int line);

// Usual procedure is to do this upon collection:
// 1. Call GC_debug_begin_marking().
// 2. Copy-collect, calling GC_debug_just_copied() each time a capability is
//    moved.
// 3. Call GC_debug_end_marking().
// Internally the routines implement a mark-and-sweep of the hash table.
void
GC_debug_just_copied (GC_cap_ptr old_cap, GC_cap_ptr new_cap, void * parent);

// GC_grow calls this whenever a heap is moved.
void
GC_debug_rebase_allocation_entries (void * oldbase,
                                    size_t oldsize,
                                    void * newbase);

void
GC_debug_begin_marking (void);

void
GC_debug_end_marking (void * space_start, void * space_end);

void
GC_debug_print_allocated_stats (void);
#else // GC_DEBUG_TRACK_ALLOCATIONS

#define GC_debug_track_allocated(cap,tracking_name)   0
#define GC_debug_find_allocated(cap)
#define GC_debug_find_invalid(cap)
#define GC_debug_just_allocated(cap,file,line)
#define GC_debug_just_copied(old_cap,new_cap,parent)
#define GC_debug_rebase_allocation_entries(oldbase,oldsize,newbase)
#define GC_debug_begin_marking()
#define GC_debug_end_marking(space_start,space_end)
#define GC_debug_print_allocated_stats()

#endif // GC_DEBUG_TRACK_ALLOCATIONS

#endif // GC_DEBUG_H_HEADER
