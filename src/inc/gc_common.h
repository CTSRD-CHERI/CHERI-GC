#ifndef GC_COMMON_H_HEADER
#define GC_COMMON_H_HEADER

#include "gc_config.h"

#ifdef GC_USE_ATTR_SENSITIVE
#define GC_FUNC
#define GC_USER_FUNC
#else // GC_USE_ATTR_SENSITIVE
#define GC_FUNC         __attribute__((sensitive))
#define GC_USER_FUNC    __attribute__((sensitive))
#endif // GC_USE_ATTR_SENSITIVE

#endif // GC_COMMON_H_HEADER
