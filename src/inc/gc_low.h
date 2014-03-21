#ifndef GC_LOW_H_HEADER
#define GC_LOW_H_HEADER

#include <stdlib.h>

#define GC_ULL  unsigned long long

typedef __capability void * GC_cap_ptr;

#include <machine/cheri.h>
#include <machine/cheric.h>

#define     GC_cheri_getbase  cheri_getbase
#define     GC_cheri_getlen   cheri_getlen
#define     GC_cheri_gettag   cheri_gettag
#define     GC_cheri_getperm  cheri_getperm
#define     GC_cheri_andperm  cheri_andperm
#define     GC_cheri_setlen   cheri_setlen
#define     GC_cheri_ptr      cheri_ptr
#define     GC_CHERI_CGETTAG  CHERI_CGETTAG
#define     GC_CHERI_CGETBASE CHERI_CGETBASE
#define     GC_CHERI_CGETLEN  CHERI_CGETLEN
#define     GC_cheri_getreg   cheri_getreg
#define     GC_cheri_setreg   cheri_setreg

// also declared in gc.h
// the void* cast of GC_INVALID_PTR must be NULL
#define     GC_INVALID_PTR    cheri_zerocap()

// also declared in gc.h
// used for old-to-young pointer handling when the technique is GC_OY_MANUAL
// (see gc_init.h)
// usage: use GC_STORE_CAP(x,y) where you would normally use x = y, where x
// and y are capabilities. x and y may be evaluated more than once, so ensure
// that they are side-effect free.
//
// semantics:
//    typedef struct
//    { 
//      __capability int * ptr;
//    } OBJ;
//
//    __capability OBJ * a;
//
//    GC_STORE_CAP(a->ptr, something):
//
//    suppose a has been allocated in an old region
//    then (perm(a) & is_old) is true
//    but perm(a->ptr) & is_old could be anything
//
//    have another perm: contained_in_old
//    then perm(a) & contained_in_old could be anything
//    but perm(a->ptr) & contained_in_old is definitely TRUE (maintain this invariant!).
//
//    now:
//
//    if ( perm(a->ptr) & contained_in_old )
//      if (!(perm(something) & is_old))
//        {STORE A YOUNG POINTER IN AN OLD POINTER}
//        (make sure its contained_in_old perm remains set).
//      else
//        a->ptr = set_perm(something, contained_in_old)
//    else
//      a->ptr = unset_perm(something, contained_in_old)
//
//    How to maintain the `is_old' invariant:
//    - on promotion, set the is_old perm.
//
//    How to maintain the contained_in_old invariant:
//    - on store, set the contained_in_old perm as appropriate.
//    - on promotion, set the contained_in_old perm for all children.
#define GC_STORE_CAP(x,y) \
  ( \
    GC_IS_CONTAINED_IN_OLD((x)) ? \
    !GC_IS_OLD((y)) ? *GC_do_oy_store((GC_cap_ptr*)&(x), (y)) :  \
    ((x) = GC_SET_CONTAINED_IN_OLD((y))) : \
    ((x) = GC_UNSET_CONTAINED_IN_OLD((y))) \
  )

GC_cap_ptr *
GC_do_oy_store (GC_cap_ptr * x, GC_cap_ptr y);

GC_cap_ptr
GC_orperm (GC_cap_ptr cap, GC_ULL perm);

// actually uses permit_store_ephemeral_capability for now
#define GC_PERM_OLD (1 << 5)
// actually uses permit_seal for now
#define GC_PERM_CONTAINED_IN_OLD (1 << 6)

#define GC_IS_CONTAINED_IN_OLD(cap) \
  ( ! (((GC_ULL) GC_cheri_getperm((cap))) & GC_PERM_CONTAINED_IN_OLD)  )
  
#define GC_SET_CONTAINED_IN_OLD(cap) \
  ( GC_cheri_andperm((cap), ~GC_PERM_CONTAINED_IN_OLD) )

#define GC_UNSET_CONTAINED_IN_OLD(cap) \
  ( GC_orperm((cap), GC_PERM_CONTAINED_IN_OLD) )

