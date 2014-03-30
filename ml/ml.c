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

int main ()
{
  ML_START_TIMING(main_time);
  
  GC_init();
  
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
  const char str[] =
    "((fn f . (fn g. (f (fn a . (g g) a))) (fn g. (f (fn a . (g g) a)))) (fn f . fn n . if n then n * f (n-1) else 1)) 6";
    
  lex_read_string(GC_cheri_ptr((void *) str, sizeof(str)));
  printf("program: %s\n\n", str);
  
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
  
  GC_CAP expr_t * expr = GC_INVALID_PTR;
  GC_STORE_CAP(expr, parse());
  
  printf("AST:\n");
  print_ast(expr);
  printf("\n\n");

  GC_CAP val_t * val = GC_INVALID_PTR;
  GC_STORE_CAP(val, eval(expr, GC_INVALID_PTR));
  
  printf("eval: ");
  print_val(val);
  printf("\n\n");

//  GC_debug_print_region_stats(&GC_state.thread_local_region);
#ifdef GC_GENERATIONAL
//  GC_debug_print_region_stats(&GC_state.old_generation);
#endif // GC_GENERATIONAL

  ML_STOP_TIMING(main_time, "main()");
  
  return 0;
}
