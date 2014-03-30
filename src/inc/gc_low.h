#ifndef GC_LOW_H_HEADER
#define GC_LOW_H_HEADER

#include "gc_config.h"

#include <stdlib.h>

#define GC_ULL  unsigned long long

typedef __capability void * GC_cap_ptr;

#include <machine/cheri.h>
#include <machine/cheric.h>

#define     GC_cheri_getbase  cheri_getbase
//#define     GC_cheri_getlen   cheri_getlen
#define GC_CAP __capability
#define     GC_cheri_getlen(x)   cheri_getlen((GC_CAP void*)(x))

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

// Sets the base and length of a capability while preserving its permissions
// TODO: preserve other things
#define     GC_setbaselen(cap,new_base,new_len) \
  GC_cheri_andperm(GC_cheri_ptr((new_base),(new_len)), GC_cheri_getperm((cap)))

// Sets the base of a capability while preserving its length and permissions
// TODO: preserve other things
#define     GC_setbase(cap,new_base) \
  GC_cheri_andperm(GC_cheri_ptr((new_base), GC_cheri_getlen((cap))), \
                   GC_cheri_getperm((cap)))

// also declared in gc.h
// the void* cast of GC_INVALID_PTR must be NULL
//#define     GC_INVALID_PTR    cheri_zerocap()
#define GC_INVALID_PTR GC_cheri_ptr(NULL, 0)

// also declared in gc.h
//#define     GC_PTR_VALID(x)   (GC_cheri_gettag((x)))

#define GC_NOOP do{}while(0)

#define GC_FORWARDING_ADDRESS_PTR(cap) \
  ( GC_ALIGN_32(GC_cheri_getbase((cap)), void *) )

#define GC_FORWARDING_CAP(cap) \
  ( * (GC_cap_ptr *) GC_FORWARDING_ADDRESS_PTR((cap)) )

#define GC_IS_FORWARDING_ADDRESS(cap) \
  ( ! (((GC_ULL) GC_cheri_getperm((cap))) & GC_PERM_FORWARDING)  )
  
#define GC_MAKE_FORWARDING_ADDRESS(cap) \
  ( GC_cheri_andperm((cap), ~GC_PERM_FORWARDING) )

#define GC_STRIP_FORWARDING(cap) \
  ( GC_orperm((cap), GC_PERM_FORWARDING) )
  
// All custom permissions:
// NOTE: GC_PERM_YOUNG, GC_PERM_CONTAINED_IN_OLD only used when WB technique is
//       GC_WB_MANUAL.
// actually uses permit_store_ephemeral_capability for now.
#define GC_PERM_YOUNG (1 << 6)
// actually uses permit_seal for now
#define GC_PERM_CONTAINED_IN_OLD (1 << 7)
// TODO: use a *custom* perm and ensure it's *always* set for non-forwarding
// addresses (even when we pass caps around and make new ones...)
// At the moment we're using the Set_Type permission
#define GC_PERM_FORWARDING    (1 << 8)
// actually uses permit_execute for now
#define GC_PERM_GC_ALLOCATED  (1 << 1)

// Ensures we copy only objects that we've allocated.
// Not strictly needed if we define pointers into our allocated memory as always
// being managed by us.
#define GC_IS_GC_ALLOCATED(cap) \
  ( ! (((GC_ULL) GC_cheri_getperm((cap))) & GC_PERM_GC_ALLOCATED)  )
  
#define GC_SET_GC_ALLOCATED(cap) \
  ( GC_cheri_andperm((cap), ~GC_PERM_GC_ALLOCATED) )

#define GC_UNSET_GC_ALLOCATED(cap) \
  ( GC_orperm((cap), GC_PERM_GC_ALLOCATED) )

// TODO: also define this in gc.h
// used for old-to-young pointer handling when the technique is GC_WB_MANUAL
// (see gc_init.h)
// usage: use GC_STORE_CAP(x,y) where you would normally use x = y, where x
// and y are capabilities, and y has been allocated by the GC. x and y are
// guaranteed to be evaluated only once.
//
// You should initialize x (even with just GC_INVALID_PTR) before the first time
// you call GC_STORE_CAP. However, it may hinder performance if you invalidate
// x every time you use GC_STORE_CAP, because the GC uses spare bits inside x to
// store age information to avoid expensive lookups.
//
// Note that even if x is a local or global variable, you still need to use
// GC_STORE_CAP. This is because the relevant permission bits need to be
// modified on the stored capability to protect against future stores.
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
// The cOLD bit essentially acts as a cache indicating whether z+offset is in
// the old region.
#ifdef GC_GENERATIONAL
// WARNING: assumes GC_init() has already been called.
// TODO: ensure tmpx and tmpy are cleared when this is done.

