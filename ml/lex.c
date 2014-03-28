#include "lex.h"

#include <gc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GC_CAP char *
copy_string (GC_CAP const char * str)
{
  GC_CAP char * copy = GC_malloc(GC_cheri_getlen(str));
  if (!PTR_VALID(copy))
  {
    fprintf(stderr, "copy_string(): out of memory\n");
    exit(1);
  }
  memcpy((char*)copy, (const char*)str, GC_cheri_getlen(str));
  return copy;
}

void
lex_read_file (GC_CAP const char * name)
{
  FILE * file = fopen((const char *) name, "rb");
  if (!file)
  {
    fprintf(stderr, "could not open file %s\n", (const char *) name);
    exit(1);
  }
  
  // A very inefficient way of reading a file, designed to stress the collector.
  lex_state.max = 0;
  lex_state.file = GC_INVALID_PTR;
  lex_state.index = 0;
  char c;
  while (fread(&c, 1, 1, file) == 1)
  {
    lex_state.max++;
    GC_CAP char * tmp = GC_malloc(lex_state.max);
    if (!PTR_VALID(tmp))
    {
      fprintf(stderr, "out of memory reading file %s\n", (const char *) name);
      exit(1);
    }
    memset((char*) tmp, 0, lex_state.max);
    if (PTR_VALID(lex_state.file))
    {
      memcpy((char *) tmp, (char *) lex_state.file, lex_state.max-1);
    }
    lex_state.file = tmp;
    ((char*)lex_state.file)[lex_state.max-1] = c;
  }

  fclose(file);
}

#include <gc_debug.h>
void
lex_read_string (GC_CAP const char * str)
{
  lex_state.file = copy_string(str);
  if (!PTR_VALID(lex_state.file))
  {
    fprintf(stderr, "lex_read_string: out of memory\n");
    exit(1);
  }
  GC_debug_track_allocated(lex_state.file, "lex_state file");
  lex_state.index = 0;
  lex_state.max = GC_cheri_getlen(str)-1;
}

#define LEX_IS_SYM(c) ( \
  ((c) == '<') || ((c) == '>') || ((c) == '+') || ((c) == '-') || ((c) == '*') \
  || ((c) == '/') || ((c) == '(') || ((c) == ')') || ((c) == '=') \
  || ((c) == ':') || ((c) == '|') || ((c) == ';') || ((c) == '.') \
)

#define LEX_IS_LETTER(c) ( \
  ( ((c) >= 'A') && ((c) <= 'Z') ) || ( ((c) >= 'a') && ((c) <= 'z') ) \
)

#define LEX_IS_DIGIT(c) ( ((c) >= '0') && ((c) <= '9') )

#define LEX_IS_WSPC(c) ( \
  ((c) == ' ') || ((c) == '\t') || ((c) == '\n') || ((c) == '\r') \
)

token_t
lex (void)
{
  token_t t;
  t.type = TKEOF;
  t.nearby_character = lex_state.index;
  
#define LEX_APPEND_STR(c) \
  do { \
    t.len++; \
    GC_CAP char * tmp = GC_malloc(t.len); \
    if (!PTR_VALID(tmp)) \
    { \
      fprintf(stderr, "out of memory lexing string\n"); \
      exit(1); \
    } \
    memcpy((char *) tmp, (char *) t.str, t.len-1); \
    t.str = tmp; \
    ((char*)t.str)[t.len-1] = (c); \
  } while (0)

#define LEX_INIT_STR(c) \
  do { \
    t.len = 1; \
    t.str = GC_malloc(1); \
    if (!PTR_VALID(t.str)) \
    { \
      fprintf(stderr, "out of memory lexing string\n"); \
      exit(1); \
    } \
    ((char*)t.str)[0] = (c); \
  } while (0)

  int state = 0;
  while (lex_state.index < lex_state.max)
  {
    char c = ((char*)lex_state.file)[lex_state.index];
    switch (state)
    {
      case 0:
      {
        if (LEX_IS_SYM(c))
        {
          t.type = TKSYM;
          LEX_INIT_STR(c);
          lex_state.index++;
          state = 1;
        }
        else if (LEX_IS_LETTER(c))
        {
          t.type = TKWORD;
          LEX_INIT_STR(c);
          lex_state.index++;
          state = 2;
        }
        else if (LEX_IS_DIGIT(c))
        {
          t.type = TKINT;
          LEX_INIT_STR(c);
          lex_state.index++;
          state = 3;
        }
        else if (LEX_IS_WSPC(c))
        {
          lex_state.index++;
        }
        else
        {
          fprintf(stderr, "lex error: unrecognized character: "
            "`%c' (%d, index %d, max %d)\n",
            c, (int) c, (int) lex_state.index, (int) lex_state.max);
          exit(1);
        }
        break;
      }
      case 1:
      {
        if (c == '>' || c == ':')
        {
          LEX_APPEND_STR(c);
          lex_state.index++;
        }
        else state = 5;
        break;
      }
      case 2:
      {
        if (LEX_IS_LETTER(c))
        {
          LEX_APPEND_STR(c);
          lex_state.index++;
        }
        else state = 5;
        break;
      }
      case 3:
      {
        if (LEX_IS_DIGIT(c))
        {
          LEX_APPEND_STR(c);
          lex_state.index++;
        }
        else state = 5;
        break;
      }
    }
    if (state == 5) break;
  }
  
  if (t.type != TKEOF)
  {
    LEX_APPEND_STR('\0');
  }
  return t;
}
