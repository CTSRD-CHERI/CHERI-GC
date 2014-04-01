#include "gc_init.h"
#include "gc_debug.h"
#include "gc_low.h"
#include "gc_config.h"
#include "gc_time.h"
#include "gc_remset.h"

#include <stdio.h>
#include <stdlib.h>

struct GC_state_struct GC_state = {.initialized = 0};
__capability struct GC_state_struct * GC_state_cap;

int
GC_init2 (void * arg_for_stack_bottom, const char * file, int line)
{
  if (!GC_state.initialized)
  {
    GC_dbgf("GC_init called from %s:%d\nGC compiled %s",
      file, line, __TIME__ " " __DATE__);
    
    GC_state_cap = GC_cheri_ptr(&GC_state, sizeof GC_state);

    int rc;
#ifdef GC_GENERATIONAL
    rc = GC_init_old_region(
      &GC_state.old_generation,
      GC_OLD_GENERATION_SEMISPACE_SIZE,
      GC_OLD_GENERATION_SEMISPACE_MAX_SIZE_BEFORE_COLLECTION,
      GC_OLD_GENERATION_SEMISPACE_MAX_SIZE);
    if (rc) return rc;
    
    rc = GC_init_young_region(
      &GC_state.thread_local_region,
      &GC_state.old_generation,
      GC_THREAD_LOCAL_HEAP_SIZE,
      GC_THREAD_LOCAL_HEAP_MAX_SIZE_BEFORE_COLLECTION,
      GC_THREAD_LOCAL_HEAP_MAX_SIZE);
    if (rc) return rc;
#ifdef GC_WB_RUNTIME
    GC_state.wb_type = GC_WB_DEFAULT;
#endif // GC_WB_RUNTIME
#else // GC_GENERATIONAL
    rc = GC_init_region(
      &GC_state.thread_local_region,
      GC_THREAD_LOCAL_HEAP_SIZE,
      GC_THREAD_LOCAL_HEAP_MAX_SIZE_BEFORE_COLLECTION,
      GC_THREAD_LOCAL_HEAP_MAX_SIZE);
    if (rc) return rc;
#endif // GC_GENERATIONAL
    
    //GC_state.stack_bottom = GC_get_stack_bottom();
    GC_state.stack_bottom = arg_for_stack_bottom;
    if (GC_state.stack_bottom == NULL) return 1;
    GC_dbgf("The stack bottom is probably near 0x%llx\n",
      (GC_ULL) GC_state.stack_bottom);
      
    GC_state.stack_top = GC_MAX_STACK_TOP;
    GC_CLEAN_STACK();
    
    GC_state.static_bottom = GC_get_static_bottom();
    if (GC_state.stack_bottom == NULL) return 1;

    GC_state.static_top = GC_get_static_top();
    if (GC_state.stack_bottom == NULL) return 1;    
      
    GC_state.initialized = 1;
    GC_dbgf("initialized");
  }
  return 0;
}

int
GC_is_initialized (void)
{
  return GC_state.initialized;
}

