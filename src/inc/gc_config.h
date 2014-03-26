#ifndef GC_CONFIG_H_HEADER
#define GC_CONFIG_H_HEADER

#define GC_DEBUG
#define GC_VERBOSE_DEBUG
#define GC_THREAD_LOCAL_HEAP_SIZE             4    // 4K
#define GC_OLD_GENERATION_SEMISPACE_SIZE      40  // 200K

// If old heap residency exceeds this, collect, and if that fails, grow.
#define GC_OLD_GENERATION_HIGH_WATERMARK      0.5

// Set to 0 to disable. Must always be defined, however.
#define GC_COLLECT_ON_ALLOCATION_FAILURE      1

// ----HEAP GROWING----
// Current policies with GC_GENERATIONAL turned on:
// GC_GROW_YOUNG_HEAP
//  If set, the young heap is grown when allocation fails.
// GC_GROW_OLD_HEAP
//  If set, grows the old heap:
//   (a) just before promotion, if it appears that the young generation won't
//       fit inside the old heap, and
//   (b) just after promotion, and if the old heap's used area divided by its
//       total area is higher than GC_OLD_GENERATION_HIGH_WATERMARK, but after
//       performing a major collection, and regardless of whether the major
//       collection freed any space up (to avoid it happening again).

// Grow the (young) heap if allocation/collection fails.
#define GC_GROW_YOUNG_HEAP
// Grow the old heap if copying collection would otherwise fail.
#define GC_GROW_OLD_HEAP
// NOTE: current policy for both is to double the current heap size, saturating
// at relevant max.
// NOTE: when GC_GENERATIONAL is undefined, only GC_GROW_YOUNG_HEAP is relevant.

// Do not edit
#if defined(GC_GROW_YOUNG_HEAP) \
    || defined(GC_GROW_OLD_HEAP)
#define GC_GROW_HEAP
#endif

// Maximum sizes for when the heap does grow. Set to 0 to allow unlimited
// growth.
#define GC_THREAD_LOCAL_HEAP_MAX_SIZE         500000   // 500kB
#define GC_OLD_GENERATION_SEMISPACE_MAX_SIZE  5500000 // 5.5M

// Determines whether we use generational GC or not. If disabled, only
// copying collection is implemented.
#define GC_GENERATIONAL

#ifdef GC_GENERATIONAL
// This determines how we deal with old-to-young pointers. See gc_init.h for
// a list and explanation of techniques. You can change the technique at runtime
// by using GC_set_oy_technique().
#define GC_OY_DEFAULT                         GC_OY_MANUAL

// If this is defined, you can change the OY technique at runtime. Otherwise,
// the technique specified by GC_OY_DEFAULT is compiled in.
#define GC_OY_RUNTIME
#endif // GC_GENERATIONAL

#define GC_COLLECT_STATS
//#define GC_DEBUG_TRACK_ALLOCATIONS

// Enable if you want the GC to time how long things take
#define GC_TIME

#endif // GC_CONFIG_H_HEADER
