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
#define     GC_cheri_setlen   cheri_setlen
#define     GC_cheri_ptr      cheri_ptr
#define     GC_CHERI_CGETTAG  CHERI_CGETTAG
#define     GC_CHERI_CGETBASE CHERI_CGETBASE
#define     GC_CHERI_CGETLEN  CHERI_CGETLEN
#define     GC_cheri_getreg   cheri_getreg
#define     GC_cheri_setreg   cheri_setreg

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

#endif // GC_LOW_H_HEADER
