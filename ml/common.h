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
// --------------------End   GC_CHERI--------------------
#else // GC_CHERI
// --------------------Begin NOT GC_CHERI--------------------
#define  GC_INVALID_PTR     NULL
#define  GC_PTR_VALID(x)    ((x) != NULL)
#define  GC_STORE_CAP(x,y)  ((x) = (y))

// Blanks
#define  GC_debug_track_allocated(x,y)
#define  GC_CHECK_ADDRESS(x)
#define  GC_assert(x)
#define  GC_debug_print_region_stats(x)
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
#define ml_malloc(x)           cheri_getbase(GC_MALLOC((x)))
// --------------------End   GC_BOEHM-------------------
#elif defined(GC_NONE)
// --------------------Begin GC_NONE--------------------
#define  ml_malloc          malloc

// Blanks
#define  GC_CAP
#define  GC_init()
#define  GC_cheri_ptr(x,y)  (x)
// --------------------End   GC_NONE-------------------
#elif defined(GC_CHERI)
#else
#error "Define one of GC_CHERI, GC_BOEHM, GC_NONE."
#endif // GC_CHERI, GC_BOEHM, GC_NONE

#include <stdlib.h>

#endif // COMMON_H_HEADER
