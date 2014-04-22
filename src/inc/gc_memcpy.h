#ifndef GC_MEMCPY_H_HEADER
#define GC_MEMCPY_H_HEADER

// memcpy replacement for user programs which saves capability tag bits
// Should not be used by the GC as it relies on GC_STORE_CAP.

#include "gc_common.h"
#include "gc_config.h"

#include <stdlib.h>

void *
GC_memcpy (void * dest, const void * src, size_t sz);

#endif // GC_MEMCPY_H_HEADER
