#include "parse.h"

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

// used by function application parsing
int
parser_is_at_start_of_expression (void)
{
  // TODO: this
  return
    (parse_tok_eq(TKWORD, GC_INVALID_PTR)
      && !parse_tok_eq(TKWORD, GC_cheri_ptr("then", sizeof("then")))
      && !parse_tok_eq(TKWORD, GC_cheri_ptr("else", sizeof("else"))))
    || (parse_tok_eq(TKSYM, GC_cheri_ptr("(", sizeof("("))))
    || (parse_tok_eq(TKINT, GC_INVALID_PTR));
}

void
parse_init (void)
{
  parse_get_next_tok();
}

GC_CAP expr_t *
parse (void)
{
  return parse_app();
}

GC_CAP expr_t *
parse_app (void)
{
  return parse_op(GC_cheri_ptr("", sizeof("")), &parse_cons);
}

GC_CAP expr_t *
parse_cons (void)
{
  return parse_op(GC_cheri_ptr("::", sizeof("::")), &parse_greater_than);
}

GC_CAP expr_t *
parse_greater_than (void)
{
  return parse_op(GC_cheri_ptr(">", sizeof(">")), &parse_less_than);
}

GC_CAP expr_t *
parse_less_than (void)
{
  return parse_op(GC_cheri_ptr("<", sizeof("<")), &parse_add);
}

GC_CAP expr_t *
parse_add (void)
{
  return parse_op(GC_cheri_ptr("+", sizeof("+")), &parse_sub);
}

GC_CAP expr_t *
parse_sub (void)
{
  return parse_op(GC_cheri_ptr("-", sizeof("-")), &parse_mul);
}

GC_CAP expr_t *
parse_mul (void)
{
  return parse_op(GC_cheri_ptr("*", sizeof("*")), &parse_div);
}

GC_CAP expr_t *
parse_div (void)
{
  return parse_op(GC_cheri_ptr("/", sizeof("/")), &parse_base_expr);
}

// (left-assoc)
// if op is "" then we're parsing a function application instead.
GC_CAP expr_t *
parse_op (GC_CAP const char * op,
          GC_CAP expr_t * (*lower_precendence_func)(void))
{
  GC_CAP expr_t * expr = GC_malloc(sizeof(expr_t));
  if (!PTR_VALID(expr))
  {
    fprintf(stderr, "parse_op(): out of memory\n");
    exit(1);
  }
  ((expr_t*)expr)->type = EXPR_OP;
  
  ((expr_t*)expr)->op_expr = GC_malloc(sizeof(op_expr_t));
  if (!PTR_VALID(((expr_t*)expr)->op_expr))
  {
    fprintf(stderr, "parse_op(): out of memory\n");
    exit(1);
  }
  
  GC_CAP expr_t * a = lower_precendence_func();
  if (!PTR_VALID(a)) return a;
  
  if (GC_cheri_getlen(op) > 1)
  {
    // op is a normal operator, check if it's present
    if (!parse_tok_eq(TKSYM, op)) return a;
  }
  else
  {
    // op is a function application; check if at valid start of another
    // expression
    if (!parser_is_at_start_of_expression()) return a;
  }
  
  ((op_expr_t *) ((expr_t*)expr)->op_expr)->a = a;
  ((op_expr_t *) ((expr_t*)expr)->op_expr)->b = GC_INVALID_PTR;
  ((op_expr_t *) ((expr_t*)expr)->op_expr)->op = copy_string(op);
  while (1)
  {
    if (GC_cheri_getlen(op) > 1)
    {
      // normal op
      if (!parse_tok_eq(TKSYM, op)) break;
      parse_get_next_tok();
      parse_eof_error();
    }
    else
    {
      // function application op
      if (!parser_is_at_start_of_expression()) break;
    }
    
    GC_CAP expr_t * b = lower_precendence_func();
    if (!PTR_VALID(b)) return b;
    
    if (!PTR_VALID(((op_expr_t *)((expr_t*)expr)->op_expr)->b))
    {
      ((op_expr_t *)((expr_t*)expr)->op_expr)->b = b;
    }
    else
    {
      GC_CAP op_expr_t * new_op_expr = GC_malloc(sizeof(op_expr_t));
      if (!PTR_VALID(new_op_expr))
      {
        fprintf(stderr, "parse_op(): out of memory\n");
        exit(1);
      }
      ((op_expr_t *) new_op_expr)->a = expr;
      ((op_expr_t *) new_op_expr)->b = b;
      ((op_expr_t *) new_op_expr)->op = copy_string(op);

      expr = GC_malloc(sizeof(expr_t));
      if (!PTR_VALID(expr))
      {
        fprintf(stderr, "parse_op(): out of memory\n");
        exit(1);
      }
      ((expr_t*)expr)->type = EXPR_OP;
      ((expr_t*)expr)->op_expr = new_op_expr;
    }
  }
  return expr;
}

