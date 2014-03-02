#ifndef GC_LOW_H_HEADER
#define GC_LOW_H_HEADER

#include <stdlib.h>

void *
GC_low_malloc (size_t sz);

void *
GC_low_calloc (size_t num, size_t sz);

void *
GC_low_realloc (void * ptr, size_t sz);

void *
GC_get_stack_bottom (void);

#endif // GC_LOW_H_HEADER
