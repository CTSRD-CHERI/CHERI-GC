#include "gc_debug.h"
#include "gc_low.h"

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
      "%s: t=1, b=0x%llx, l=0x%llx"
#ifdef GC_GENERATIONAL
      ", young=%d, cOLD=%d, eph=%d, region=%s"
#endif // GC_GENERATIONAL
      "\n",
      name, (GC_ULL) GC_cheri_getbase(cap), (GC_ULL) GC_cheri_getlen(cap)
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
GC_debug_print_region_stats(struct GC_region region)
{
  GC_ULL     from  = (GC_ULL) GC_cheri_getbase(region.fromspace),
             to    = (GC_ULL) GC_cheri_getbase(region.tospace),
             free  = (GC_ULL) GC_cheri_getbase(region.free),
             scan  = (GC_ULL) region.scan,
#ifdef GC_GENERATIONAL
             old   = (GC_ULL) region.older_region,
#endif // GC_GENERATIONAL
             lfrom = (GC_ULL) GC_cheri_getlen(region.fromspace),
             lto   = (GC_ULL) GC_cheri_getlen(region.tospace),
             lfree = (GC_ULL) GC_cheri_getlen(region.free),
             ncoll = (GC_ULL) region.num_collections;
  printf
  (
    "Region statistics\n"
    "-----------------\n"
    "fromspace   : b=0x%-16llx  l=0x%-16llx\n"
    "tospace     : b=0x%-16llx  l=0x%-16llx\n"
    "free        : b=0x%-16llx  l=0x%-16llx\n"
    "scan        :   0x%-16llx\n"
#ifdef GC_GENERATIONAL
    "old         :   0x%-16llx\n"
#endif // GC_GENERATIONAL
    "\n"
    "used size   : 0x%-16llx bytes (%llu%s)\n"
    "free size   : 0x%-16llx bytes (%llu%s)\n"
    "heap size   : 0x%-16llx bytes (%llu%s)\n"
    "collections : %llu (%llu%s)\n"
#ifdef GC_GENERATIONAL
    "This region stores %s objects.\n"
#endif // GC_GENERATIONAL
    ,
    from, lfrom,
    to, lto,
    free, lfree,
    scan,
#ifdef GC_GENERATIONAL
    old,
#endif // GC_GENERATIONAL
    free - to, GC_MEM_PRETTY(free - to), GC_MEM_PRETTY_UNIT(free - to),
    lfree, GC_MEM_PRETTY(lfree), GC_MEM_PRETTY_UNIT(lfree),
    lto, GC_MEM_PRETTY(lto), GC_MEM_PRETTY_UNIT(lto),
    ncoll, GC_NUM_PRETTY(ncoll), GC_NUM_PRETTY_UNIT(ncoll)
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

// To store the addresses of allocated objects, we use a hash table.
#define GC_DEBUG_TBL_SZ     997 /* prime, to avoid synchronisation */
#define GC_DEBUG_ALLOC      malloc
#define GC_DEBUG_REALLOC    realloc
typedef struct
{
  void * base;
  size_t len;
  int valid;
} GC_debug_value; // isn't an actual capability to avoid clashes with GC
typedef struct
{
  GC_debug_value * arr;
  unsigned int sz;
} GC_debug_arr;
struct
{
  GC_debug_arr * tbl;
  unsigned int sz;
} GC_debug_tbl;

static void
GC_debug_allocated_init (void)
{
  static int first_time = 1;
  if (first_time)
  {
    first_time = !first_time;
    GC_debug_tbl.sz = GC_DEBUG_TBL_SZ;
    GC_debug_tbl.tbl = GC_DEBUG_ALLOC(sizeof(GC_debug_arr)*GC_debug_tbl.sz);
    if (!GC_debug_tbl.tbl) {GC_errf("GC_DEBUG_ALLOC"); exit(0);}
    unsigned int i;
    for (i=0; i<GC_debug_tbl.sz; i++)
    {
      GC_debug_tbl.tbl[i].arr = NULL;
      GC_debug_tbl.tbl[i].sz = 0;
    }
  }
}

static unsigned int
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
  return v;
}

int
GC_debug_is_allocated (GC_cap_ptr cap)
{
  GC_debug_allocated_init();
  GC_debug_value v = GC_debug_value_from_cap(cap);
  GC_debug_arr * entry = &GC_debug_tbl.tbl[GC_debug_hash(v)];
  
  if (!entry->arr) return 0;
  
  unsigned int i;
  for (i=0; i<entry->sz; i++)
  {
    if (entry->arr[i].valid && GC_debug_value_compare(entry->arr[i], v))
    {
      return 1;
    }
  }
  return 0;
}

void
GC_debug_just_allocated (GC_cap_ptr cap)
{
  GC_debug_allocated_init();
  GC_debug_value v = GC_debug_value_from_cap(cap);
  GC_debug_arr * entry = &GC_debug_tbl.tbl[GC_debug_hash(v)];
  
  unsigned int i;
  for (i=0; i<entry->sz; i++)
  {
    if (!entry->arr[i].valid)
    {
      entry->arr[i] = v;
      return;
    }
  }
  
  entry->sz++;
  entry->arr = GC_DEBUG_REALLOC(entry->arr, sizeof(GC_debug_value)*entry->sz);
  entry->arr[entry->sz-1] = v;
}

void
GC_debug_just_deallocated (GC_cap_ptr cap)
{
  GC_debug_allocated_init();
  GC_debug_value v = GC_debug_value_from_cap(cap);
  GC_debug_arr * entry = &GC_debug_tbl.tbl[GC_debug_hash(v)];
  
  if (entry->arr)
  {  
    unsigned int i;
    for (i=0; i<entry->sz; i++)
    {
      if (entry->arr[i].valid && GC_debug_value_compare(entry->arr[i], v))
      {
        entry->arr[i].valid = 0;
        return;
      }
    }
  }
  GC_dbgf("WARNING: GC_debug_just_deallocated() could not find an allocated"
          " entry for b=0x%llx, l=0x%llx, hash=%u.\n",
          (GC_ULL) GC_cheri_getbase(cap), (GC_ULL) GC_cheri_getlen(cap),
          GC_debug_hash(v));
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
  
  unsigned int j;
  for (j=0; j<GC_debug_tbl.sz; j++)
  {
    GC_debug_arr * entry = &GC_debug_tbl.tbl[j];
    
    if (entry->arr)
    {
      total_slots_used++;
      unsigned int i;
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
    "Total allocated                    :     %llu B (%llu%s)\n"
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