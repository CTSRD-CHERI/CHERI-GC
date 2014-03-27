#ifndef LEX_H_HEADER
#define LEX_H_HEADER

#include <gc.h>
#define PTR_VALID   GC_PTR_VALID

#define TKEOF   0
#define TKINT   1
#define TKWORD  2
#define TKSYM   3

typedef struct
{
  GC_CAP char * str;
  int type;
  size_t len;
} token_t;

struct
{
  GC_CAP char * file;
  size_t index;
  size_t max;
} lex_state;

void
lex_read_file (GC_CAP const char * name);

token_t
lex (void);

#endif // LEX_H_HEADER
