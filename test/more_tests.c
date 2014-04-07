#include <gc_init.h>
#include <gc_malloc.h>
#include <gc_debug.h>
#include <gc_collect.h>
#include <gc_low.h>
#include <gc_config.h>

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#define ATTR_SENSITIVE __attribute__((sensitive))
//#define ATTR_SENSITIVE

typedef struct
{
  GC_CAP void * ptr;
} PtrContainer;

struct struct1
{
  __capability void * ptr;
};

typedef struct node_tag
{
  int value;
  __capability struct node_tag * next;
} node;


void
collection_test (void);
void
rebase_test (void);
void
grow_test (void);
void
tracking_test (void);
void
remset_test (void);
void
low_realloc_test (void);
void
realloc_preserves_caps_test (void);
void
fill_test (void);
void
stack_fill_test (void);
void
oy_test (void);
ATTR_SENSITIVE void
stack_test (void);

int
main (int argc, char **argv)
{
  //GC_init();
  //remset_test();
  //tracking_test();
  //rebase_test();
  //collection_test();
  //grow_test();
  //low_realloc_test();
  //realloc_preserves_caps_test();
  //fill_test();
  //stack_fill_test();
  //remset_test();
  //oy_test();
  //stack_test();
  return 0;
}

ATTR_SENSITIVE __capability void *
GC_cheri_ptr2 (void * x, size_t len)
{
  return cheri_setlen(cheri_incbase(cheri_getreg(0), x), len);
}

ATTR_SENSITIVE void
stack_mess (int tmp, __capability void * arg);
ATTR_SENSITIVE void
stack_mess (int tmp, __capability void * arg)
{
  if (tmp) stack_mess(tmp-1,arg);
  __capability void * my_local;
  my_local = GC_cheri_ptr2(tmp, 0x5678);
  arg = GC_cheri_ptr2((void*)0x9988, 0x7766);
  //*((__capability void **) (((char*)&arg))) = cheri_ptr(0x1111, 0x2222);
}

ATTR_SENSITIVE void
stack_test (void)
{
  char local;
  void * max_top = &local-0x1000;
  GC_assert( max_top > GC_MAX_STACK_TOP );
  GC_debug_capdump(max_top, &local);
  printf("done\n");
  stack_mess(10, GC_INVALID_PTR);
  printf("done2\n");
  // Should get nothing printed out if __attribute__((sensitive)) works properly
  GC_debug_capdump(max_top, &local);
  printf("max_top: 0x%llx, &local: 0x%llx\n", (GC_ULL) max_top, (GC_ULL) &local);
}

void
oy_test (void)
{
#ifdef GC_GENERATIONAL
#if (GC_OY_STORE_DEFAULT == GC_OY_STORE_REMEMBERED_SET)
  GC_CAP PtrContainer * P = GC_INVALID_PTR;
  GC_STORE_CAP(P, GC_malloc(sizeof(PtrContainer)));
  #define CHILDSIZE 50
  while (!GC_IN(P, GC_state.old_generation.tospace))
  {
    GC_malloc(50);
  }
  GC_CAP void * child = GC_malloc(CHILDSIZE);
  if (!GC_IN(child, GC_state.thread_local_region.tospace))
  {
    GC_fatalf("Young pointer not young...\n");
  }
  
  GC_PRINT_CAP(P);
  GC_PRINT_CAP(((PtrContainer*)P)->ptr);
  
  ((PtrContainer*)P)->ptr = GC_INVALID_PTR;
  GC_STORE_CAP(((PtrContainer*)P)->ptr, child);
  memset((void*) ((PtrContainer*)P)->ptr, 0x22, CHILDSIZE);
  GC_debug_memdump((void*) ((PtrContainer*)P)->ptr, ((void*) ((PtrContainer*)P)->ptr ) + CHILDSIZE);

  //void * child_addr = &child;
  
  // Forget the root.
  child = NULL;

  // Collect
  GC_collect();
  
  // Check that it moved
  if (!GC_IN((void*) ((PtrContainer*)P)->ptr, GC_state.old_generation.tospace))
  {
    printf("Warning: young pointer has not been promoted.\n");
  }

  // Clear the young region
  memset(GC_cheri_getbase(GC_state.thread_local_region.tospace), 0x36, GC_cheri_getlen(GC_state.thread_local_region.tospace));
  
  GC_debug_memdump((void*) ((PtrContainer*)P)->ptr, ((void*) ((PtrContainer*)P)->ptr ) + CHILDSIZE);
#endif // GC_OY_STORE_DEFAULT
#endif // GC_GENERATIONAL
}

