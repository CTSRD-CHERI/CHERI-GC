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

// TODO: also define this in gc.h
// used for old-to-young pointer handling when the technique is GC_OY_MANUAL
// (see gc_init.h)
// usage: use GC_STORE_CAP(x,y) where you would normally use x = y, where x
// and y are capabilities. x and y may be evaluated more than once, so ensure
// that they are side-effect free.
//
// How this works:
//
// We use two custom permission bits:
// YOUNG: the object **pointed to by** the capability is young.
// cOLD:  the **address of** the capability is contained in the old region.
//
// Why do we use "YOUNG" instead of "OLD"? Because NOT OLD doesn't imply YOUNG.
// A NOT OLD object could be allocated on the stack or elsewhere, but we need a
// sure-fire way of precisely identifying young objects. Basically, we can
// safely treat NOT YOUNG objects as OLD.
//
// Suppose A is the address of an object in a region (old or young).
// Suppose sz A is the size of this object.
// Suppose B is the address of an object in a region (old or young).
// Suppose sz B is the size of this object.
//
// Now a capability is a tuple of the form (tag?, base, length, OLD?, cOLD?).
// So suppose we have:
// z = (1, A, sz A, ?, ?)
// y = (1, B, sz B, ?, ?) or y = (0, ...)
//
// Then we wish to process:
// *(z+offset) = y;
// This is supplied to us as:
// GC_STORE_CAP(*(z+offset), y).
// That is, in the following, "x" is "*(z+offset)".
//
// So now we consider each case in turn:
//
// (1) y = (0, ...)
// If:
//      *(z+offset) = (0, ...), do nothing.
//      *(z+offset) = (1, ...), invalidate *(z+offset).
//
// (2) y = (1, B, sz B, NOT YOUNG, ?)
// If:
//      *(z+offset) = (0, ...) then:                (OLD->OLD or YOUNG->OLD)
//          set *(z+offset) = y, and
//          cOLD[*(z+offset)] = GC_IN(z+offset, old_region).
//          (That is, do a time-consuming check to see if z+offset is contained
//           in the old region.)
//      *(z+offset) = (1, ?, ?, ?, NOT cOLD) then:  (YOUNG->OLD)
//          set *(z+offset) = y, and
//          cOLD[*(z+offset)] = false.
//      *(z+offset) = (1, ?, ?, ?, cOLD) then:      (OLD->OLD)
//          set *(z+offset) = y, and
//          cOLD[*(z+offset)] = true.
//
// (3) y = (1, B, sz B, YOUNG, ?)
// If:
//      *(z+offset) = (0, ...) then:                (OLD->YOUNG or YOUNG->YOUNG)
//          set *(z+offset) = y, and
//          cOLD[*(z+offset)] = GC_IN(z+offset, old_region), and
//          if cOLD[*(z+offset)] is now set then process the old-young pointer.
//      *(z+offset) = (1, ?, ?, ?, NOT cOLD) then:  (YOUNG->YOUNG)
//          set *(z+offset) = y, and
//          cOLD[*(z+offset)] = false.
//      *(z+offset) = (1, ?, ?, ?, cOLD) then:      (OLD->YOUNG)
//          set *(z+offset) = y, and
//          cOLD[*(z+offset)] = true, and
//          process the old-young pointer.
//
// The only way the time-consuming GC_IN check can happen is if a store
// invalidates *(z+offset), or if it's never been initialized. If it has been
// initialized, the generational copy will ensure that the cOLD flag is set.
#define GC_STORE_CAP(x,y) \
  do { \
    if (GC_cheri_gettag((y))) \
    { \
      int tmp = GC_cheri_gettag((x)) ?  \
                GC_IS_CONTAINED_IN_OLD((x)) : \
                GC_IN(&(x), GC_state.old_generation.tospace); \
      (x) = tmp ? GC_SET_CONTAINED_IN_OLD((y)) \
                : GC_UNSET_CONTAINED_IN_OLD((y)); \
      if (tmp && GC_IS_YOUNG((y))) \
        GC_handle_oy_store((GC_cap_ptr *)&(x), (y)); \
    } \
    else \
    { \
      (x) = GC_INVALID_PTR; \
    } \
  } while (0)

GC_cap_ptr *
GC_handle_oy_store (GC_cap_ptr * x, GC_cap_ptr y);

GC_cap_ptr
GC_orperm (GC_cap_ptr cap, GC_ULL perm);

// actually uses permit_store_ephemeral_capability for now
#define GC_PERM_YOUNG (1 << 5)
// actually uses permit_seal for now
#define GC_PERM_CONTAINED_IN_OLD (1 << 6)

#define GC_IS_CONTAINED_IN_OLD(cap) \
  ( ! (((GC_ULL) GC_cheri_getperm((cap))) & GC_PERM_CONTAINED_IN_OLD)  )
  
#define GC_SET_CONTAINED_IN_OLD(cap) \
  ( GC_cheri_andperm((cap), ~GC_PERM_CONTAINED_IN_OLD) )

#define GC_UNSET_CONTAINED_IN_OLD(cap) \
  ( GC_orperm((cap), GC_PERM_CONTAINED_IN_OLD) )

#define GC_IS_YOUNG(cap) \
  ( ! (((GC_ULL) GC_cheri_getperm((cap))) & GC_PERM_YOUNG)  )
  
#define GC_SET_YOUNG(cap) \
  ( GC_cheri_andperm((cap), ~GC_PERM_YOUNG) )

#define GC_UNSET_YOUNG(cap) \
  ( GC_orperm((cap), GC_PERM_YOUNG) )


  
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
