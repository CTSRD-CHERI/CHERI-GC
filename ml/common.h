// You should include common.h in every file so that memwatch.h is included in
// every file, as per its README.
#ifdef MEMWATCH
#include <string.h>
#include <memwatch.h>
#endif // MEMWATCH

#ifndef COMMON_H_HEADER
#define COMMON_H_HEADER

#ifdef GC_CHERI
// --------------------Begin GC_CHERI--------------------
#include <gc.h>
#include <gc_init.h>
#include <gc_debug.h>

#define ml_malloc           GC_malloc
#define ml_collect          GC_collect
// --------------------End   GC_CHERI--------------------
#else // GC_CHERI
// --------------------Begin NOT GC_CHERI--------------------
#define  GC_STORE_CAP(x,y)  ((x) = (y))

// Blanks
#define  GC_debug_track_allocated(x,y)
#define  GC_CHECK_ADDRESS(x)
#define  GC_assert(x)
#define  GC_debug_print_region_stats(x)
#define  ml_collect()
#define  GC_USER_FUNC
// --------------------End NOT GC_CHERI--------------------
#endif // GC_CHERI

#if defined(GC_BOEHM)
// --------------------Begin GC_BOEHM--------------------
#include <gc.h>
#define  GC_CAP
#include <machine/cheri.h>
#include <machine/cheric.h>
#define GC_cheri_ptr(x,y)     ((x))
void
__LOCK_MALLOC (void);
void
__UNLOCK_MALLOC (void);
#define  ml_malloc(x)       GC_MALLOC(x)
#define  GC_INVALID_PTR()   NULL
#define  GC_PTR_VALID(x)    ((x) != NULL)
// --------------------End   GC_BOEHM-------------------
#elif defined(GC_NONE)
// --------------------Begin GC_NONE--------------------
/*#define  ml_malloc          malloc
#define  GC_cheri_ptr(x,y)  (x)

// Blanks
#define  GC_CAP
#define  GC_init()*/

#include <machine/cheri.h>
#include <machine/cheric.h>
#define  ml_malloc          ml_no_gc_malloc
#define  GC_cheri_ptr       cheri_ptr
#define  GC_CAP             __capability
#define  GC_init()
#define  GC_PTR_VALID(x)   ( ((void*)(cheri_getbase(x))) != NULL)
#define  GC_INVALID_PTR()  NULL
GC_USER_FUNC GC_CAP void *
ml_no_gc_malloc (size_t sz);
#define ML_ALIGN_32(p) \
  ( (void *) ( (((uintptr_t) (p)) + (uintptr_t) 31) & ~(uintptr_t) 0x1F ) )
// --------------------End   GC_NONE-------------------
#elif defined(GC_NOCAP)
// --------------------Begin GC_NOCAP--------------------
#define GC_CAP
#define GC_cheri_ptr(x,y) (x)
#define ml_malloc         malloc
#define GC_init()
#define GC_PTR_VALID(x)   (x)
#define GC_INVALID_PTR()  NULL
// --------------------End   GC_NOCAP-------------------
#elif defined(GC_CHERI)
#else
#error "Define one of GC_CHERI, GC_BOEHM, GC_NONE, GC_NOCAP."
#endif // GC_CHERI, GC_BOEHM, GC_NONE

#include <stdlib.h>

// These are not necessarily capability-aware, but are just here because of the
// type issue (casts to standard pointers lose capability tracking, often
// unpredictably).
GC_USER_FUNC GC_CAP void *
cmemset (GC_CAP void * ptr, int value, size_t num);
GC_USER_FUNC GC_CAP void *
cmemcpy (GC_CAP void * dest, GC_CAP const void * source, size_t num);
GC_USER_FUNC int
cstrcmp (GC_CAP const char * str1, GC_CAP const char * str2);
GC_USER_FUNC size_t
cstrlen (GC_CAP const char * str);

#endif // COMMON_H_HEADER
