#include "lex.h"

#include <gc.h>

#include <stdio.h>

int main ()
{
  GC_init();
  
  GC_CAP const char * filename = GC_cheri_ptr("ml-tmp", sizeof("ml-tmp"));
  
  lex_read_file(filename);
  
  size_t i;
  for (i=0; i<lex_state.max; i++)
  {
    putchar(((char*)lex_state.file)[i]);
  }
  
  token_t t;
  t = lex();
  while (t.type != TKEOF)
  {
    printf("[%d] %s\n", t.type, t.str);
  }
  
  return 0;
}