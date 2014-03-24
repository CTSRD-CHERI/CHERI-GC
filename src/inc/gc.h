#ifndef GC_H_HEADER
#define GC_H_HEADER

// The public header for the user.

#include <machine/cheri.h>
#include <machine/cheric.h>

// Return values:
// 0 : success
// 1 : error
#define GC_init()   GC_init2(__FILE__, __LINE__)
int
GC_init2 (const char * file, int line);

#ifdef GC_GENERATIONAL
// Return values:
// 0 : success
// 1 : error
int
GC_set_oy_technique (int oy_technique);
#else // GC_GENERATIONAL
#define GC_set_oy_technique(oy_technique) 0
#endif // GC_GENERATIONAL

// returns GC_INVALID_PTR on failure, whose void* cast is guaranteed to be equal
// to NULL
__capability void *
GC_malloc (size_t sz);

// the void* cast of GC_INVALID_PTR is guaranteed to be NULL
#define     GC_INVALID_PTR    cheri_zerocap()

#endif // GC_H_HEADER
