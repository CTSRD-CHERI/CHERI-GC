#include "gc_debug.h"
#include "gc_low.h"
#include "gc_time.h"

#include <stdarg.h>
#include <stdio.h>

// see gc_debug.h
/*
void
GC_dbgf2 (const char * file, int line, const char * format, ...)
{
  va_list vl;
  printf("[GC debug] %s:%d ", file, line);
  va_start(vl, format);
  vprintf(format, vl);
  va_end(vl);
  putchar('\n');
}

void
GC_errf2 (const char * file, int line, const char * format, ...)
{
  va_list vl;
  printf("[GC error] %s:%d ", file, line);
  va_start(vl, format);
  vprintf(format, vl);
  va_end(vl);
  putchar('\n');
}
*/

void GC_debug_print_cap (const char * name, GC_cap_ptr cap)
{
  if (GC_cheri_gettag(cap) == 0)
  {
    GC_dbgf("%s: t=0\n", name);
  }
  else
  {
    GC_dbgf(
      "%s: t=1, b=0x%llx, l=0x%llx, alloc=%d"
#ifdef GC_GENERATIONAL
      ", young=%d, cOLD=%d, eph=%d, region=%s"
#endif // GC_GENERATIONAL
      "\n",
      name, (GC_ULL) GC_cheri_getbase(cap), (GC_ULL) GC_cheri_getlen(cap),
      (int) GC_IS_GC_ALLOCATED(cap)
#ifdef GC_GENERATIONAL
      , (int) GC_IS_YOUNG(cap), (int) GC_IS_CONTAINED_IN_OLD(cap),
        (int) GC_IS_EPHEMERAL(cap),
        GC_is_initialized() ?
            GC_IN(cap, GC_state.thread_local_region.tospace) ? "young"
          : GC_IN(cap, GC_state.old_generation.tospace) ? "old"
          : "unknown" \
        : "(GC uninitialized)"
#endif // GC_GENERATIONAL
    );
  }
}

void
GC_debug_print_region_stats(struct GC_region * region)
{
  int fromspace_exists = GC_cheri_gettag(region->fromspace);
  GC_ULL     from  = (fromspace_exists ? 
              (GC_ULL) GC_cheri_getbase(region->fromspace) : 0),
             to    = (GC_ULL) GC_cheri_getbase(region->tospace),
             free  = (GC_ULL) GC_cheri_getbase(region->free),
             scan  = (GC_ULL) region->scan,
#ifdef GC_GENERATIONAL
             old   = (GC_ULL) region->older_region,
#endif // GC_GENERATIONAL
             lfrom = (fromspace_exists ?
              (GC_ULL) GC_cheri_getlen(region->fromspace) : 0),
             lto   = (GC_ULL) GC_cheri_getlen(region->tospace),
             lfree = (GC_ULL) GC_cheri_getlen(region->free),
             efrom = from+lfrom,
             eto = to+lto,
             efree = free+lfree
#ifdef GC_COLLECT_STATS
             , nalloc = (GC_ULL) region->num_allocations,
             ncoll = (GC_ULL) region->num_collections
#endif // GC_COLLECT_STATS
#ifdef GC_TIME
             , tcoll = (GC_ULL) region->time_spent_in_collector
#endif // GC_TIME
             ;
  printf
  (
    "Region statistics\n"
    "-----------------\n"
    "fromspace   : b=  0x%-16llx  l=0x%-16llx\n"
    "              end=0x%-16llx\n"
    "tospace     : b=  0x%-16llx  l=0x%-16llx\n"
    "              end=0x%-16llx\n"
    "free        : b=  0x%-16llx  l=0x%-16llx\n"
    "              end=0x%-16llx\n"
    "scan        :     0x%-16llx\n"
#ifdef GC_GENERATIONAL
    "old         :   0x%-16llx\n"
#endif // GC_GENERATIONAL
    "\n"
    "used size   : 0x%-16llx bytes (%llu%s)\n"
    "free size   : 0x%-16llx bytes (%llu%s)\n"
    "heap size   : 0x%-16llx bytes (%llu%s)\n"
    "\n"
#ifdef GC_COLLECT_STATS
    "stats:\n"
    "allocations : %llu (%llu%s)\n"
    "collections : %llu (%llu%s)\n"
#endif // GC_COLLECT_STATS
#ifdef GC_TIME
    "time spent in collector : %llu%s\n"
#endif // GC_TIME
#ifdef GC_GENERATIONAL
    "This region stores %s objects.\n"
#endif // GC_GENERATIONAL
    ,
    from, lfrom,
    efrom,
    to, lto,
    eto,
    free, lfree,
    efree,
    scan,
#ifdef GC_GENERATIONAL
    old,
#endif // GC_GENERATIONAL
    free - to, GC_MEM_PRETTY(free - to), GC_MEM_PRETTY_UNIT(free - to),
    lfree, GC_MEM_PRETTY(lfree), GC_MEM_PRETTY_UNIT(lfree),
    lto, GC_MEM_PRETTY(lto), GC_MEM_PRETTY_UNIT(lto)
#ifdef GC_COLLECT_STATS
    , nalloc, GC_NUM_PRETTY(nalloc), GC_NUM_PRETTY_UNIT(nalloc),
    ncoll, GC_NUM_PRETTY(ncoll), GC_NUM_PRETTY_UNIT(ncoll)
#endif // GC_COLLECT_STATS
#ifdef GC_TIME
    , GC_TIME_PRETTY(tcoll), GC_TIME_PRETTY_UNIT(tcoll)
#endif // GC_TIME
#ifdef GC_GENERATIONAL
    , old ? "YOUNG" : "OLD"
#endif // GC_GENERATIONAL
  );
}