#define GC_IS_OLD(cap) \
  ( ! (((GC_ULL) GC_cheri_getperm((cap))) & GC_PERM_OLD)  )
  
#define GC_SET_OLD(cap) \
  ( GC_cheri_andperm((cap), ~GC_PERM_OLD) )

#define GC_UNSET_OLD(cap) \
  ( GC_orperm((cap), GC_PERM_OLD) )


  
#define GC_PUSH_CAP_REG(cap_reg,dest_addr) \
  __asm __volatile \
  ( \
    "csc $c" #cap_reg ", %0, 0($c0)" : : "r"(dest_addr) : "memory" \
  )

#define GC_RESTORE_CAP_REG(cap_reg,source_addr) \
  __asm __volatile \
  ( \
    "clc $c" #cap_reg ", %0, 0($c0)" : : "r"(source_addr) \
  )

#define GC_RESTORE_CAP_REG_CAP_INDIRECT(cap_reg,source_cap,source_addr) \
  __asm __volatile \
  ( \
    "clc $c" #cap_reg ", %0, 0($c" #source_cap ")" : : "r"(source_addr) \
  )

// Goes to a higher memory address to align if need be
#define GC_ALIGN_32(ptr,typ) \
  ( (typ) ( (((uintptr_t) (ptr)) + (uintptr_t) 31) & ~(uintptr_t) 0x1F ) ) \

// Goes to a lower memory address to align
#define GC_ALIGN_32_LOW(ptr,typ) \
  ( (typ) ( ((uintptr_t) (ptr)) & ~(uintptr_t) 0x1F ) )
  
// The stack looks like this:
// ----high memory addresses----
// function arguments
// $ra
// $fp
// preserved registers
// local variables
// $gp
// ----low memory addresses----
// $sp and $fp point just beneath $gp during the call.
// i.e., the stack grows down.

#define GC_GET_STACK_PTR(ret) \
  __asm __volatile \
  ( \
    "daddiu %0, $sp, 0" : "=r"(ret) : : \
  )

#define GC_GET_FRAME_PTR(ret) \
  __asm __volatile \
  ( \
    "daddiu %0, $fp, 0" : "=r"(ret) : : \
  )

#define GC_IN(ptr,cap) \
  ( \
    ((uintptr_t) (ptr)) >= ((uintptr_t) GC_cheri_getbase((cap))) \
    && \
    ((uintptr_t) (ptr)) < ( ((uintptr_t) GC_cheri_getbase((cap))) \
                          + (uintptr_t) GC_cheri_getlen((cap)) ) \
  )
  
#define GC_FORWARDING_ADDRESS_PTR(cap) \
  ( GC_ALIGN_32(GC_cheri_getbase((cap)), void *) )

#define GC_FORWARDING_CAP(cap) \
  ( * (GC_cap_ptr *) GC_FORWARDING_ADDRESS_PTR((cap)) )

#define GC_IS_FORWARDING_ADDRESS(cap) \
  ( ! (((GC_ULL) GC_cheri_getperm((cap))) & GC_PERM_FORWARDING)  )
  
#define GC_MAKE_FORWARDING_ADDRESS(cap) \
  ( GC_cheri_andperm((cap), ~GC_PERM_FORWARDING) )

#define GC_STRIP_FORWARDING(cap) \
  ( GC_cheri_ptr(GC_cheri_getbase((cap)), GC_cheri_getlen((cap))) )

// TODO: use a *custom* perm and ensure it's *always* set for non-forwarding
// addresses (even when we pass caps around and make new ones...)
#define GC_PERM_FORWARDING    (1 << 7)

void *
GC_low_malloc (size_t sz);

void *
GC_low_calloc (size_t num, size_t sz);

void *
GC_low_realloc (void * ptr, size_t sz);

void *
GC_get_stack_bottom (void);

void *
GC_get_static_bottom (void);

void *
GC_get_static_top (void);

GC_cap_ptr
GC_cap_memcpy (GC_cap_ptr dest, GC_cap_ptr src);

GC_cap_ptr
GC_cap_memset (GC_cap_ptr dest, int value);

#endif // GC_LOW_H_HEADER
