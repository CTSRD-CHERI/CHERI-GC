#ifndef GC_H_HEADER
#define GC_H_HEADER

// The public header for the user.

#include <gc_config.h>

#define GC_CAP __capability

#define     GC_cheri_ptr      cheri_ptr
#define     GC_cheri_getlen(x)   cheri_getlen((GC_CAP void*)(x))

// The preferred way of checking the validity of a pointer
#define GC_PTR_VALID(x)   ( ((void*)(cheri_getbase(x))) != NULL)
// OLD:
// (Hacky conversion to GC_CAP void * to avoid const issues...)
//#define     GC_PTR_VALID(x)   ((int) GC_cheri_gettag((GC_CAP void *) (x)))
//#define GC_cheri_gettag cheri_gettag

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

// GC_malloc:
// returns GC_INVALID_PTR on failure, whose void* cast is guaranteed to be equal
// to NULL
#ifdef GC_DEBUG
#define GC_malloc(sz) GC_malloc2(__FILE__, __LINE__, (sz))
#else // GC_DEBUG
#define GC_malloc GC_malloc2
#endif // GC_DEBUG
__capability void *
GC_malloc2
(
#ifdef GC_DEBUG
  const char * file, int line,
#endif // GC_DEBUG
  size_t sz
);

// the void* cast of GC_INVALID_PTR is guaranteed to be NULL
//#define     GC_INVALID_PTR    cheri_zerocap()
#define GC_INVALID_PTR GC_cheri_ptr(NULL, 0)

#endif // GC_H_HEADER