int
#ifdef GC_GENERATIONAL
GC_init_old_region
#else // GC_GENERATIONAL
GC_init_region
#endif // GC_GENERATIONAL
(struct GC_region * region,
 size_t semispace_size,
 size_t max_grow_size_before_collection,
 size_t max_grow_size_after_collection)
{
  // WARNING: if you change how this is allocated, you must change how GC_grow
  // works.
  semispace_size = GC_ALIGN_32(semispace_size, size_t);
  void * p = GC_low_malloc(semispace_size+32);
  if (p == NULL)
  {
    GC_errf("GC_low_malloc(%llu) (tospace)", (GC_ULL) (semispace_size+32));
    return 1;
  }
  region->tospace_misaligned = p;
  p = GC_ALIGN_32(p, void *);
  region->tospace = GC_cheri_ptr(p, semispace_size);
  // If we do this here, we get stray pointers. Must do it on GC_malloc only
  // (and when we copy an object).
  //region->tospace = GC_SET_GC_ALLOCATED(region->tospace);
  
  // GC_grow assumes this is a separate region of memory.
  p = GC_low_malloc(semispace_size+32);
  if (p == NULL)
  {
    GC_errf("GC_low_malloc(%llu) (fromspace)", (GC_ULL) (semispace_size+32));
    return 1;
  }
  region->fromspace_misaligned = p;
  p = GC_ALIGN_32(p, void *);
  region->fromspace = GC_cheri_ptr(p, semispace_size);
  //region->fromspace = GC_SET_GC_ALLOCATED(region->fromspace);
  
  region->free = region->tospace;
  region->scan = NULL;
#ifdef GC_GENERATIONAL
  region->older_region = NULL;
#if (GC_OY_STORE_DEFAULT == GC_OY_STORE_REMEMBERED_SET)
  region->remset = NULL;
#endif // GC_OY_STORE_DEFAULT
#endif // GC_GENERATIONAL
  region->num_collections = 0;
#ifdef GC_GROW_HEAP
  region->max_grow_size_before_collection =
    GC_ALIGN_32(max_grow_size_before_collection, size_t);
  region->max_grow_size_after_collection =
    GC_ALIGN_32(max_grow_size_after_collection, size_t);
#endif // GC_GROW_HEAP
#ifdef GC_TIME
  region->time_spent_in_collector = 0;
#endif // GC_TIME
  return 0;
}

#ifdef GC_GENERATIONAL
int
GC_is_young (struct GC_region * region)
{
  return (GC_cheri_getbase(region->older_region) != NULL);
}

int
GC_init_young_region (struct GC_region * region,
                      struct GC_region * older_region,                      
                      size_t sz,
                      size_t max_grow_size_before_collection,
                      size_t max_grow_size_after_collection)
{
  // WARNING: if you change how this is allocated, you must change how GC_grow
  // works.
  sz = GC_ALIGN_32(sz, size_t);
  void * p = GC_low_malloc(sz+32);
  if (p == NULL)
  {
    GC_errf("GC_low_malloc");
    return 1;
  }
  region->tospace_misaligned = p;
  p = GC_ALIGN_32(p, void *);
  region->tospace = GC_cheri_ptr(p, sz);

  GC_SWITCH_WB_TYPE(
    {region->tospace = GC_SET_YOUNG(region->tospace);},      // GC_WB_MANUAL
    {region->tospace = GC_SET_EPHEMERAL(region->tospace);}   // GC_WB_EPHEMERAL
  );
  
  //region->tospace = GC_SET_GC_ALLOCATED(region->tospace);
  
  region->fromspace_misaligned = NULL;
  region->fromspace = GC_INVALID_PTR;
  region->free = region->tospace;
  region->scan = NULL;
  region->older_region = older_region;
  region->num_collections = 0;
#ifdef GC_GENERATIONAL
#if (GC_OY_STORE_DEFAULT == GC_OY_STORE_REMEMBERED_SET)
  region->remset = GC_low_malloc(sizeof(struct GC_remembered_set));
  if (region->remset == NULL)
  {
    GC_errf("GC_low_malloc(%llu) (remset)",
      (GC_ULL) (sizeof(struct GC_remembered_set)));
    return 1;
  }
  GC_remembered_set_init(region->remset);
#endif // GC_OY_STORE_DEFAULT
#endif // GC_GENERATIONAL
#ifdef GC_GROW_HEAP
  region->max_grow_size_before_collection =
    GC_ALIGN_32(max_grow_size_before_collection, size_t);
  region->max_grow_size_after_collection =
    GC_ALIGN_32(max_grow_size_after_collection, size_t);
#endif // GC_GROW_HEAP
#ifdef GC_TIME
  region->time_spent_in_collector = 0;
#endif // GC_TIME
  return 0;
}

int
GC_set_wb_type (int wb_type)
{
  if (!GC_is_initialized)
  {
    //int rc = GC_init();
    //if (rc) return rc;
    GC_fatalf("GC not initialized, call GC_init from main.");
  }
  GC_state.wb_type = wb_type;
  return 0;
}
#endif // GC_GENERATIONAL
