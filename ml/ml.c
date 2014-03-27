#include "lex.h"
#include "parse.h"
#include "eval.h"

#include <gc.h>

#include <stdio.h>

int main ()
{
  GC_init();
  
  GC_CAP const char * filename = GC_cheri_ptr("ml-tmp", sizeof("ml-tmp"));
  
  lex_read_file(filename);
  
  /*size_t i;
  for (i=0; i<lex_state.max; i++)
  {
    putchar(((char*)lex_state.file)[i]);
  }
  
  token_t t;
  t = lex();
  while (t.type != TKEOF)
  {
    printf("[%d] %s\n", t.type, (char*) t.str);
    t = lex();
  }*/
  
  parse_init();
  
  GC_CAP expr_t * expr = parse();
  
  printf("Got expression of type %d\n", ((expr_t*)expr)->type);
  
  print_ast(expr);
  printf("\n");

  GC_CAP val_t * val = eval(expr, GC_INVALID_PTR);
  
  print_val(val);
  printf("\n");

  return 0;
}
