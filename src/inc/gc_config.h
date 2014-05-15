#ifndef GC_CONFIG_H_HEADER
#define GC_CONFIG_H_HEADER

//#define GC_DEBUG
//#define GC_VERBOSE_DEBUG

// Can be used when testing against the Boehm collector, defined here for
// convenience. test_all.c, for example, uses this.
#define GC_BOEHM_MAX_HEAP_SIZE                0//1900000

#define GC_THREAD_LOCAL_HEAP_SIZE                              (65536)
#define GC_OLD_GENERATION_SEMISPACE_SIZE                       (10)

// Maximum sizes for when the heap does grow. Set to 0 to allow unlimited
// growth.
#define GC_THREAD_LOCAL_HEAP_MAX_SIZE_BEFORE_COLLECTION        (65536)
#define GC_THREAD_LOCAL_HEAP_MAX_SIZE                          (0)
#define GC_OLD_GENERATION_SEMISPACE_MAX_SIZE_BEFORE_COLLECTION (10)
#define GC_OLD_GENERATION_SEMISPACE_MAX_SIZE                   (10)

// Determines whether we use generational GC or not. If disabled, only
// copying collection is implemented.
#define GC_GENERATIONAL

// Enables support for trapping ephemeral stores. Required for GC_WB_EPHEMERAL.
//#define GC_EPHEMERAL_TRAP_SUPPORT

// If old heap residency exceeds this, collect, and if that fails, grow.
#define GC_OLD_GENERATION_HIGH_WATERMARK      0.5

// Set to 0 to disable. Must always be defined, however.
#define GC_COLLECT_ON_ALLOCATION_FAILURE      1

// Enable this to use a bitmap to track allocated objects.
#define GC_USE_BITMAP

// Kind of temporary
#define GC_MAX_STACK_TOP    (void*) 0x7fff000000

// If this is defined, we manually clean the stack. Otherwise, we rely on
// __attribute__((sensitive)) annotations to do it for us.
//#define GC_USE_GC_STACK_CLEAN
#ifndef GC_USE_GC_STACK_CLEAN
//#define GC_USE_ATTR_SENSITIVE
#endif // GC_USE_GC_STACK_CLEAN

// Magic values for debugging

// Used by lower-level routines:
#define GC_MAGIC_JUST_ALLOCATED               0x41      // 'A'
#define GC_MAGIC_JUST_REALLOCATED             0x42      // 'B'

// Used by GC_cap_memclr
#define GC_MAGIC_JUST_CLEARED                 0x43      // 'C'

// Used only if GC_DEBUG_TRACK_ALLOCATIONS is defined, when a deallocation is
// detected
#define GC_MAGIC_DEALLOCATION_DETECTED        0x44      // 'D'

// Used to clobber a capability that's just been copied during a collection
#define GC_MAGIC_JUST_COPIED                  0x45      // 'E'

// Used when cleaning the stack
#define GC_MAGIC_JUST_CLEARED_STACK           0x46      // 'F'

// Used by GC_malloc on freshly allocated objects
#define GC_MAGIC_JUST_GC_ALLOCATED            0x47      // 'G'

// Used by GC_collect to clear the fromspace
#define GC_MAGIC_JUST_CLEARED_FROMSPACE       0x48      // 'H'

// Used by GC_reset_region to clear the tospace
#define GC_MAGIC_JUST_CLEARED_TOSPACE         0x49      // 'I'

// Used by GC_low_realloc
#define GC_MAGIC_JUST_FREED                   0x4A      // 'J'

// Used by GC_gen_promote to clear the young tospace
#define GC_MAGIC_JUST_CLEARED_YOUNG_TOSPACE   0x4B      // 'K'

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

#ifdef GC_GENERATIONAL
// This determines the write barrier technique we use to deal with old-to-young
// pointers. See gc_init.h for a list and explanation of techniques. You can
// change the technique at runtime by using GC_set_wb_type().
#define GC_WB_DEFAULT                         GC_WB_MANUAL

// This determines the technique used to deal with old-young pointers. See
// gc_init.h for a list.
#define GC_OY_STORE_DEFAULT                   GC_OY_STORE_REMEMBERED_SET

// If this is defined, you can change the write barrier technique at runtime.
// Otherwise, the technique specified by GC_WB_DEFAULT is compiled in.
//#define GC_WB_RUNTIME
#endif // GC_GENERATIONAL

#define GC_COLLECT_STATS
//#define GC_DEBUG_TRACK_ALLOCATIONS

// Enable if you want the GC to time how long things take
//#define GC_TIME

#endif // GC_CONFIG_H_HEADER
