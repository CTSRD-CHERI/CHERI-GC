#ifndef LEX_H_HEADER
#define LEX_H_HEADER

#include <gc.h>

#define TKEOF   0
#define TKINT   1
#define TKWORD  2
#define TKSYM   3

typedef struct
{
  int type;
  GC_CAP char * str;
} token_t;

struct
{
  GC_CAP char * file;
  size_t index;
  size_t max;
} lex_state;

void
lex_read_file (GC_CAP const char * name);

#endif // LEX_H_HEADER
