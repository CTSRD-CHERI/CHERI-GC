#ifndef GC_BITMAP_H_HEADER
#define GC_BITMAP_H_HEADER

#include "gc_common.h"
#include "gc_low.h"
#include "gc_config.h"

struct GC_bitmap
{
  char * map;
  size_t size;    // size of the map in bits (no. of entries)
  size_t used;    // used size in bits
};

// returns 0 on success and non-zero on failure
GC_FUNC int
GC_init_bitmap (struct GC_bitmap * bitmap,
                size_t size);

// returns 0 on success and non-zero on failure
GC_FUNC int
GC_grow_bitmap (struct GC_bitmap * bitmap,
                size_t new_size);

// can only set in increments (fill up lower addresses first), just like the
// copying allocator.
GC_FUNC void
GC_bitmap_allocate (struct GC_bitmap * bitmap,
                    size_t len);

GC_FUNC void
GC_bitmap_clr (struct GC_bitmap * bitmap);

// returns non-zero if the entry is found
#define GC_bitmap_find(x,y,z) GC_bitmap_find3(__FILE__,__LINE__,(x),(y),(z))
GC_FUNC int
GC_bitmap_find3 (const char * file, int line, struct GC_bitmap * bitmap,
                size_t pos,
                size_t len);

#define GC_BITMAP_BITS_TO_BYTES(size) ( ((size)+7)/8 )

#define GC_BITMAP_BIG_INDEX(index)   ( (index)/8 )
#define GC_BITMAP_SMALL_INDEX(index) ( (index)%8 )

#define GC_BITMAP_GET(bitmap,index) \
  ( ((bitmap)->map[GC_BITMAP_BIG_INDEX(index)] >> \
    GC_BITMAP_SMALL_INDEX(index)) & 1 )

#define GC_BITMAP_SET(bitmap,index) \
  ( (bitmap)->map[GC_BITMAP_BIG_INDEX(index)] |= \
    (1 << GC_BITMAP_SMALL_INDEX(index)) )

#define GC_BITMAP_CLR(bitmap,index) \
  ( (bitmap)->map[GC_BITMAP_BIG_INDEX(index)] &= \
    ~(1 << GC_BITMAP_SMALL_INDEX(index)) )
    
#ifdef GC_USE_BITMAP
#define GC_IS_IN_BITMAP(bitmap,cap,base) \
  ( \
    GC_bitmap_find( \
      (bitmap), \
      ((size_t) (GC_cheri_getbase((cap)) - (base))) / 32, \
      (GC_ALIGN_32(GC_cheri_getlen((cap)), size_t)) / 32) \
  )
#define GC_ADD_TO_BITMAP(bitmap,cap) \
  ( \
    GC_bitmap_allocate( \
      (bitmap), \
      (GC_ALIGN_32(GC_cheri_getlen((cap)), size_t)) / 32) \
  )
#else // GC_USE_BITMAP
#define GC_IS_IN_BITMAP(bitmap,cap,base) (1)
#define GC_ADD_TO_BITMAP(bitmap,cap) do{}while(0)
#endif // GC_USE_BITMAP

#define GC_IS_IN_FROMSPACE_BITMAP(region,cap) \
  GC_IS_IN_BITMAP((region)->fromspace_bitmap,(cap),GC_cheri_getbase((region)->fromspace))

#define GC_IS_IN_TOSPACE_BITMAP(region,cap) \
  GC_IS_IN_BITMAP((region)->tospace_bitmap,(cap),GC_cheri_getbase((region)->tospace))

#endif // GC_BITMAP_H_HEADER
