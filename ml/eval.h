#ifndef EVAL_H_HEADER
#define EVAL_H_HEADER

#include "parse.h"

#include <gc.h>

#include <gc_debug.h>
void
print_ast (GC_CAP expr_t * expr);

struct env_struct;

typedef struct
{
  num_t num;
} num_val_t;

typedef struct
{
  GC_CAP char * name;
  GC_CAP expr_t * body;
  GC_CAP struct env_struct * env;
} fn_val_t;

#define VAL_NUM 0
#define VAL_FN  1

typedef struct
{
  union
  {
    GC_CAP num_val_t * num_val;
    GC_CAP fn_val_t * fn_val;
  };
  int type;
} val_t;

typedef struct env_struct
{
  GC_CAP char * name;
  GC_CAP val_t * val;
  GC_CAP struct env_struct * next;
} env_t;

GC_CAP val_t *
lookup (GC_CAP char * name, GC_CAP env_t * env);

GC_CAP val_t *
eval (GC_CAP expr_t * expr, GC_CAP env_t * env);

void
print_val (GC_CAP val_t * val);

void
print_env (GC_CAP env_t * env);

#endif // EVAL_H_HEADER
