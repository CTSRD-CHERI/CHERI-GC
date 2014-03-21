#ifndef GC_CONFIG_H_HEADER
#define GC_CONFIG_H_HEADER

#define GC_DEBUG
#define GC_THREAD_LOCAL_HEAP_SIZE             0x1000
#define GC_OLD_GENERATION_SEMISPACE_SIZE      0x30000
#define GC_COLLECT_ON_ALLOCATION_FAILURE      1

// This determines how we deal with old-to-young pointers. See gc_init.h for
// a list and explanation of techniques. You can change the technique at runtime
// by using GC_set_oy_technique().
#define GC_OY_DEFAULT                         GC_OY_MANUAL

#endif // GC_CONFIG_H_HEADER
