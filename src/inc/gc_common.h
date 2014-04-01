#ifndef GC_COMMON_H_HEADER
#define GC_COMMON_H_HEADER

#include "gc_config.h"

#ifdef GC_USE_GC_STACK_CLEAN
#define GC_FUNC
#define GC_USER_FUNC
#else // GC_USE_GC_STACK_CLEAN
#define GC_FUNC         __attribute__((sensitive))
#define GC_USER_FUNC    __attribute__((sensitive))
#endif // GC_USE_GC_STACK_CLEAN

#endif // GC_COMMON_H_HEADER
