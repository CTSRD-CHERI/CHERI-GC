#ifdef MEMWATCH
#include <string.h>
#include <memwatch.h>
#endif // MEMWATCH

#ifndef TEST_ALL_H_HEADER
#define TEST_ALL_H_HEADER

#define tf_ull_t unsigned long long

#ifdef GC_CHERI
// --------------------Begin GC_CHERI--------------------
#include <gc.h>
#include <gc_init.h>
#include <gc_debug.h>

#include <gc_malloc.h>
#include <gc_collect.h>
#include <gc_low.h>
#include <gc_config.h>
#include <gc_bitmap.h>

#define tf_cap_t            GC_CAP
#define tf_malloc           GC_malloc
#define tf_free(x)          do{}while(0)
#define tf_collect          GC_collect
#define tf_store_cap        GC_STORE_CAP
#define tf_func_t           GC_USER_FUNC
#define tf_cheri_ptr        GC_cheri_ptr
#define tf_cheri_getbase    GC_cheri_getbase
#define tf_cheri_getlen     GC_cheri_getlen
#define tf_cheri_gettag     GC_cheri_gettag
#define tf_invalid_ptr      GC_INVALID_PTR
#define tf_ptr_valid        GC_PTR_VALID
#define tf_gc_init          GC_init
// --------------------End   GC_CHERI--------------------

#elif defined(GC_BOEHM)
// --------------------Begin GC_BOEHM--------------------
#include <gc.h>
#include <machine/cheri.h>
#include <machine/cheric.h>
#define  tf_cap_t
#define  tf_malloc(x)       cheri_getbase(GC_MALLOC((x)))
#define  tf_free(x)         do{}while(0)
#define  tf_collect         GC_COLLECT
#define  tf_store_cap(x,y)  ((x) = (y))
#define  tf_func_t
#define  tf_cheri_ptr(x,y)  ((x))
#define  tf_cheri_getbase(x) ((x))
#undef tf_cheri_getlen
#undef tf_cheri_gettag
#define  tf_invalid_ptr     NULL
#define  tf_ptr_valid(x)    ((x) != NULL)
#define  tf_gc_init()       (GC_init(),0)
void
__LOCK_MALLOC (void);
void
__UNLOCK_MALLOC (void);
// --------------------End   GC_BOEHM-------------------

#elif defined(GC_NONE)
// --------------------Begin GC_NONE--------------------
#include <machine/cheri.h>
#include <machine/cheric.h>

#define tf_cap_t            __capability
#define tf_malloc           tf_no_gc_malloc
#define tf_free             tf_no_gc_free
#define tf_collect()        do{}while(0)
#define tf_store_cap(x,y)   ((x) = (y))
#define tf_func_t
#define tf_cheri_ptr        cheri_ptr
#define tf_cheri_getbase    cheri_getbase
#define tf_cheri_getlen     cheri_getlen
#define tf_cheri_gettag     cheri_gettag
#define tf_invalid_ptr      cheri_ptr(NULL, 0)
#define tf_ptr_valid(x)     ( ((void*)(cheri_getbase(x))) != NULL)
#define tf_gc_init()        0

tf_func_t tf_cap_t void *
tf_no_gc_malloc (size_t sz);
tf_func_t void
tf_no_gc_free (tf_cap_t void * ptr);
// --------------------End   GC_NONE-------------------

#else

#error "Define one of GC_CHERI, GC_BOEHM, GC_NONE."

#endif // GC_CHERI, GC_BOEHM, GC_NONE

#include <stdlib.h>

#define TF_ALIGN_32(p) \
  ( (void *) ( (((uintptr_t) (p)) + (uintptr_t) 31) & ~(uintptr_t) 0x1F ) )

#include <stdio.h>
#define tf_printf printf

tf_func_t void
tf_dump_stats (void);

#define tf_mem_pretty(x) \
((tf_ull_t)( \
  (x) < 1000 ? (x) : \
  (x) < 1000000 ? ((x)+1000/2) / 1000 : \
  (x) < 1000000000 ? ((x)+1000000/2) / 1000000 : \
  ((x)+1000000000/2) / 1000000000 \
))
#define tf_mem_pretty_unit(x) \
( \
  (x) < 1000 ? "B" : \
  (x) < 1000000 ? "kB" : \
  (x) < 1000000000 ? "MB" : \
  "GB" \
)
#define tf_num_pretty tf_mem_pretty
#define tf_num_pretty_unit(x) \
( \
  (x) < 1000 ? "" : \
  (x) < 1000000 ? "k" : \
  (x) < 1000000000 ? "M" : \
  "G" \
)

#endif // TEST_ALL_H_HEADER
