#include "parse.h"

#include <gc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
parse_init (void)
{
  parse_get_next_tok();
}

GC_CAP expr_t *
parse (void)
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
  else if (parse_tok_eq(TKINT, GC_INVALID_PTR))
  {
    ((expr_t *) expr)->type = EXPR_NUM;
    ((expr_t *) expr)->num_expr = parse_num();
    if (!PTR_VALID(((expr_t *) expr)->num_expr)) expr = GC_INVALID_PTR;
  }
  else
  {
    parse_unexpected();
  }
  return expr;
}

#include <gc_init.h>
#include <gc_debug.h>
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
  parse_get_next_tok();
  
  parse_expect(TKWORD, GC_cheri_ptr("then", sizeof("then")));
  parse_get_next_tok();
  
  ((if_expr_t *) if_expr)->true_expr = parse();
  if (!PTR_VALID( ((if_expr_t *) if_expr)->true_expr))
    return GC_INVALID_PTR;
  parse_get_next_tok();
  
  parse_expect(TKWORD, GC_cheri_ptr("else", sizeof("else")));
  parse_get_next_tok();
  
  ((if_expr_t *) if_expr)->false_expr = parse();
  if (!PTR_VALID( ((if_expr_t *) if_expr)->false_expr))
    return GC_INVALID_PTR;
  parse_get_next_tok();
  
  return if_expr;
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
  return num_expr;
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
  fprintf(stderr, "unexpected token %s\n", (char*) parse_state.tok.str);
  exit(1);
}

void
parse_expect (int type, GC_CAP const char * str)
{
  parse_eof_error();
  if (!parse_tok_eq(type, str))
  {
    fprintf(stderr, "expected %s, not %s\n", (const char *) str, 
            (char*) parse_state.tok.str);
    exit(1);
  }
}