GC_CAP expr_t *
parse_base_expr (void)
{
  GC_CAP expr_t * expr = GC_malloc(sizeof(expr_t));
  if (!PTR_VALID(expr))
  {
    fprintf(stderr, "parse(): out of memory\n");
    exit(1);
  }
  if (parse_tok_eq(TKWORD, GC_cheri_ptr("if", sizeof("if"))))
  {
    ((expr_t *) expr)->type = EXPR_IF;
    ((expr_t *) expr)->if_expr = parse_if();
    if (!PTR_VALID(((expr_t *) expr)->if_expr)) expr = GC_INVALID_PTR;
  }
  else if (parse_tok_eq(TKWORD, GC_cheri_ptr("fn", sizeof("fn"))))
  {
    ((expr_t *) expr)->type = EXPR_FN;
    ((expr_t *) expr)->fn_expr = parse_fn();
    if (!PTR_VALID(((expr_t *) expr)->fn_expr)) expr = GC_INVALID_PTR;
  }
  else if (parse_tok_eq(TKWORD, GC_INVALID_PTR))
  {
    ((expr_t *) expr)->type = EXPR_NAME;
    ((expr_t *) expr)->name_expr = parse_name();
    if (!PTR_VALID(((expr_t *) expr)->name_expr)) expr = GC_INVALID_PTR;
  }
  else if (parse_tok_eq(TKINT, GC_INVALID_PTR))
  {
    ((expr_t *) expr)->type = EXPR_NUM;
    ((expr_t *) expr)->num_expr = parse_num();
    if (!PTR_VALID(((expr_t *) expr)->num_expr)) expr = GC_INVALID_PTR;
  }
  else if (parse_tok_eq(TKSYM, GC_cheri_ptr("(", sizeof("("))))
  {
    parse_get_next_tok();
    expr = parse();
    if (PTR_VALID(expr))
    {
      parse_expect(TKSYM, GC_cheri_ptr(")", sizeof(")")));
      parse_get_next_tok();
    }
  }
  else
  {
    parse_unexpected();
  }
  return expr;
}

GC_CAP if_expr_t *
parse_if (void)
{
  GC_CAP if_expr_t * if_expr = GC_malloc(sizeof(if_expr_t));
  if (!PTR_VALID(if_expr))
  {
    fprintf(stderr, "parse_if(): out of memory\n");
    exit(1);
  }
  parse_get_next_tok();
  parse_eof_error();
  
  ((if_expr_t *) if_expr)->cond = parse();
  if (!PTR_VALID( ((if_expr_t *) if_expr)->cond))
    return GC_INVALID_PTR;
  parse_eof_error();
  
  parse_expect(TKWORD, GC_cheri_ptr("then", sizeof("then")));
  parse_get_next_tok();
  parse_eof_error();
  
  ((if_expr_t *) if_expr)->true_expr = parse();
  if (!PTR_VALID( ((if_expr_t *) if_expr)->true_expr))
    return GC_INVALID_PTR;
  parse_eof_error();
  
  parse_expect(TKWORD, GC_cheri_ptr("else", sizeof("else")));
  parse_get_next_tok();
  parse_eof_error();
  
  ((if_expr_t *) if_expr)->false_expr = parse();
  if (!PTR_VALID( ((if_expr_t *) if_expr)->false_expr))
    return GC_INVALID_PTR;  
  return if_expr;
}

