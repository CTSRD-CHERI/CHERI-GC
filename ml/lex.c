#include "lex.h"

#include <gc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
lex_read_file (GC_CAP const char * name)
{
  FILE * file = fopen((const char *) name, "rb");
  if (!file)
  {
    fprintf(stderr, "could not open file %s\n", (const char *) name);
    exit(1);
  }
  
  // A very inefficient way of reading a file, designed to test the collector.
  lex_state.max = 0;
  lex_state.file = GC_INVALID_PTR;
  lex_state.index = 0;
  char c;
  while (fread(&c, 1, 1, file) == 1)
  {
    lex_state.max++;
    GC_CAP char * tmp = GC_malloc(lex_state.max);
    if (!(char *) tmp)
    {
      fprintf(stderr, "out of memory reading file %s\n", (const char *) name);
      exit(1);
    }
    if (lex_state.file)
    {
      memcpy((char *) tmp, (char *) lex_state.file, lex_state.max-1);
    }
    lex_state.file[lex_state.max-1] = c;
  }

  fclose(file);
}