#include <ucontext.h>
#include <string.h>

static void
GC_debug_print_stack_stats_helper (int arg)
{
  
  int local;
  
  void * stack_ptr = NULL;
  GC_GET_STACK_PTR(stack_ptr);
  GC_dbgf("Stack pointer: 0x%llx\n",
    (GC_ULL) stack_ptr);

  void * frame_ptr = NULL;
  GC_GET_FRAME_PTR(frame_ptr);
  GC_dbgf("Frame pointer: 0x%llx\n",
    (GC_ULL) frame_ptr);
  
  GC_dbgf("Address of a local variable: 0x%llx",
    (GC_ULL) &local);

  GC_dbgf("Address of an argument: 0x%llx",
    (GC_ULL) &arg);
  
  GC_dbgf("GC_get_stack_bottom(): 0x%llx\n",
    (GC_ULL) GC_state.stack_bottom);
}

void
GC_debug_print_stack_stats (void)
{
  GC_debug_print_stack_stats_helper(0);
}

void
GC_debug_capdump (const void * start, const void * end)
{
  if ( ((uintptr_t) end) < ((uintptr_t) start) )
  {
    const void * tmp = end;
    end = start;
    start = tmp;
  }
  
  printf("Capability memory dump from 0x%llx to 0x%llx\n",
    (GC_ULL) start,
    (GC_ULL) end);
  start = GC_ALIGN_32(start, const void *);
  end = GC_ALIGN_32_LOW(end, const void *);  
  
  const void * p;
  for (p = start;
       ((uintptr_t) p) < ((uintptr_t) end);
       p = (const void *) ( ((uintptr_t) p) + sizeof(GC_cap_ptr) ))
  {
    // Do a CLC to $c1, then get the tag
    GC_RESTORE_CAP_REG(1, p);
    unsigned tag = 0;
    GC_CHERI_CGETTAG(tag, 1);
    if (tag)
    {
      const void * base = NULL;
      size_t length = 0;
      GC_CHERI_CGETBASE(base, 1);
      GC_CHERI_CGETLEN(length, 1);
      
      printf("[0x%llx]  t=%u  b=0x%llx  l=0x%llx\n",
        (GC_ULL) p,
        tag,
        (GC_ULL) base,
        (GC_ULL) length);
    }
    else
    {
      printf("[0x%llx]  t=0\n",
        (GC_ULL) p);
    }
  }
}

void
GC_debug_memdump (const void * start, const void * end)
{
  const char * p,
             * start8 = (const void *) (((uintptr_t) start) & ~7),
             * end8 = (const void *) ((((uintptr_t) end)+8) & ~7);
  char buf[34];
  memset(buf, ' ', sizeof buf);
  buf[33] = '\0';
  size_t i = 0;

  printf("Memory dump from 0x%llx to 0x%llx\n",
    (GC_ULL) start,
    (GC_ULL) end);
  
  for (p = start8; p < end8; p++)
  {
    i = ((uintptr_t) p) % 8;
    int out_bounds = (p < (const char *) start || p > (const char *) end);
    buf[i*3+0] = out_bounds ? '?' : "0123456789ABCDEF"[(*p>>4) & 0xF];
    buf[i*3+1] = out_bounds ? '?': "0123456789ABCDEF"[*p & 0xF];
    buf[i+25] =  out_bounds ? '?' : (*p < 32 || *p > 126) ? '.' : *p;
    if (i == 7)
      printf("0x%llx: %s\n", (GC_ULL) ((uintptr_t) p) - 7, buf);
  }
}

