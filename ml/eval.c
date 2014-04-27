#include "eval.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GC_USER_FUNC void
print_val (GC_CAP val_t * val)
{
  if (!PTR_VALID(val))
  {
    printf("(invalid value)");
    return;
  }
  switch (val->type)
  {
    case VAL_NUM:
    {
      printf("num(%d)", val->num_val->num);
      break;
    }
    case VAL_FN:
    {
      printf("fn(%s,", (char *) val->fn_val->name);
      print_ast(val->fn_val->body);
      printf(",");
      print_env(val->fn_val->env);
      printf(")");
    }
  }
}

GC_USER_FUNC void
print_env (GC_CAP env_t * env)
{
  printf("env[");
  while (PTR_VALID(env))
  {
    printf("%s := ", (char *) env->name);
    print_val(env->val);
    GC_STORE_CAP(env, env->next);
    if (PTR_VALID(env)) printf(",");
  }
  printf("]");
}

GC_USER_FUNC void
print_ast (GC_CAP expr_t * expr)
{
  if (!PTR_VALID(expr))
  {
    printf("(invalid expression)");
    return;
  }
  switch (expr->type)
  {
    case EXPR_IF:
    {
      if (!PTR_VALID(expr->if_expr))
      {
        fprintf(stderr, "Invalid if_expr\n");
        exit(1);
      }
      printf("if(");
      print_ast(expr->if_expr->cond);
      printf(",");
      print_ast(expr->if_expr->true_expr);
      printf(",");
      print_ast(expr->if_expr->false_expr);
      printf(")");
      break;
    }
    case EXPR_NAME:
    {
      if (!PTR_VALID(expr->name_expr))
      {
        fprintf(stderr, "Invalid name_expr\n");
        exit(1);
      }
      if (!PTR_VALID(expr->name_expr->name))
      {
        fprintf(stderr, "Invalid name_expr->name\n");
        exit(1);
      }
      printf("name(%s)", (char*) expr->name_expr->name);
      break;
    }
    case EXPR_NUM:
    {
      if (!PTR_VALID(expr->num_expr))
      {
        fprintf(stderr, "Invalid num_expr\n");
        exit(1);
      }
      printf("num(%d)", expr->num_expr->num);
      break;
    }
    case EXPR_OP:
    {
      if (!PTR_VALID(expr->op_expr))
      {
        fprintf(stderr, "Invalid op_expr\n");
        exit(1);
      }
      if (!PTR_VALID(expr->op_expr->op))
      {
        fprintf(stderr, "Invalid op_expr->op\n");
        exit(1);
      }
      size_t oplen = cstrlen(expr->op_expr->op)+1;
      if (oplen > 1)
      {
        printf("op(%s,", (char *) expr->op_expr->op);
      }
      else
      {
        printf("app(");
      }
      print_ast(expr->op_expr->a);
      printf(",");
      print_ast(expr->op_expr->b);
      printf(")");
      break;
    }
    case EXPR_FN:
    {
      if (!PTR_VALID(expr->fn_expr))
      {
        fprintf(stderr, "Invalid fn_expr\n");
        exit(1);
      }
      printf("fn(%s,", (char*) expr->fn_expr->name);
      print_ast(expr->fn_expr->body);
      printf(")");
      break;
    }
    default:
    {
      printf("unknown expression type %d\n", expr->type);
    }
  }
}

GC_USER_FUNC GC_CAP val_t *
lookup (GC_CAP char * name, GC_CAP env_t * env)
{
  while (PTR_VALID(env))
  {
    if (!cstrcmp(name, env->name))
    {
      return env->val;
    }
    GC_STORE_CAP(env, env->next);
  }
  fprintf(stderr, "unbound variable: %s\n", (char*) name);
  exit(1);
  return GC_INVALID_PTR(); // should never reach here
}