void
stack_fill_test (void)
{
  void * stack_ptr = NULL;
  GC_GET_STACK_PTR(stack_ptr);
  GC_cap_ptr * p;
  for (p=GC_ALIGN_32(GC_MAX_STACK_TOP, GC_cap_ptr *);
       p<GC_ALIGN_32_LOW(stack_ptr, GC_cap_ptr *);
       p++)
  {
    *p = GC_SET_GC_ALLOCATED(
      GC_setbaselen(
        GC_state.thread_local_region.tospace,
        GC_cheri_getbase(GC_state.thread_local_region.tospace)+0x80,
        0x20));
  }
}

static void
fill_test_helper (void)
{
  int i;
  GC_CAP void * store[20];
  GC_CAP void * y;
  void * stack_top = NULL;
  GC_GET_STACK_PTR(stack_top);
  printf("fill_test_helper(): stack top: 0x%llx\n", (GC_ULL) stack_top);
  for (i=0; i<10000; i++)
  {
    y = GC_malloc(500);
    store[i % 20] = y;
    memset((void*)y, 0x22, 500);
  }
  struct GC_region * region = &GC_state.thread_local_region;
  memset(GC_cheri_getbase(region->fromspace), 0x55, GC_cheri_getlen(region->fromspace));
}

void
fill_test (void)
{
  
  // from gc_low:
  // 0x41: just realloc'd
  // 0x42: just malloc'd
  // 0x43: just deallocated, set by GC_debug when GC_DEBUG_TRACK_ALLOCATIONS is defined
  // 0x44: just memclr'd
  
  // our codes:
  // 0x33: our data
  // 0x22: temporary data
  
  //GC_init();
__asm__("daddiu $6, $6, 0");
  GC_CAP struct struct1 * a = GC_malloc(600);
__asm__("daddiu $7, $7, 0");
__asm__("daddiu $6, $6, 0");
  GC_CAP struct struct1 * b = GC_malloc(600);
__asm__("daddiu $7, $7, 0");
__asm__("daddiu $6, $6, 0");
  GC_CAP struct struct1 * c = GC_malloc(600);
__asm__("daddiu $7, $7, 0");
__asm__("daddiu $6, $6, 0");
  GC_CAP struct struct1 * d = GC_malloc(600);
__asm__("daddiu $7, $7, 0");
__asm__("daddiu $6, $6, 0");
  GC_CAP struct struct1 * e = GC_malloc(600);
__asm__("daddiu $7, $7, 0");
__asm__("daddiu $6, $6, 0");
  GC_CAP struct struct1 * f = GC_malloc(600);
__asm__("daddiu $7, $7, 0");
__asm__("daddiu $6, $6, 0");
  GC_CAP struct struct1 * g = GC_malloc(600);
__asm__("daddiu $7, $7, 0");
__asm__("daddiu $6, $6, 0");
  GC_CAP struct struct1 * h = GC_malloc(600);
__asm__("daddiu $7, $7, 0");
__asm__("daddiu $6, $6, 0");
  GC_CAP struct struct1 * i = GC_malloc(600);
__asm__("daddiu $7, $7, 0");
__asm__("daddiu $6, $6, 0");
  GC_CAP struct struct1 * j = GC_malloc(600);
__asm__("daddiu $7, $7, 0");
__asm__("daddiu $6, $6, 0");
  GC_CAP struct struct1 * k = GC_malloc(600);
__asm__("daddiu $7, $7, 0");
  printf("a: 0x%llx\n", (GC_ULL) GC_cheri_getbase(a));
  printf("b: 0x%llx\n", (GC_ULL) GC_cheri_getbase(b));
  printf("c: 0x%llx\n", (GC_ULL) GC_cheri_getbase(c));
  printf("d: 0x%llx\n", (GC_ULL) GC_cheri_getbase(d));
  printf("e: 0x%llx\n", (GC_ULL) GC_cheri_getbase(e));
  printf("f: 0x%llx\n", (GC_ULL) GC_cheri_getbase(f));
  printf("g: 0x%llx\n", (GC_ULL) GC_cheri_getbase(g));
  printf("h: 0x%llx\n", (GC_ULL) GC_cheri_getbase(h));
  printf("i: 0x%llx\n", (GC_ULL) GC_cheri_getbase(i));
  printf("j: 0x%llx\n", (GC_ULL) GC_cheri_getbase(j));
  printf("k: 0x%llx\n", (GC_ULL) GC_cheri_getbase(k));
  #define GC_FILL(x) \
  ((struct struct1*)x)->ptr = GC_malloc(25); \
  printf("stack address &" #x ": 0x%llx\n", (GC_ULL) &x); \
  if (!strcmp(#x, "a")){ \
  GC_debug_track_allocated(x, "object " #x); \
  GC_debug_track_allocated(((struct struct1*)x)->ptr, "object " #x "'s ptr");} \
  memset((void*)((struct struct1*)x)->ptr, 0x33, 25);
  GC_FILL(a);
  GC_FILL(b);
  GC_FILL(c);
  GC_FILL(d);
  GC_FILL(e);
  GC_FILL(f);
  GC_FILL(g);
  GC_FILL(h);
  GC_FILL(i);
  GC_FILL(j);
  GC_FILL(k);
__asm__("daddiu $8, $8, 0");
  printf("(1) a->ptr: b=0x%llx\n", (GC_ULL) GC_cheri_getbase(((struct struct1*)a)->ptr));
__asm__("daddiu $9, $9, 0");
  GC_collect();
  printf("(2) a->ptr: b=0x%llx\n", (GC_ULL) GC_cheri_getbase(((struct struct1*)a)->ptr));
  void * stack_top = NULL;
  GC_GET_STACK_PTR(stack_top);
  printf("stack top: 0x%llx\n", (GC_ULL) stack_top);
  fill_test_helper();
  printf("fill_test_helper() returned\n");
  printf("(3) a->ptr: b=0x%llx\n", (GC_ULL) GC_cheri_getbase(((struct struct1*)a)->ptr));
  GC_collect();
  printf("(4) a->ptr: b=0x%llx\n", (GC_ULL) GC_cheri_getbase(((struct struct1*)a)->ptr));
  #define GC_OUTPUT(x) \
  printf("x: 0x%llx\n", (GC_ULL) GC_cheri_getbase(x)); \
  GC_assert(GC_IN(x, GC_state.thread_local_region.tospace)); \
  GC_PRINT_CAP(((struct struct1*)x)->ptr); \
  GC_debug_memdump( \
    GC_cheri_getbase(((struct struct1*)x)->ptr), \
    GC_cheri_getbase(((struct struct1*)x)->ptr)+GC_cheri_getlen(((struct struct1*)x)->ptr));
  GC_OUTPUT(a)
  GC_OUTPUT(b)
  GC_OUTPUT(c)
  GC_OUTPUT(d)
  GC_OUTPUT(e)
  GC_OUTPUT(f)
  GC_OUTPUT(g)
  GC_OUTPUT(h)
  GC_OUTPUT(i)
  GC_OUTPUT(j)
  GC_OUTPUT(k)
}

void
realloc_preserves_caps_test (void)
{
  //GC_init();
  struct struct1 * x = GC_low_malloc(1000);
  x->ptr = GC_cheri_ptr((void*) 0x1234, 0x5678);
  printf("x->ptr tag: %d\n", (int) GC_cheri_gettag(x->ptr));
  x = GC_low_realloc(x, 20000);
  printf("x->ptr tag: %d\n", (int) GC_cheri_gettag(x->ptr));
}

void
low_realloc_test (void)
{
  int sz = 10;
  void * ptr = GC_low_malloc(sz);
  memset(ptr, 0x88, sz);
  GC_debug_memdump(ptr, ptr+sz);
  ptr = GC_low_realloc(ptr, sz*2);
  GC_debug_memdump(ptr, ptr+sz*2);
}

void
remset_test (void)
{
#ifdef GC_GENERATIONAL
#if (GC_OY_STORE_DEFAULT == GC_OY_STORE_REMEMBERED_SET)
  //GC_init();
  GC_remembered_set_add(GC_state.thread_local_region.remset, (void *) 0x123400);
  GC_remembered_set_add(GC_state.thread_local_region.remset, (void *) 0x567800);
  GC_remembered_set_add(GC_state.thread_local_region.remset, (void *) 0x999900);
  GC_remembered_set_add(GC_state.thread_local_region.remset, (void *) 0x888800);
  GC_remembered_set_add(GC_state.thread_local_region.remset, (void *) 0x777700);
  size_t i;
  for (i=0; i<GC_state.thread_local_region.remset->nroots; i++)
  {
    printf("[%d] 0x%llx\n", (int) i, (GC_ULL) GC_state.thread_local_region.remset->roots[i]);
  }
  printf("size: %d\n", (int) GC_state.thread_local_region.remset->size);
#endif // GC_OY_STORE_DEFAULT
#endif // GC_GENERATIONAL
}

void
tracking_test (void)
{
  //GC_init();
  GC_CAP void * obj = GC_malloc(100);
  if (GC_debug_track_allocated(obj, "test object"))
  {
    printf("error: couldn't start tracking object\n");
  }
  collection_test();
}

void
grow_test (void)
{
  //GC_init();
  int i;
  for (i=0; i<10000; i++)
  {
    GC_cap_memset(GC_malloc(13), 0xAA);
    printf("%d\n", i);
  }
  printf("exiting\n");
  return;
}

void
rebase_test (void)
{
  GC_cap_ptr cap = GC_cheri_ptr((void*)0x1234, 0x5678);
  GC_cap_ptr cap3 = cap;
  GC_PRINT_CAP(cap);
  GC_cap_ptr cap2[100];
  int i;
  for (i=0; i<100; i++)
    cap2[i] = cap;
  
  void * old_base = (void*) 0x1000;
  size_t old_size = 0xABC;
  void * new_base = (void*) 0x9001;
  
  //GC_init();
  void * stack_top = NULL;
  GC_GET_STACK_PTR(stack_top);
  GC_assert(stack_top <= GC_state.stack_bottom);
  GC_rebase(stack_top, GC_state.stack_bottom,
            old_base, old_size, new_base);
  
  GC_PRINT_CAP(cap);
}

void
collection_test (void)
{
  #define NALLOC    100000      // 100,000 5k allocations = 500,000 k = 500 MB
  #define NSTORE    50          // 50*5k = 250k stored
  #define NBYTES    5
  #define LLSTORE   20         // the long-lived store
  #define LLBYTES   500
  int i;
  GC_cap_ptr arr_unaligned[NSTORE+1];
  GC_cap_ptr arrll_unaligned[LLSTORE+1];
  GC_cap_ptr * arr = &arr_unaligned[0];
  GC_ALIGN_32(arr, GC_cap_ptr *);
  GC_cap_ptr * arrll = &arrll_unaligned[0];
  GC_ALIGN_32(arrll, GC_cap_ptr *);
  //GC_init();
  time_t start = time(NULL);
    printf("arrll runs from 0x%llx to 0x%llx\n", (GC_ULL) arrll, (GC_ULL) &arrll[LLSTORE-1]);
    printf("arr runs from 0x%llx to 0x%llx\n", (GC_ULL) arr, (GC_ULL) &arr[NSTORE-1]);
  for (i=0; i<LLSTORE; i++)
  {
    arrll[i] = GC_malloc(LLBYTES);
    if (!(void*)arrll[i])
      {printf("ERROR: LL oom %d\n", i);break;}
  }
  for (i=0; i<NALLOC; i++)
  {
    arr[i%NSTORE] = GC_malloc(NBYTES);
    if (!(void*)arr[i%NSTORE])
      {printf("ERROR: oom %d\n", i);break;}
  }
  time_t end = time(NULL);
  GC_debug_print_region_stats(&GC_state.thread_local_region);
#ifdef GC_GENERATIONAL
  GC_debug_print_region_stats(&GC_state.old_generation);
#endif // GC_GENERATIONAL
  printf("total time: %d sec\n", (int) (end - start));
}

/*void
collection_test (void)
{
  int i;
  GC_init();
  
  GC_CHOOSE_OY(
    printf("The OY technique is GC_OY_MANUAL\n"),
    printf("The OY technique is GC_OY_EPHEMERAL\n")
  );
  
  __capability struct struct1 * old_object = GC_malloc(sizeof(struct struct1));
   
  // fill up the young generation right to the top
  __capability void * dummy;
  for (i=0; i<(GC_THREAD_LOCAL_HEAP_SIZE/sizeof(struct struct1))-1; i++)
    dummy = GC_malloc(sizeof(struct struct1));
  
  // you should confirm that the young generation is filled up
  GC_debug_print_region_stats(GC_state.thread_local_region);
  GC_debug_print_region_stats(GC_state.old_generation);
  
  // a young->young pointer
  GC_CHOOSE_OY(
    {GC_STORE_CAP(old_object->ptr, dummy);},  // GC_OY_MANUAL
    {old_object->ptr = dummy;}                // GC_OY_EPHEMERAL
  );
  GC_PRINT_CAP(old_object);
  GC_PRINT_CAP(old_object->ptr);
   
  // force old_object into the old generation
  dummy = GC_malloc(sizeof(struct struct1));
  
  // you should confirm that the object moved
  GC_debug_print_region_stats(GC_state.thread_local_region);
  GC_debug_print_region_stats(GC_state.old_generation);

  // a old->young pointer
  GC_CHOOSE_OY(
    {GC_STORE_CAP(old_object->ptr, dummy);},  // GC_OY_MANUAL
    {old_object->ptr = dummy;}                // GC_OY_EPHEMERAL
  );
  GC_PRINT_CAP(old_object);
  GC_PRINT_CAP(old_object->ptr);
  
}*/