// NOTE: we evaluate y first because it could potentially cause &x to move
// elsewhere, and tmpx will not generally be considered a root (whereas if y
// moves somewhere afterwards then tmpy should be updated because it is a root).
#define GC_STORE_CAP(x,y) \
  do { \
    printf("[GC_STORE_CAP] Begin processing %s = %s &x=0x%llx\n", #x, #y, (unsigned long long) &x); \
    __capability void * tmpy = (y); \
    printf("evaluated %s to get 0x%llx\n", #y, (GC_ULL) tmpy); \
    __capability void * __capability * tmpx = (__capability void * __capability *) &(x); \
    GC_PRINT_CAP(tmpx); \
    if (GC_IN(tmpx, GC_state.old_generation.tospace)) \
    { \
      printf("[GC_STORE_CAP] & ( %s ) is old.\n", #x); \
      if (GC_IN(tmpy, GC_state.thread_local_region.tospace)) \
      { \
        GC_dbgf("%s is young.\n", #y); \
        GC_handle_oy_store(tmpx, tmpy); \
      } \
    } \
    else if (GC_IN(tmpx, GC_state.thread_local_region.tospace)) \
      printf("[GC_STORE_CAP] & ( %s ) is young.\n", #x); \
    else \
      printf("[GC_STORE_CAP] & ( %s ) is neither young nor old!\n", #x); \
    printf("Trying to do *(0x%llx) = 0x%llx\n", (unsigned long long) tmpx, (unsigned long long) tmpy); \
    __asm__ ("daddiu $1, $1, 0"); \
    *tmpx = tmpy; \
    __asm__ ("daddiu $2, $2, 0"); \
    printf("[GC_STORE_CAP] &x=0x%llx, tmpx=0x%llx\n", (unsigned long long) &(x), (unsigned long long) tmpx); \
    GC_assert((uintptr_t)&(x) ==(uintptr_t) tmpx); \
    tmpx = (__capability void * __capability *) GC_cheri_ptr(NULL, 0); \
    printf("[GC_STORE_CAP] __DONE__: %s=%s\n", #x, #y); \
  } while (0)
  
  
#define GC_STORE_CAP_OLD(x,y) \
  do { \
    GC_cap_ptr * tmpx = (GC_cap_ptr *) &(x); \
    GC_cap_ptr tmpy = (GC_cap_ptr) (y); \
    if (GC_cheri_gettag(tmpy)) \
    { \
      int tmp = GC_cheri_gettag(*tmpx) ?  \
                GC_IS_CONTAINED_IN_OLD(*tmpx) : \
                GC_IN(tmpx, GC_state.old_generation.tospace); \
      *tmpx = tmp ? GC_SET_CONTAINED_IN_OLD(tmpy) \
                : GC_UNSET_CONTAINED_IN_OLD(tmpy); \
      if (tmp && GC_IS_YOUNG(tmpy)) \
        GC_handle_oy_store(tmpx, tmpy); \
    } \
    else \
    { \
      printf("no tag %s=%s\n",#x,#y); \
      *tmpx = GC_INVALID_PTR; \
    } \
    tmpx = NULL; \
    tmpy = GC_INVALID_PTR; \
  } while (0)
#else // GC_GENERATIONAL
#define GC_STORE_CAP(x,y) ( (x) = (y) )
#endif // GC_GENERATIONAL

#ifdef GC_GENERATIONAL

__capability void * __capability *
GC_handle_oy_store (__capability void * __capability * x, GC_cap_ptr y);

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

#define GC_PERM_NON_EPHEMERAL (1 << 0)

#define GC_IS_EPHEMERAL(cap) \
  ( ! (((GC_ULL) GC_cheri_getperm((cap))) & GC_PERM_NON_EPHEMERAL)  )
  
#define GC_SET_EPHEMERAL(cap) \
  ( GC_cheri_andperm((cap), ~GC_PERM_NON_EPHEMERAL) )

#define GC_UNSET_EPHEMERAL(cap) \
  ( GC_orperm((cap), GC_PERM_NON_EPHEMERAL) )

#endif // GC_GENERATIONAL

GC_cap_ptr
GC_orperm (GC_cap_ptr cap, GC_ULL perm);

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
  
#define GC_IS_ALIGNED_32(ptr) \
  ( (((uintptr_t)(ptr)) & 0x1F) == 0 )
  
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

#define GC_IN_OR_ON_BOUNDARY(ptr,cap) \
  ( \
    ((uintptr_t) (ptr)) >= ((uintptr_t) GC_cheri_getbase((cap))) \
    && \
    ((uintptr_t) (ptr)) <=( ((uintptr_t) GC_cheri_getbase((cap))) \
                          + (uintptr_t) GC_cheri_getlen((cap)) ) \
  )

  
// GC_PUSH_CAP_REGS(buf):
//  Allocates an buffer named buf on the stack, containing the capability
//  registers. The buffer is an array of length GC_NUM_CAP_REGS and each element
//  in the array is a capability.
// GC_RESTORE_CAP_REGS(buf):
//  Restores the capability state from a buffer allocated with GC_PUSH_CAP_REGS.

// This information is from version 1.8 of the CHERI spec:
// We ignore:
//  $c0 because it spans the entire address space
//  $c1 - $c2 because they are caller-save
//  $c3 - $c10 because they are used to pass arguments
//  $c11 - $c16 because they are also caller-save
//  $c27 - $c31 because they are for kernel use 
// We therefore don't ignore:
//  $c17 - $c24 because they are callee-save
//  $c25
//  $c26 (IDC)
//
// This information is from version 1.5 of the CHERI spec:
// We ignore:
//  $c0 because it spans the entire address space
//  $c1 - $c4 because they are used to pass arguments and can be treated as
//            clobbered
//  $c5 - $c15, $c15 - $c23 because they are caller-save
//  $c27 - $c31 because they are for kernel use 
// We therefore don't ignore:
//  $c16 - $c23 because they are callee-save
//  $c24 - $c26 because whether they can be ignored is unknown
//
// So to be conservative we save $c16 - $c26.
#define GC_NUM_CAP_REGS   11
#define GC_PUSH_CAP_REGS(buf) \
  GC_cap_ptr buf##_misaligned[sizeof(GC_cap_ptr)*GC_NUM_CAP_REGS+32]; \
  GC_cap_ptr * buf = buf##_misaligned; \
  /* CSC instruction needs 32-byte aligned destination address. */ \
  buf = GC_ALIGN_32(buf, GC_cap_ptr *);  \
  GC_PUSH_CAP_REG(16, &buf[0]); \
  GC_PUSH_CAP_REG(17, &buf[1]); \
  GC_PUSH_CAP_REG(18, &buf[2]); \
  GC_PUSH_CAP_REG(19, &buf[3]); \
  GC_PUSH_CAP_REG(20, &buf[4]); \
  GC_PUSH_CAP_REG(21, &buf[5]); \
  GC_PUSH_CAP_REG(22, &buf[6]); \
  GC_PUSH_CAP_REG(23, &buf[7]); \
  GC_PUSH_CAP_REG(24, &buf[8]); \
  GC_PUSH_CAP_REG(25, &buf[9]); \
  GC_PUSH_CAP_REG(26, &buf[10]);

#define GC_RESTORE_CAP_REGS(buf) \
  do { \
    GC_RESTORE_CAP_REG(16, &buf[0]); \
    GC_RESTORE_CAP_REG(17, &buf[1]); \
    GC_RESTORE_CAP_REG(18, &buf[2]); \
    GC_RESTORE_CAP_REG(19, &buf[3]); \
    GC_RESTORE_CAP_REG(20, &buf[4]); \
    GC_RESTORE_CAP_REG(21, &buf[5]); \
    GC_RESTORE_CAP_REG(22, &buf[6]); \
    GC_RESTORE_CAP_REG(23, &buf[7]); \
    GC_RESTORE_CAP_REG(24, &buf[8]); \
    GC_RESTORE_CAP_REG(25, &buf[9]); \
    GC_RESTORE_CAP_REG(26, &buf[10]); \
  } while (0)

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

// Remove all tags from the given region while possibly corrupting the region.
GC_cap_ptr
GC_cap_memclr (GC_cap_ptr dest);

#define GC_MIN(x,y)   ((x)<(y)?(x):(y))
#define GC_MAX(x,y)   ((x)>(y)?(x):(y))

#ifdef GC_GROW_HEAP
struct GC_region;
// Tries to grow the heap by at least `hint' bytes. If successful, returns
// non-zero, otherwise returns zero.
int
GC_grow (struct GC_region * region, size_t hint);
#endif // GC_GROW_HEAP

#endif // GC_LOW_H_HEADER
