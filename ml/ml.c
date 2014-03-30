#include "lex.h"
#include "parse.h"
#include "eval.h"

#include <gc.h>

#include <stdio.h>


/* What we require from the GC:

  GC_malloc
  GC_INVALID_PTR
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

#include <gc_init.h>
#include <gc_debug.h>
int main ()
{
  GC_init();
  
  //GC_CAP const char * filename = GC_cheri_ptr("ml-tmp", sizeof("ml-tmp"));
  
  //lex_read_file(filename);
  
  //const char str[] = "(fn x . ((fn x . x) 3) + x) 2";
  //const char str[] = "fn f . (fn g. (f (fn a . (g g) a))) (fn g. (f (fn a . (g g) a)))";
  
  //const char str[] =
    //"((fn f . fn n . if n then n * f (n-1) else 1) (fn n . n)) 5";
  
  // factorial:
  const char str[] =
    "((fn f . (fn g. (f (fn a . (g g) a))) (fn g. (f (fn a . (g g) a)))) (fn f . fn n . if n then n * f (n-1) else 1)) 20";
    
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

  GC_debug_print_region_stats(&GC_state.thread_local_region);
#ifdef GC_GENERATIONAL
  GC_debug_print_region_stats(&GC_state.old_generation);
#endif // GC_GENERATIONAL

  return 0;
}
