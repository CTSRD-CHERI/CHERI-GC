#ifndef GC_H_HEADER
#define GC_H_HEADER

// The public header for the user.

#include <gc_common.h>
#include <gc_config.h>

// includes for GC_STORE_CAP. TODO: make this cleaner
#include <gc_low.h>
#include <gc_init.h>
#include <gc_debug.h>

#define GC_CAP __capability
/*
// this doesn't have __attribute__((sensitive))
//#define     GC_cheri_ptr      cheri_ptr
#define GC_cheri_ptr(ptr,len) \
  ( cheri_setlen( (__capability void *)(void*)(ptr), (len) ) )

// (Hacky conversion to GC_CAP void * to avoid const issues...)
#define     GC_cheri_getlen(x)   cheri_getlen((GC_CAP void*)(x))

// The preferred way of checking the validity of a pointer
#define GC_PTR_VALID(x)   ( ((void*)(cheri_getbase(x))) != NULL)
// OLD:
// (Hacky conversion to GC_CAP void * to avoid const issues...)
//#define     GC_PTR_VALID(x)   ((int) GC_cheri_gettag((GC_CAP void *) (x)))
//#define GC_cheri_gettag cheri_gettag
*/

// The preferred way of checking the validity of a pointer
#define GC_PTR_VALID(x)   ( ((void*)(x)) != NULL )

#include <machine/cheri.h>
#include <machine/cheric.h>

// Return values:
// GC_init(): MUST be called from main(), and main must take argc as an
// argument.
// Return values:
// 0 : success
// 1 : error
#define GC_init()   GC_init2(&argc, __FILE__, __LINE__)
GC_FUNC int
GC_init2 (void * arg_for_stack_bottom, const char * file, int line);

#ifdef GC_GENERATIONAL
// Return values:
// 0 : success
// 1 : error
GC_FUNC int
GC_set_wb_type (int wb_type);
#else // GC_GENERATIONAL
#define GC_set_wb_type(wb_type) 0
#endif // GC_GENERATIONAL

// GC_malloc:
// returns GC_INVALID_PTR on failure, whose void* cast is guaranteed to be equal
// to NULL
#ifdef GC_DEBUG
#define GC_malloc(sz) GC_malloc2(__FILE__, __LINE__, (sz))
#else // GC_DEBUG
#define GC_malloc GC_malloc2
#endif // GC_DEBUG
GC_FUNC __capability void *
GC_malloc2
(
#ifdef GC_DEBUG
  const char * file, int line,
#endif // GC_DEBUG
  size_t sz
);

GC_FUNC void
GC_collect (void);

// In non-generational mode, this has the same effect as GC_collect()
GC_FUNC void
GC_major_collect (void);

// Free the entire young generation. Does not collect.
GC_FUNC void
GC_minor_free (void);

// the void* cast of GC_INVALID_PTR is guaranteed to be NULL
//#define     GC_INVALID_PTR    cheri_zerocap()
//#define GC_INVALID_PTR GC_cheri_ptr(NULL, 0)
//#define GC_INVALID_PTR ((__capability void *)NULL)

#endif // GC_H_HEADER