#ifdef GC_DEBUG_TRACK_ALLOCATIONS

// To store the addresses of allocated objects, we use a hash table.
#define GC_DEBUG_TBL_SZ     997 /* prime, to avoid synchronisation */
#define GC_DEBUG_ALLOC      malloc
#define GC_DEBUG_REALLOC    realloc
#define GC_DEBUG_DEALLOC    free

static void
GC_debug_allocated_init (void)
{
  static int first_time = 1;
  if (first_time)
  {
    first_time = !first_time;
    GC_debug_tbl.sz = GC_DEBUG_TBL_SZ;
    GC_debug_tbl.tbl = GC_DEBUG_ALLOC(sizeof(GC_debug_arr)*GC_debug_tbl.sz);
    if (!GC_debug_tbl.tbl) GC_fatalf("GC_DEBUG_ALLOC");
    size_t i;
    for (i=0; i<GC_debug_tbl.sz; i++)
    {
      GC_debug_tbl.tbl[i].arr = NULL;
      GC_debug_tbl.tbl[i].sz = 0;
    }
  }
}

static size_t
GC_debug_hash (GC_debug_value v)
{
  return 
  (
    ((unsigned int) (((GC_ULL) v.base) & 0xFFFFFFFF))
    ^ ((unsigned int) ( (((GC_ULL) v.base)>>32) & 0xFFFFFFFF))
    ^ ((unsigned int) (((GC_ULL) v.len) & 0xFFFFFFFF))
    ^ ((unsigned int) ( (((GC_ULL) v.len)>>32) & 0xFFFFFFFF))
  ) % (GC_debug_tbl.sz);
}

static int
GC_debug_value_compare(GC_debug_value u, GC_debug_value v)
{
  return (u.base == v.base) && (u.len == v.len);
}

static GC_debug_value
GC_debug_value_from_cap (GC_cap_ptr cap)
{
  GC_debug_value v;
  v.base = GC_cheri_getbase(cap);
  v.len = GC_cheri_getlen(cap);
  v.valid = 1;
  v.tracking = 0;
  v.tracking_name = NULL;
  v.marked = 0;
  v.file = NULL;
  v.line = 0;
  return v;
}

GC_debug_value *
GC_debug_find_allocated (GC_cap_ptr cap)
{
  GC_debug_allocated_init();
  GC_debug_value v = GC_debug_value_from_cap(cap);
  GC_debug_arr * entry = &GC_debug_tbl.tbl[GC_debug_hash(v)];
  
  if (!entry->arr) return NULL;
  
  size_t i;
  for (i=0; i<entry->sz; i++)
  {
    if (entry->arr[i].valid && GC_debug_value_compare(entry->arr[i], v))
    {
      return &entry->arr[i];
    }
  }
  return NULL;
}

GC_debug_value *
GC_debug_find_invalid (GC_cap_ptr cap)
{
  GC_debug_allocated_init();
  GC_debug_value v = GC_debug_value_from_cap(cap);
  GC_debug_arr * entry = &GC_debug_tbl.tbl[GC_debug_hash(v)];
  
  if (!entry->arr) return NULL;
  
  size_t i;
  for (i=0; i<entry->sz; i++)
  {
    if (!entry->arr[i].valid && GC_debug_value_compare(entry->arr[i], v))
    {
      return &entry->arr[i];
    }
  }
  return NULL;
}

static GC_debug_value *
GC_debug_add_to_hash_table (GC_debug_value v)
{
  GC_debug_arr * entry = &GC_debug_tbl.tbl[GC_debug_hash(v)];
  size_t i;
  for (i=0; i<entry->sz; i++)
  {
    if (!entry->arr[i].valid)
    {
      v.valid = 1;
      entry->arr[i] = v;
      return &entry->arr[i];
    }
  }
  entry->sz++;
  entry->arr = GC_DEBUG_REALLOC(entry->arr, sizeof(GC_debug_value)*entry->sz);
  if (!entry->arr) GC_fatalf("GC_DEBUG_REALLOC");
  entry->arr[entry->sz-1] = v;
  return &entry->arr[entry->sz-1];
}

void
GC_debug_just_allocated (GC_cap_ptr cap, const char * file, int line)
{
  GC_debug_allocated_init();
  GC_debug_value v = GC_debug_value_from_cap(cap);
  size_t len = strlen(file)+1;
  v.file = GC_DEBUG_ALLOC(len);
  if (!v.file) GC_fatalf("GC_DEBUG_ALLOC");
  memcpy(v.file, file, len);
  v.line = line;
  GC_debug_add_to_hash_table(v);
}

