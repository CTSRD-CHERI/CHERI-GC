#include "gc_common.h"
#include "gc_bitmap.h"
#include "gc_debug.h"

#include <string.h>

GC_FUNC int
GC_init_bitmap (struct GC_bitmap * bitmap,
                size_t size)
{
  bitmap->size = size;
  bitmap->used = 0;
  bitmap->map = GC_low_malloc((size+7)/8);
  if (bitmap->map == NULL) return 1;
  GC_bitmap_clr(bitmap);
  return 0;
}

GC_FUNC void
GC_bitmap_allocate (struct GC_bitmap * bitmap,
                    size_t len)
{
  GC_BITMAP_SET(bitmap, bitmap->used);
  bitmap->used += len;
  GC_assert( bitmap->used <= bitmap->size );
}

GC_FUNC void
GC_bitmap_clr (struct GC_bitmap * bitmap)
{
  memset(bitmap->map, 0, bitmap->size);
}

GC_FUNC int
GC_bitmap_find (struct GC_bitmap * bitmap,
                size_t pos,
                size_t len)
{
  if (pos+len > bitmap->used) return 0;
  if (!len) return 0;
  if (!GC_BITMAP_GET(bitmap, pos)) return 0;
  if (!GC_BITMAP_GET(bitmap, pos+len)) return 0;
  int i;
  for (i=1; i<len; i++)
    if (GC_BITMAP_GET(bitmap, pos+i)) return 0;
  return 1;
}