GC_CAP fn_expr_t *
parse_fn (void)
{
  GC_CAP fn_expr_t * fn_expr = GC_malloc(sizeof(fn_expr_t));
  if (!PTR_VALID(fn_expr))
  {
    fprintf(stderr, "parse_fn(): out of memory\n");
    exit(1);
  }
  parse_get_next_tok();
  parse_eof_error();
  
  // parse the argument name
  parse_expect(TKWORD, GC_INVALID_PTR);
  GC_CAP name_expr_t * name_expr = parse_name();
  if (!PTR_VALID(name_expr))
    return GC_INVALID_PTR;
  ((fn_expr_t *) fn_expr)->name = ((name_expr_t *) name_expr)->name;
  parse_eof_error();

  parse_expect(TKSYM, GC_cheri_ptr(".", sizeof(".")));
  parse_get_next_tok();
  parse_eof_error();
  
  
  ((fn_expr_t *) fn_expr)->body = parse();
  if (!PTR_VALID( ((fn_expr_t *) fn_expr)->body))
    return GC_INVALID_PTR;
  return fn_expr;
}

GC_CAP num_expr_t *
parse_num (void)
{
  GC_CAP num_expr_t * num_expr = GC_malloc(sizeof(num_expr_t));
  if (!PTR_VALID(num_expr))
  {
    fprintf(stderr, "parse_num(): out of memory\n");
    exit(1);
  }
  
  ((num_expr_t *) num_expr)->num = 0;
  size_t i;
  for (i=0; i<parse_state.tok.len-1; i++)
  {
    ((num_expr_t *) num_expr)->num *= 10;
    ((num_expr_t *) num_expr)->num += ((char*)parse_state.tok.str)[i] - '0';
  }
  parse_get_next_tok();
  return num_expr;
}

GC_CAP name_expr_t *
parse_name (void)
{
  GC_CAP name_expr_t * name_expr = GC_malloc(sizeof(name_expr_t));
  if (!PTR_VALID(name_expr))
  {
    fprintf(stderr, "parse_name(): out of memory\n");
    exit(1);
  }
  ((name_expr_t *) name_expr)->name = parse_state.tok.str;
  parse_get_next_tok();
  return name_expr;
}

void
parse_get_next_tok (void)
{
  parse_state.tok = lex();
}

// Set str to GC_INVALID_PTR to ignore
int
parse_tok_eq (int type, GC_CAP const char * str)
{
  return
  (
    PTR_VALID(str) &&
    (type == parse_state.tok.type) &&
    PTR_VALID(parse_state.tok.str) &&
    (!strcmp((const char*)str,(char*)parse_state.tok.str))
  ) ||
  (
    !PTR_VALID(str) &&
    (type == parse_state.tok.type)
  );
}

void
parse_eof_error (void)
{
  if (parse_tok_eq(TKEOF, GC_INVALID_PTR))
  {
    fprintf(stderr, "unexpected eof while parsing\n");
    exit(1);
  }
}

void
parse_unexpected (void)
{
  parse_eof_error(); // because then the string is invalid
  fprintf(stderr, "unexpected token %s (near character %d)\n",
    (char*) parse_state.tok.str,
    (int) parse_state.tok.nearby_character);
  exit(1);
}

void
parse_expect (int type, GC_CAP const char * str)
{
  parse_eof_error();
  if (!parse_tok_eq(type, str))
  {
    fprintf(stderr, "expected `%s', not `%s' (near character %d)\n",
            (const char *) str, 
            (char*) parse_state.tok.str,
            (int) parse_state.tok.nearby_character);
    exit(1);
  }
}