int
GC_debug_track_allocated (GC_cap_ptr cap, const char * tracking_name)
{
  GC_debug_allocated_init();
  GC_debug_value * v = GC_debug_find_allocated(cap);
  if (!v) return 1;
  v->tracking = 1;
  if (tracking_name)
  {
    size_t len = strlen(tracking_name)+1;
    v->tracking_name = GC_DEBUG_ALLOC(len);
    if (!v->tracking_name) GC_fatalf("GC_DEBUG_ALLOC");
    memcpy(v->tracking_name, tracking_name, len);
  }
  else
  {
    v->tracking_name = NULL;
  }
  printf("[GC track] Began tracking object %s (allocated in %s:%d):\n",
    v->tracking_name ? v->tracking_name : "(null)", v->file, v->line);
  GC_PRINT_CAP(cap);
  return 0;
}

// Be very careful if you modify the structure of the hash table itself if you
// use this!
#define GC_DEBUG_HASH_TABLE_FOR_EACH_VALID(v,cmd) \
  do { \
    size_t j; \
    for (j=0; j<GC_debug_tbl.sz; j++) \
    { \
      GC_debug_arr * entry = &GC_debug_tbl.tbl[j]; \
      if (entry->arr) \
      { \
        size_t i; \
        for (i=0; i<entry->sz; i++) \
        { \
          if (entry->arr[i].valid) \
          { \
            v = &entry->arr[i]; \
            cmd \
          } \
        } \
      } \
    } \
  } while (0)
  
static GC_debug_value *
GC_debug_move_allocation (GC_debug_value * old, void * newbase, int newlen,
                          const char * reason)
{
  if (old->tracking)
  {
    printf("[GC track] Object %s moved (reason: %s)\n",
      old->tracking_name ? old->tracking_name : "(null)", reason);
    printf("[GC track] old: b=0x%llx, l=0x%llx\n",
      (GC_ULL) old->base, (GC_ULL) old->len);
    printf("[GC track] new: b=0x%llx, l=0x%llx\n",
      (GC_ULL) newbase, (GC_ULL) newlen);
  }
  old->marked = 1;
  GC_debug_value u = *old;
  u.base = newbase;
  u.len = newlen;
  u.marked = 1;
  old->valid = 0;
  return GC_debug_add_to_hash_table(u);
}

void
GC_debug_just_copied (GC_cap_ptr old_cap, GC_cap_ptr new_cap)
{
  GC_debug_allocated_init();
  GC_debug_value * v = GC_debug_find_allocated(old_cap);
  if (!v)
  {
    GC_dbgf(
      "WARNING: invalid capability just copied.\n"
      "Old capability:\n");
    GC_PRINT_CAP(old_cap);
    size_t oldhash = GC_debug_hash(GC_debug_value_from_cap(old_cap));
    GC_dbgf("Hash: %d\n", (int) oldhash);
    GC_dbgf("New capability:\n");
    GC_PRINT_CAP(new_cap);
    GC_dbgf("Hash: %d\n", (int) GC_debug_hash(GC_debug_value_from_cap(new_cap)));
    v = GC_debug_find_invalid(old_cap);
    if (v)
    {
      GC_dbgf("The capability was previously allocated, initially at %s:%d.",
        v->file, v->line);
    }
    /*GC_dbgf("All known valid objects of the same length:\n");
    GC_DEBUG_HASH_TABLE_FOR_EACH_VALID(v,
    {
      if (v->len == GC_cheri_getlen(old_cap))
      {
        GC_dbgf("hash=%d  %s:%d  b=0x%llx, l=0x%llx, j=%d, i=%d\n",
          (int) GC_debug_hash(*v), v->file, v->line, (GC_ULL) v->base,
          (GC_ULL) v->len, (int) j, (int) i);
        if (v->base == GC_cheri_getbase(old_cap)) exit(1);
      }
    });*/
    /*GC_dbgf("All known valid objects with the same hash:\n");
    {
      GC_debug_arr * entry = &GC_debug_tbl.tbl[oldhash];
      if (entry->arr)
      {
        size_t i;
        for (i=0; i<entry->sz; i++)
        {
          if (entry->arr[i].valid &&
              (GC_debug_hash(entry->arr[i]) == oldhash))
          {
            v = &entry->arr[i];
            GC_dbgf("%s:%d b=0x%llx, l=0x%llx\n",
              v->file, v->line, (GC_ULL) v->base, (GC_ULL) v->len);
          }
        }
      }
      else
      {
        GC_dbgf("(none)");
      }
    }*/
    GC_dbgf("Here's a memdump of the invalid capability:");
    GC_debug_memdump(
      GC_cheri_getbase(old_cap),
      GC_cheri_getbase(old_cap)+GC_cheri_getlen(old_cap));
    GC_debug_print_region_stats(&GC_state.thread_local_region);
    GC_fatalf("exiting.");
  }
  else
  {
    GC_debug_move_allocation(
      v, GC_cheri_getbase(new_cap), GC_cheri_getlen(new_cap), "copy collect");
  }
}


