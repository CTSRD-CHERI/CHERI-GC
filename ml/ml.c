#include "lex.h"
#include "parse.h"
#include "eval.h"
#include "ml_time.h"

#include "common.h"

#include <stdio.h>

#ifdef GC_BOEHM
void
__LOCK_MALLOC (void)
{
}
void
__UNLOCK_MALLOC (void)
{
}
#endif // GC_BOEHM

#ifdef GC_NONE
#include <stdlib.h>
GC_USER_FUNC GC_CAP void *
ml_no_gc_malloc (size_t sz)
{
  void * p = malloc(sz+32);
  if (p)
    return GC_cheri_ptr(ML_ALIGN_32(p), sz);
  else
    return GC_INVALID_PTR();
}
#endif // GC_NONE

GC_USER_FUNC void
ml_print_gc_stats (void);

GC_USER_FUNC void
ml_print_plot_data (void);

#include <string.h>
GC_USER_FUNC GC_CAP void *
cmemset (GC_CAP void * ptr, int value, size_t num)
{
  memset((void*)ptr, value, num);
  return ptr;
}
GC_USER_FUNC GC_CAP void *
cmemcpy (GC_CAP void * dest, GC_CAP const void * source, size_t num)
{
  memcpy((void*)dest, (const void*)source, num);
  return dest;
}
GC_USER_FUNC int
cstrcmp (GC_CAP const char * str1, GC_CAP const char * str2)
{
  return strcmp((const char*)str1, (const char*)str2);
}
GC_USER_FUNC size_t
cstrlen (GC_CAP const char * str)
{
  return strlen((const char*)str);
}


/* What we require from the GC:

  GC_init
  GC_malloc
  GC_INVALID_PTR
  GC_PTR_VALID
  GC_CAP
  GC_cheri_ptr
  GC_cheri_getlen
  GC_STORE_CAP
*/


/*
  ML syntax:
  
  E ::= x | n | if E then E else E | fn x . E | E E | E op E | (E)

  (E is an expression)
  (x is a variable name, consisting only of letters)
  (n is an integer)
  (op is +, -, *, /, < or >)
  
  The interpreter is call-by-value.
  
  Evaluation results in either a number or a function value. Unbound variables
  will be reported.

*/

