#include "eval.h"

#include <stdio.h>

void
print_ast (GC_CAP expr_t * expr)
{
  if (!PTR_VALID(expr))
  {
    printf("print_ast: invalid expression\n");
    return;
  }
  switch (((expr_t*)expr)->type)
  {
    case EXPR_IF:
    {
      printf("if(");
      print_ast( (((if_expr_t*) ((expr_t*)expr)->if_expr))->cond );
      printf(",");
      print_ast( (((if_expr_t*) ((expr_t*)expr)->if_expr))->true_expr );
      printf(",");
      print_ast( (((if_expr_t*) ((expr_t*)expr)->if_expr))->false_expr );
      printf(")");
      break;
    }
    case EXPR_NAME:
    {
      printf("name(%s)",
        (char*) (((name_expr_t*) ((expr_t*)expr)->name_expr))->name);
      break;
    }
    case EXPR_NUM:
    {
      printf("num(%d)", (((num_expr_t*) ((expr_t*)expr)->num_expr))->num);
      break;
    }
    case EXPR_OP:
    {
      if (GC_cheri_getlen((((op_expr_t*) ((expr_t*)expr)->op_expr))->op) > 1)
      {
        printf("op(%s,",
          (char *) (((op_expr_t*) ((expr_t*)expr)->op_expr))->op);
      }
      else
      {
        printf("app(");
      }
      print_ast( (((op_expr_t*) ((expr_t*)expr)->op_expr))->a );
      printf(",");
      print_ast( (((op_expr_t*) ((expr_t*)expr)->op_expr))->b );
      printf(")");
      break;
    }
    case EXPR_FN:
    {
      printf("fn(%s,", (char*) (((fn_expr_t*) ((expr_t*)expr)->fn_expr))->name);
      print_ast( (((fn_expr_t*) ((expr_t*)expr)->fn_expr))->body );
      printf(")");
      break;
    }
    default:
    {
      printf("unknown expression type %d\n", ((expr_t*)expr)->type);
    }
  }
}