GC_USER_FUNC GC_CAP val_t *
eval (GC_CAP expr_t * expr, GC_CAP env_t * env)
{
  if (!PTR_VALID(expr))
  {
    printf("eval: invalid expression\n");
    return GC_INVALID_PTR();
  }
  switch (expr->type)
  {
    case EXPR_IF:
    {
      GC_CAP val_t * cond = GC_INVALID_PTR();
      GC_STORE_CAP(cond, eval(expr->if_expr->cond, env));
      if (!PTR_VALID(cond))
        return GC_INVALID_PTR();
      if ( cond->type != VAL_NUM )
      {
        fprintf(stderr, "eval: type error: if condition not a number\n");
        exit(1);
      }
      
      if (cond->num_val->num)
      {
        return eval(expr->if_expr->true_expr, env);
      }
      else
      {
        return eval(expr->if_expr->false_expr, env);
      }
    }
    case EXPR_NAME:
    {
      return lookup(expr->name_expr->name, env);
    }
    case EXPR_NUM:
    {
      GC_CAP val_t * val = GC_INVALID_PTR();
      GC_STORE_CAP(val, ml_malloc(sizeof(val_t)));
      if (!PTR_VALID(val))
      {
        fprintf(stderr, "eval: out of memory allocating val_t\n");
        exit(1);
      }
      val->type = VAL_NUM;
      val->num_val = GC_INVALID_PTR();
      GC_STORE_CAP(val->num_val, ml_malloc(sizeof(num_val_t)));
      if (!PTR_VALID(val->num_val))
      {
        fprintf(stderr, "eval: out of memory allocating num_val_t\n");
        exit(1);
      }
      val->num_val->num = expr->num_expr->num;
      return val;
    }
    case EXPR_OP:
    {
      // No short-circuiting for operators and no call-by-name for now.
      // Can emulate call-by-name by passing a function by value instead if need
      // be..
      GC_CAP val_t * a = GC_INVALID_PTR();
      GC_STORE_CAP(a, eval(expr->op_expr->a, env));
      GC_CAP val_t * b = GC_INVALID_PTR();
      GC_STORE_CAP(b, eval(expr->op_expr->b, env));
      
      size_t oplen = cstrlen(expr->op_expr->op)+1;
      if (oplen > 1)
      {
        if ( a->type != VAL_NUM )
        {
          fprintf(stderr, "eval: type error: operator argument not a number\n");
          exit(1);
        }
        if ( b->type != VAL_NUM )
        {
          fprintf(stderr, "eval: type error: operator argument not a number\n");
          exit(1);
        }
        
        num_t a_num = a->num_val->num;
        num_t b_num = b->num_val->num;
        num_t result = 0;

        GC_CAP val_t * val = GC_INVALID_PTR();
        GC_STORE_CAP(val, ml_malloc(sizeof(val_t)));
        if (!PTR_VALID(val))
        {
          fprintf(stderr, "eval: out of memory allocating val_t\n");
          exit(1);
        }
        val->type = VAL_NUM;
        val->num_val = GC_INVALID_PTR();
        GC_STORE_CAP(val->num_val, ml_malloc(sizeof(num_val_t)));
        if (!PTR_VALID(val->num_val))
        {
          fprintf(stderr, "eval: out of memory allocating num_val_t\n");
          exit(1);
        }
        char op = expr->op_expr->op[0];
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
        
        val->num_val->num = result;
        return val;
      }
      else
      {
        // function application
        if (a->type != VAL_FN)
        {
          fprintf(stderr, "eval: type error: not a function\n");
          exit(1);
        }
        
        // add new environment entry to the function value's saved environment
        GC_CAP env_t * new_env = GC_INVALID_PTR();
        GC_STORE_CAP(new_env, ml_malloc(sizeof(env_t)));
        if (!PTR_VALID(new_env))
        {
          fprintf(stderr, "eval: out of memory allocating env_t\n");
          exit(1);
        }
        new_env->name = GC_INVALID_PTR();
        GC_STORE_CAP(new_env->name, a->fn_val->name);
        new_env->val = GC_INVALID_PTR();
        GC_STORE_CAP(new_env->val, b);
        new_env->next = GC_INVALID_PTR();
        GC_STORE_CAP(new_env->next, a->fn_val->env);
        return eval(a->fn_val->body, new_env);
      }
      return GC_INVALID_PTR(); // should never reach here
    }
    case EXPR_FN:
    {
      GC_CAP val_t * val = GC_INVALID_PTR();
      GC_STORE_CAP(val, ml_malloc(sizeof(val_t)));
      if (!PTR_VALID(val))
      {
        fprintf(stderr, "eval: out of memory allocating val_t\n");
        exit(1);
      }
      val->type = VAL_FN;
      val->fn_val = GC_INVALID_PTR();
      GC_STORE_CAP(val->fn_val, ml_malloc(sizeof(fn_val_t)));
      if (!PTR_VALID(val->fn_val))
      {
        fprintf(stderr, "eval: out of memory allocating fn_val_t\n");
        exit(1);
      }
      val->fn_val->name = GC_INVALID_PTR();
      GC_STORE_CAP(val->fn_val->name, expr->fn_expr->name);
      val->fn_val->body = GC_INVALID_PTR();
      GC_STORE_CAP(val->fn_val->body, expr->fn_expr->body);
      val->fn_val->env = GC_INVALID_PTR();
      GC_STORE_CAP(val->fn_val->env, env);
      return val;
    }
    default:
    {
      printf("unknown expression type %d\n", expr->type);
      return GC_INVALID_PTR();
    }
  }
}
