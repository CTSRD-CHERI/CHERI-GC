#include "eval.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
print_val (GC_CAP val_t * val)
{
  if (!PTR_VALID(val))
  {
    printf("(invalid value)");
    return;
  }
  switch (((val_t*)val)->type)
  {
    case VAL_NUM:
    {
      printf("num(%d)", ((num_val_t *) (((val_t *) val)->num_val))->num);
      break;
    }
    case VAL_FN:
    {
      printf("fn(%s,", (char*) ((fn_val_t *) (((val_t *) val)->fn_val))->name);
      print_ast(((fn_val_t *) (((val_t *) val)->fn_val))->body);
      printf(",");
      print_env(((fn_val_t *) (((val_t *) val)->fn_val))->env);
      printf(")");
    }
  }
}

void
print_env (GC_CAP env_t * env)
{
  printf("env[");
  while ((env_t*) env)
  {
    printf("%s := ", (char *) ((env_t*) env)->name);
    print_val(((env_t *) env)->val);
    env = ((env_t *) env)->next;
    if ((env_t*) env) printf(",");
  }
  printf("]");
}

void
print_ast (GC_CAP expr_t * expr)
{
  if (!PTR_VALID(expr))
  {
    printf("(invalid expression)");
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

GC_CAP val_t *
lookup (GC_CAP char * name, GC_CAP env_t * env)
{
  while ((env_t*) env)
  {
    if (!strcmp((char *) name, (char *) ((env_t*) env)->name))
    {
      return ((env_t *) env)->val;
    }
    env = ((env_t *) env)->next;
  }
  fprintf(stderr, "unbound variable: %s\n", (char*) name);
  exit(1);
  return GC_INVALID_PTR; // should never reach here
}

GC_CAP val_t *
eval (GC_CAP expr_t * expr, GC_CAP env_t * env)
{
  if (!PTR_VALID(expr))
  {
    printf("eval: invalid expression\n");
    return GC_INVALID_PTR;
  }
  switch (((expr_t*)expr)->type)
  {
    case EXPR_IF:
    {      
      GC_CAP val_t * cond = 
        eval((((if_expr_t*) ((expr_t*)expr)->if_expr))->cond, env);
      if (!PTR_VALID(cond))
        return GC_INVALID_PTR;
      if ( ((val_t *) cond)->type != VAL_NUM )
      {
        fprintf(stderr, "eval: type error: if condition not a number\n");
        exit(1);
      }
      
      if ( ((num_val_t *) (((val_t *) cond)->num_val))->num )
      {
        return eval((((if_expr_t*) ((expr_t*)expr)->if_expr))->true_expr, env);
      }
      else
      {
        return eval((((if_expr_t*) ((expr_t*)expr)->if_expr))->false_expr, env);
      }
    }
    case EXPR_NAME:
    {
      return lookup((((name_expr_t*) ((expr_t*)expr)->name_expr))->name, env);
    }
    case EXPR_NUM:
    {
      GC_CAP val_t * val = GC_malloc(sizeof(val_t));
      if (!PTR_VALID(val))
      {
        fprintf(stderr, "eval: out of memory allocating val_t");
      }
      ((val_t*) val)->type = VAL_NUM;
      ((val_t*) val)->num_val = GC_malloc(sizeof(num_val_t));
      if (!PTR_VALID(((val_t*) val)->num_val))
      {
        fprintf(stderr, "eval: out of memory allocating num_val_t");
      }
      ((num_val_t *) (((val_t*) val)->num_val))->num =
        (((num_expr_t*) ((expr_t*)expr)->num_expr))->num;
      return val;
    }
    case EXPR_OP:
    {
      
      // No short-circuiting for operators and no call-by-name for now.
      // Can emulate call-by-name by passing a function by value instead if need
      // be..
      GC_CAP val_t * a =
        eval( (((op_expr_t*) ((expr_t*)expr)->op_expr))->a, env );
      GC_CAP val_t * b =
        eval( (((op_expr_t*) ((expr_t*)expr)->op_expr))->b, env );
      
      if (GC_cheri_getlen((((op_expr_t*) ((expr_t*)expr)->op_expr))->op) > 1)
      {
        if ( ((val_t *) a)->type != VAL_NUM )
        {
          fprintf(stderr, "eval: type error: operator argument not a number\n");
          exit(1);
        }
        if ( ((val_t *) b)->type != VAL_NUM )
        {
          fprintf(stderr, "eval: type error: operator argument not a number\n");
          exit(1);
        }
        
        num_t a_num = ((num_val_t *) (((val_t *) a)->num_val))->num;
        num_t b_num = ((num_val_t *) (((val_t *) b)->num_val))->num;
        num_t result = 0;

        GC_CAP val_t * val = GC_malloc(sizeof(val_t));
        if (!PTR_VALID(val))
        {
          fprintf(stderr, "eval: out of memory allocating val_t");
        }
        ((val_t*) val)->type = VAL_NUM;
        ((val_t*) val)->num_val = GC_malloc(sizeof(num_val_t));
        if (!PTR_VALID(((val_t*) val)->num_val))
        {
          fprintf(stderr, "eval: out of memory allocating num_val_t");
        }
        
        char op = ((char*) ((((op_expr_t*) ((expr_t*)expr)->op_expr))->op))[0];
        switch (op)
        {
          case ':':
          {
            printf("warning: unhandled: cons operator\n");
            break;
          }
          case '<':
          {
            result = a_num < b_num;
            break;
          }
          case '>':
          {
            result = a_num > b_num;
            break;
          }
          case '+':
          {
            result = a_num + b_num;
            break;
          }
          case '-':
          {
            result = a_num - b_num;
            break;
          }
          case '*':
          {
            result = a_num * b_num;
            break;
          }
          case '/':
          {
            if (b_num != 0) result = a_num / b_num;
            break;
          }
        }
        
        ((num_val_t *) (((val_t*) val)->num_val))->num = result;
        return val;
      }
      else
      {
        // function application
        if ( ((val_t *) a)->type != VAL_FN )
        {
          fprintf(stderr, "eval: type error: not a function\n");
          exit(1);
        }
        
        // add new environment entry to the function value's saved environment
        GC_CAP env_t * new_env = GC_malloc(sizeof(env_t));
        if (!PTR_VALID(new_env))
        {
          fprintf(stderr, "eval: out of memory allocating env_t\n");
          exit(1);
        }
        ((env_t *) new_env)->name = 
          ((fn_val_t *) (((val_t *) a)->fn_val))->name;
        ((env_t *) new_env)->val = b;
        ((env_t *) new_env)->next =
          ((fn_val_t *) (((val_t *) a)->fn_val))->env;
        return eval(((fn_val_t *) (((val_t *) a)->fn_val))->body, new_env);
      }
      return GC_INVALID_PTR; // should never reach here
    }
    case EXPR_FN:
    {
      GC_CAP val_t * val = GC_malloc(sizeof(val_t));
      if (!PTR_VALID(val))
      {
        fprintf(stderr, "eval: out of memory allocating val_t");
      }
      ((val_t*) val)->type = VAL_FN;
      ((val_t*) val)->fn_val = GC_malloc(sizeof(fn_val_t));
      if (!PTR_VALID(((val_t*) val)->fn_val))
      {
        fprintf(stderr, "eval: out of memory allocating fn_val_t");
      }
      ((fn_val_t *) (((val_t*) val)->fn_val))->name =
        (((fn_expr_t*) ((expr_t*)expr)->fn_expr))->name;
      ((fn_val_t *) (((val_t*) val)->fn_val))->body =
        (((fn_expr_t*) ((expr_t*)expr)->fn_expr))->body;
      ((fn_val_t *) (((val_t*) val)->fn_val))->env = env;
      return val;
    }
    default:
    {
      printf("unknown expression type %d\n", ((expr_t*)expr)->type);
      return GC_INVALID_PTR;
    }
  }
}