void
GC_debug_rebase_allocation_entries (void * oldbase,
                                    size_t oldsize,
                                    void * newbase)
{
  GC_debug_value * v;
  GC_DEBUG_HASH_TABLE_FOR_EACH_VALID(v,
  {
    v->rebased = 0;
  });
  // NOTE: it's okay to modify the hash table structure here, as we're only
  // adding entries we won't want to scan again and not removing any (so we
  // can't possibly skip over an interesting one).
  GC_DEBUG_HASH_TABLE_FOR_EACH_VALID(
    v,
    {
      if (!v->rebased && (v->base >= oldbase)
          && (v->base <= (oldbase+oldsize)))
      {
        v->rebased = 1;
        v = GC_debug_move_allocation(
          v, v->base-oldbase+newbase, v->len, "rebase");
        v->rebased = 1;
      }
    }
  );
}

void
GC_debug_begin_marking (void)
{
  GC_debug_allocated_init();
  GC_START_TIMING(GC_debug_begin_marking_time);
  GC_debug_value * v = NULL;
  GC_DEBUG_HASH_TABLE_FOR_EACH_VALID(
    v,
    {v->marked = 0;}
  );
  //GC_STOP_TIMING(GC_debug_begin_marking_time, "GC_debug_begin_marking");
}

void
GC_debug_end_marking (void * space_start, void * space_end)
{
  GC_debug_allocated_init();
  GC_START_TIMING(GC_debug_end_marking_time);
  GC_debug_value * v = NULL;
  GC_DEBUG_HASH_TABLE_FOR_EACH_VALID(
    v,
    {
      if (!v->marked && (v->base >= space_start) && (v->base <= space_end))
      {
        if (v->tracking)
        {
          printf("[GC track] Object %s (b=0x%llx, l=0x%llx) deallocated\n",
            v->tracking_name ? v->tracking_name : "(null)",
            (GC_ULL) v->base, (GC_ULL) v->len);
          if (v->tracking_name) GC_DEBUG_DEALLOC(v->tracking_name);
        }
        v->valid = 0;
      }
    }
  );
  //GC_STOP_TIMING(GC_debug_end_marking_time, "GC_debug_end_marking");
}

void
GC_debug_print_allocated_stats (void)
{
  GC_debug_allocated_init();
  GC_ULL total_allocated = 0,
         total_valid_entries = 0,
         total_invalid_entries = 0,
         total_slots_used = 0,
         total_slots = GC_debug_tbl.sz;
  
  size_t j;
  for (j=0; j<GC_debug_tbl.sz; j++)
  {
    GC_debug_arr * entry = &GC_debug_tbl.tbl[j];
    
    if (entry->arr)
    {
      total_slots_used++;
      size_t i;
      for (i=0; i<entry->sz; i++)
      {
        if (entry->arr[i].valid)
        {
          total_valid_entries++;
          total_allocated += entry->arr[i].len;
        }
        else
        {
          total_invalid_entries++;
        }
      }
    }
  }
  
  printf(
    "Hash table stats\n"
    "----------------\n"
    "Total allocated (incl aliasing)    :     %llu B (%llu%s)\n"
    "Total valid entries                :     %llu (%llu%s)\n"
    "Total invalid entries              :     %llu (%llu%s)\n"
    "Total used slots (incl invalid)    :     %llu (%llu%s)\n"
    "Total slots (table size)           :     %llu (%llu%s)\n",
    total_allocated, GC_MEM_PRETTY(total_allocated), GC_MEM_PRETTY_UNIT(total_allocated),
    total_valid_entries, GC_NUM_PRETTY(total_valid_entries), GC_NUM_PRETTY_UNIT(total_valid_entries),
    total_invalid_entries, GC_NUM_PRETTY(total_invalid_entries), GC_NUM_PRETTY_UNIT(total_invalid_entries),
    total_slots_used, GC_NUM_PRETTY(total_slots_used), GC_NUM_PRETTY_UNIT(total_slots_used),
    total_slots, GC_NUM_PRETTY(total_slots), GC_NUM_PRETTY_UNIT(total_slots)
  );
  
}
#endif // GC_DEBUG_TRACK_ALLOCATIONS