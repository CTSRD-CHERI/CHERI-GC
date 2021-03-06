#ifndef PARSE_H_HEADER
#define PARSE_H_HEADER

#include "lex.h"
#include "common.h"

typedef int num_t;

struct
{
  GC_CAP token_t * tok;
} parse_state;

struct expr_struct;

// WARNING! Pointers must all be 32-byte aligned!
typedef struct
{
  GC_CAP struct expr_struct * cond, * true_expr, * false_expr;
} if_expr_t;

typedef struct
{
  GC_CAP char * name;
} name_expr_t;

typedef struct
{
  num_t num;
} num_expr_t;

typedef struct
{
  GC_CAP struct expr_struct * a, * b;
  GC_CAP char * op; // op = "" means function application expression
} op_expr_t;

typedef struct
{
  GC_CAP char * name;
  GC_CAP struct expr_struct * body;
} fn_expr_t;

/*typedef struct
{
  GC_CAP struct expr_struct * func, * arg;
} app_expr_t;*/

#define EXPR_IF   0
#define EXPR_NAME 1
#define EXPR_NUM  2
#define EXPR_OP   3
#define EXPR_FN   4
//#define EXPR_APP  5

// Warning! GC_CAP pointers inside a struct must be 32-byte aligned!
typedef struct expr_struct
{
  union
  {
    GC_CAP if_expr_t * if_expr;
    GC_CAP name_expr_t * name_expr;
    GC_CAP num_expr_t * num_expr;
    GC_CAP op_expr_t * op_expr;
    GC_CAP fn_expr_t * fn_expr;
    //GC_CAP app_expr_t * app_expr;
  };
  int type;
} expr_t;

GC_USER_FUNC int
parser_is_at_start_of_expression (void);

// Assumes you've called lex_read_file().
GC_USER_FUNC void
parse_init (void);

// Assumes you've called parse_init().
#define parse() parse2(__FILE__, __LINE__)
GC_USER_FUNC GC_CAP expr_t *
parse2 (const char * file, int line);

GC_USER_FUNC GC_CAP expr_t *
parse_cons (void);

GC_USER_FUNC GC_CAP expr_t *
parse_greater_than (void);

GC_USER_FUNC GC_CAP expr_t *
parse_less_than (void);

GC_USER_FUNC GC_CAP expr_t *
parse_add (void);

GC_USER_FUNC GC_CAP expr_t *
parse_sub (void);

GC_USER_FUNC GC_CAP expr_t *
parse_mul (void);

GC_USER_FUNC GC_CAP expr_t *
parse_div (void);

GC_USER_FUNC GC_CAP expr_t *
parse_app (void);

// Clang/LLVM doesn't like function pointers when GC_USER_FUNC is __attribute__((sensitive))

#define parse_op(op,func) \
  parse_op__##func(op)

#define PARSE_OP_DECLARE(op,func) \
  GC_USER_FUNC GC_CAP expr_t * \
  parse_op__##func(GC_CAP const char * op);
  
#define X_LIST \
  X_MACRO(op, parse_greater_than) \
  X_MACRO(op, parse_less_than) \
  X_MACRO(op, parse_add) \
  X_MACRO(op, parse_sub) \
  X_MACRO(op, parse_mul) \
  X_MACRO(op, parse_div) \
  X_MACRO(op, parse_app) \
  X_MACRO(op, parse_base_expr) \
  
#define X_MACRO PARSE_OP_DECLARE
  X_LIST
#undef X_MACRO


/*GC_USER_FUNC GC_CAP expr_t *
parse_op (GC_CAP const char * op,
          GC_USER_FUNC GC_CAP expr_t * (*lower_precedence_func)(void));*/

GC_USER_FUNC GC_CAP expr_t *
parse_base_expr (void);

GC_USER_FUNC GC_CAP if_expr_t *
parse_if (void);

GC_USER_FUNC GC_CAP fn_expr_t *
parse_fn (void);

GC_USER_FUNC GC_CAP num_expr_t *
parse_num (void);

GC_USER_FUNC GC_CAP name_expr_t *
parse_name (void);

#define parse_get_next_tok() parse_get_next_tok2(__FILE__,__LINE__)
GC_USER_FUNC void
parse_get_next_tok2 (const char * file, int line);

GC_USER_FUNC int
parse_tok_eq (int type, GC_CAP const char * str);

GC_USER_FUNC void
parse_eof_error (void);

GC_USER_FUNC void
parse_unexpected (void);

GC_USER_FUNC void
parse_expect (int type, GC_CAP const char * str);

#endif // PARSE_H_HEADER
