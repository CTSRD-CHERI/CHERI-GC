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

// can only set in increments (fill up lower addresses first), just like the
// copying allocator.
GC_FUNC void
GC_bitmap_allocate (struct GC_bitmap * bitmap,
                    size_t len);

GC_FUNC void
GC_bitmap_clr (struct GC_bitmap * bitmap);

// returns non-zero if the entry is found
GC_FUNC int
GC_bitmap_find (struct GC_bitmap * bitmap,
                size_t pos,
                size_t len);

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

#endif // GC_BITMAP_H_HEADER
