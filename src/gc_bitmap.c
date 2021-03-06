#include "gc_common.h"
#include "gc_bitmap.h"
#include "gc_debug.h"
#include "gc_low.h"

#include <string.h>

GC_FUNC int
GC_init_bitmap (struct GC_bitmap * bitmap,
                size_t size)
{
  bitmap->size = size;
  bitmap->used = 0;
  bitmap->map = GC_low_malloc(GC_BITMAP_BITS_TO_BYTES(size));
  if (bitmap->map == NULL) return 1;
  GC_bitmap_clr(bitmap);
  return 0;
}

GC_FUNC int
GC_grow_bitmap (struct GC_bitmap * bitmap, size_t new_size)
{
  GC_assert( new_size >= bitmap->size );
  char * tmp = GC_low_realloc(bitmap->map, GC_BITMAP_BITS_TO_BYTES(new_size), 0);
  if (!tmp) return 1;
  bitmap->map = tmp;
  memset(
    bitmap->map+GC_BITMAP_BITS_TO_BYTES(bitmap->size),
    0,
    GC_BITMAP_BITS_TO_BYTES(new_size)-GC_BITMAP_BITS_TO_BYTES(bitmap->size));
  bitmap->size = new_size;
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
  memset(bitmap->map, 0, GC_BITMAP_BITS_TO_BYTES(bitmap->size));
  bitmap->used = 0;
}

GC_FUNC int
GC_bitmap_find (struct GC_bitmap * bitmap,
                size_t pos,
                size_t len)
{
  if (!len) return 0;
  if (pos+len > bitmap->used) return 0;
  if (!GC_BITMAP_GET(bitmap, pos)) return 0;
  if ((pos+len != bitmap->used) && !GC_BITMAP_GET(bitmap, pos+len)) return 0;
  int i;
  for (i=1; i<len; i++)
    if (GC_BITMAP_GET(bitmap, pos+i)) return 0;
  return 1;
}

GC_FUNC int
GC_bitmap_get_container (struct GC_bitmap * bitmap,
                         size_t pos,
                         size_t * relbase_out,
                         size_t * len_out)
{
  size_t opos = pos;
  if (pos >= bitmap->used) return 0;
  while (1)
  {
    if (GC_BITMAP_GET(bitmap, pos))
    {
      *relbase_out = opos-pos;
      *len_out = 1;
      pos++;
      while ((pos != bitmap->used) && !GC_BITMAP_GET(bitmap, pos))
      {
        (*len_out)++;
        pos++;
      }
      return 1;
    }
    if (!pos) break;
    pos--;
  }
  return 0;
}