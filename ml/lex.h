#ifndef LEX_H_HEADER
#define LEX_H_HEADER

#include "common.h"
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
  size_t nearby_character;
  size_t token_number;
} token_t;

struct
{
  GC_CAP char * file;
  size_t index;
  size_t max;
  size_t num_tokens;
} lex_state;

GC_USER_FUNC GC_CAP char *
copy_string (GC_CAP const char * str);

GC_USER_FUNC void
lex_read_file (GC_CAP const char * name);

GC_USER_FUNC void
lex_read_string (GC_CAP const char * str);

GC_USER_FUNC GC_CAP token_t *
lex (void);

// buf must have length at least 11; this is not checked for (we can't rely on
// GC_cheri_getlen being present when compiling for no gc or Boehm)
GC_USER_FUNC void
ml_itoa (unsigned num, GC_CAP char * buf);

#endif // LEX_H_HEADER
