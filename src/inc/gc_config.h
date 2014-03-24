#ifndef GC_CONFIG_H_HEADER
#define GC_CONFIG_H_HEADER

#define GC_DEBUG
//#define GC_VERBOSE_DEBUG
#define GC_THREAD_LOCAL_HEAP_SIZE             0x1000
#define GC_OLD_GENERATION_SEMISPACE_SIZE      0x30000
#define GC_COLLECT_ON_ALLOCATION_FAILURE      1

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

#endif // GC_CONFIG_H_HEADER