GC_USER_FUNC int main (int argc, char ** argv)
{
#ifdef MEMWATCH
  mwInit();
  mwDoFlush(1);
#endif // MEMWATCH
  
  ML_START_TIMING(main_time);
  
  GC_init();
  
  //ml_print_gc_stats();
  
  printf("Compiled for "
#if   defined(GC_CHERI)
  "GC_CHERI"
#elif defined(GC_BOEHM)
  "GC_BOEHM"
#elif defined(GC_NONE)
  "GC_NONE"
#else
#error "Define one of GC_CHERI, GC_BOEHM, GC_NONE."
#endif // GC_CHERI, GC_BOEHM, GC_NONE
  " at %s\n", __TIME__  " " __DATE__);
  
  //GC_CAP const char * filename = GC_cheri_ptr("ml-tmp", sizeof("ml-tmp"));
  
  //lex_read_file(filename);
  
  //const char str[] = "(fn x . ((fn x . x) 3) + x) 2";
  //const char str[] = "fn f . (fn g. (f (fn a . (g g) a))) (fn g. (f (fn a . (g g) a)))";
  
  //const char str[] =
    //"((fn f . fn n . if n then n * f (n-1) else 1) (fn n . n)) 5";
  
  // factorial:
  //const char str[] =
  //  "((fn f . (fn g. (f (fn a . (g g) a))) (fn g. (f (fn a . (g g) a)))) (fn f . fn n . if n then n * f (n-1) else 1)) 6";

  // for the benchmark:
  if (argc < 2)
  {
    printf("Need a number argument\n");
  }
  const char str[] =
    "((fn f . (fn g. (f (fn a . (g g) a))) (fn g. (f (fn f . (g g) f)))) (fn f . fn n . if n then n + f (n-1) else 1)) ";
  GC_CAP char * str2 = ml_malloc(sizeof(str)+strlen(argv[1]));
  cmemcpy(str2+sizeof(str)-1, GC_cheri_ptr(argv[1], strlen(argv[1]+1)), strlen(argv[1]+1));
  
  
  
  lex_read_string(str2);
  printf("program: %s\n\n", (void*)(str2));
  
  /*size_t i;
  for (i=0; i<lex_state.max; i++)
  {
    putchar(((char*)lex_state.file)[i]);
  }
  
  GC_CAP token_t * t;
  t = lex();
  while (t->type != TKEOF)
  {
    printf("[%d] (tag=%d alloc=%d) %s\n", ((token_t*)t)->type, (int) GC_cheri_gettag(((token_t*)t)->str), (int) GC_IS_GC_ALLOCATED(((token_t*)t)->str), (char*) ((token_t*)t)->str);
    GC_malloc(5000);
    t = lex();
  }
  printf("Finished\n");
  return 0;*/
  
  parse_init();
  
  GC_CAP expr_t * expr = GC_INVALID_PTR();
  GC_STORE_CAP(expr, parse());
  
  printf("AST:\n");
  print_ast(expr);
  printf("\nDone printing AST\n");
  
  /*printf("collecting loads\n");
  ml_collect();
  ml_collect();
  ml_collect();
  ml_collect();
  ml_collect();
  ml_collect();
  ml_collect();
  ml_collect();
  ml_collect();
  ml_collect();
  ml_collect();
  ml_collect();
  ml_collect();
  ml_collect();
  ml_collect();
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~done collecting loads\n");
  GC_debug_print_region_stats(&GC_state.thread_local_region);*/
  
  GC_CAP val_t * val = GC_INVALID_PTR();
  GC_STORE_CAP(val, eval(expr, GC_INVALID_PTR()));
  
  printf("eval: ");
  if (!PTR_VALID(val))
    printf("(invalid");
  else
    print_val(val);
  printf("\n\n");

  ML_STOP_TIMING(main_time, "main()");
  
  ml_print_gc_stats();
  
  ml_print_plot_data();
  
#ifdef MEMWATCH
  mwTerm();
#endif // MEMWATCH
  return 0;
}

GC_USER_FUNC void
ml_print_gc_stats (void)
{
#if defined(GC_CHERI)
  GC_debug_print_region_stats(&GC_state.thread_local_region);
#ifdef GC_GENERATIONAL
  GC_debug_print_region_stats(&GC_state.old_generation);
#endif // GC_GENERATIONAL
#elif defined(GC_BOEHM)
  printf("Boehm heap size: %llu\n", (unsigned long long) GC_get_heap_size());
#endif // GC selector
}

GC_USER_FUNC void
ml_print_plot_data (void)
{
#if defined(GC_CHERI)
  unsigned long long
    heapsyinit = GC_THREAD_LOCAL_HEAP_SIZE,
    heapsycurr = GC_cheri_getlen(GC_state.thread_local_region.tospace),
    heapsymaxb = GC_state.thread_local_region.max_grow_size_before_collection,
    heapsymaxa = GC_state.thread_local_region.max_grow_size_after_collection;
  printf("[plotdata] # heap size: (young generation) I=%llu B=%llu A=%llu F=%llu\n",
    heapsyinit, heapsymaxb, heapsymaxa, heapsycurr);
  
#ifdef GC_GENERATIONAL
  unsigned long long
    heapsoinit = GC_OLD_GENERATION_SEMISPACE_SIZE,
    heapsocurr = GC_cheri_getlen(GC_state.old_generation.tospace),
    heapsomaxb = GC_state.old_generation.max_grow_size_before_collection,
    heapsomaxa = GC_state.old_generation.max_grow_size_after_collection;
  printf("[plotdata] # heap size: (old generation) I=%llu B=%llu A=%llu F=%llu\n",
    heapsoinit, heapsomaxb, heapsomaxa, heapsocurr);
#endif // GC_GENERATIONAL
#elif defined(GC_BOEHM)
  printf("[plotdata] # Boehm heap size: %llu\n",
    (unsigned long long) GC_get_heap_size());
#endif
}
