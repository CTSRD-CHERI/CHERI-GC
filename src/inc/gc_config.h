#ifndef GC_CONFIG_H_HEADER
#define GC_CONFIG_H_HEADER

#define GC_DEBUG
//#define GC_VERBOSE_DEBUG
//#define GC_DEBUG_TRACK_ALLOCATIONS
#define GC_THREAD_LOCAL_HEAP_SIZE             4096    // 4K
#define GC_OLD_GENERATION_SEMISPACE_SIZE      200000  // 200K

// Set to 0 to disable. Must always be defined, however.
#define GC_COLLECT_ON_ALLOCATION_FAILURE      1

// Grow the heap if collection/allocation fails.
// NOTE: current policy is to double the current heap size, saturating at
// relevant max.
#define GC_GROW_HEAP_ON_COLLECTION_FAILURE
#define GC_GROW_HEAP_ON_ALLOCATION_FAILURE

// Do not edit
#if defined(GC_GROW_HEAP_ON_COLLECTION_FAILURE) \
    || defined(GC_GROW_HEAP_ON_ALLOCATION_FAILURE)
#define GC_GROW_HEAP
#endif

// Maximum sizes for when the heap does grow. Set to 0 to allow unlimited
// growth.
#define GC_THREAD_LOCAL_HEAP_MAX_SIZE         500000   // 500K
#define GC_OLD_GENERATION_SEMISPACE_MAX_SIZE  18000000 // 18M

// Determines whether we use generational GC or not. If disabled, only
// copying collection is implemented.
//#define GC_GENERATIONAL

#ifdef GC_GENERATIONAL
// This determines how we deal with old-to-young pointers. See gc_init.h for
// a list and explanation of techniques. You can change the technique at runtime
// by using GC_set_oy_technique().
#define GC_OY_DEFAULT                         GC_OY_MANUAL

// If this is defined, you can change the OY technique at runtime. Otherwise,
// the technique specified by GC_OY_DEFAULT is compiled in.
#define GC_OY_RUNTIME
#endif // GC_GENERATIONAL

#endif // GC_CONFIG_H_HEADER
