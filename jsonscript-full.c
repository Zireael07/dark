/*
  jsonscript-full.c
  A small lisp-y interpreter
  Based on Build Your Own Lisp - http://www.buildyourownlisp.com/contents
*/

#include <errno.h>

/* Create Enumeration of possible value Types */
enum { SVAL_ERR, SVAL_NUM, SVAL_SYM, SVAL_SEXPR };

typedef struct script_val {
  int type; /* See enum above */

  /* Basic */
  long num;
  /* Error and Symbol types have some string data */
  char* err;
  char* sym;
  /* Count and Pointer to a list of "script val*" 
  ( for S-expressions)
  */
  int count;
  struct script_val** cell;
} script_val;


//Initializers for the types
/* Construct a pointer to a new Error script val */
script_val* script_val_err(char* fmt, ...) {
  script_val* v = malloc(sizeof(script_val));
  v->type = SVAL_ERR;  
  va_list va;
  va_start(va, fmt);  
  v->err = malloc(512);  
  vsnprintf(v->err, 511, fmt, va);  
  v->err = realloc(v->err, strlen(v->err)+1);
  va_end(va);  
  return v;
}

/* Construct a pointer to a new Number script val */
script_val* script_val_num(long x) {
  script_val* v = malloc(sizeof(script_val));
  v->type = SVAL_NUM;
  v->num = x;
  return v;
}

/* Construct a pointer to a new Symbol script val */
script_val* script_val_sym(char* s) {
  script_val* v = malloc(sizeof(script_val));
  v->type = SVAL_SYM;
  //because strings are 0-terminated!
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

/* S-Expressions represented as a variable sized array for simplicity */
/* A pointer to a new empty Sexpr script val */
script_val* script_val_sexpr(void) {
  script_val* v = malloc(sizeof(script_val));
  v->type = SVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

//Cleanup our stuff
void script_val_del(script_val* v) {

  switch (v->type) {
    case SVAL_NUM: break;
    /* For Err or Sym free the string data */
    case SVAL_ERR: free(v->err); break;
    case SVAL_SYM: free(v->sym); break;
    /* If Sexpr then delete all elements inside */
    case SVAL_SEXPR:
      for (int i = 0; i < v->count; i++) {
        script_val_del(v->cell[i]);
      }
      /* Also free the memory allocated to contain the pointers */
      free(v->cell);
    break;
  }
  /* Free the memory allocated for the "lval" struct itself */
  free(v);
}

/* Adds element to S-expression */ 
script_val* script_val_add(script_val* v, script_val* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(script_val*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

/* Print values */
void script_val_print(script_val* v);

void script_val_print_expr(script_val* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {
    /* Print Value contained within */
    script_val_print(v->cell[i]);    
    /* Don't print trailing space if last element */
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void script_val_print(script_val* v) {
  switch (v->type) {
    case SVAL_NUM:   printf("%li", v->num); break;
    case SVAL_ERR:   printf("Error: %s", v->err); break;
    case SVAL_SYM:   printf("%s", v->sym); break;
    case SVAL_SEXPR: script_val_print_expr(v, '[', ']'); break;
  }
}

void script_val_println(script_val* v) { script_val_print(v); putchar('\n'); }

/* Remove item at i and shift everything else backwards */
script_val* script_val_pop(script_val* v, int i) {
  /* Find the item at "i" */
  script_val* x = v->cell[i];

  /* Shift memory after the item at "i" over the top */
  memmove(&v->cell[i], &v->cell[i+1],
    sizeof(script_val*) * (v->count-i-1));

  /* Decrease the count of items in the list */
  v->count--;

  /* Reallocate the memory used */
  v->cell = realloc(v->cell, sizeof(script_val*) * v->count);
  return x;
}

script_val* script_val_take(script_val* v, int i) {
  script_val* x = script_val_pop(v, i);
  script_val_del(v);
  return x;
}

/* Built-ins */
script_val* builtin_op(script_val* a, char* op) {

  /* Ensure all arguments are numbers */
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != SVAL_NUM) {
      script_val_del(a);
      return script_val_err("Cannot operate on non-number!");
    }
  }

  /* Pop the first element */
  script_val* x = script_val_pop(a, 0);

  /* If no arguments and sub then perform unary negation */
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  /* While there are still elements remaining */
  while (a->count > 0) {

    /* Pop the next element */
    script_val* y = script_val_pop(a, 0);

    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        script_val_del(x); script_val_del(y);
        x = script_val_err("Division By Zero!"); break;
      }
      x->num /= y->num;
    }

    script_val_del(y);
  }

  script_val_del(a); 
  return x;
}


/* Evaluation */
/* forward declare */
script_val* script_val_eval(script_val* v);

script_val* script_val_eval_sexpr(script_val* v) {

  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = script_val_eval(v->cell[i]);
  }

  /* Error Checking */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == SVAL_ERR) { return script_val_take(v, i); }
  }

  /* Empty Expression */
  if (v->count == 0) { return v; }

  /* Single Expression */
  if (v->count == 1) { return script_val_take(v, 0); }

  /* Ensure First Element is Symbol */
  script_val* f = script_val_pop(v, 0);
  if (f->type != SVAL_SYM) {
    script_val_del(f); script_val_del(v);
    return script_val_err("S-expression Does not start with symbol!");
  }

  /* Call builtin with operator */
  script_val* result = builtin_op(v, f->sym);
  script_val_del(f);
  return result;
}


script_val* script_val_eval(script_val* v) {
  /* Evaluate Sexpressions */
  if (v->type == SVAL_SEXPR) { return script_val_eval_sexpr(v); }
  /* All other script val types remain the same */
  return v;
}

/* Hand rolled parser - http://www.buildyourownlisp.com/appendix_a_hand_rolled_parser */

/* Reading */

int is_valid_identifier(char s) {
  return strchr(
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789_+-*\\/=<>!&", s) && s != '\0';
}

int is_whitespace(char s){
  return strchr(" \t\v\r\n;,", s) && s != '\0';
}

script_val* script_val_read_sym(char* s, int* i) {
  
  /* Allocate Empty String */
  char* part = calloc(1,1);
  
  /* While valid identifier characters */
  while (is_valid_identifier(s[*i])) {
    
    /* Append character to end of string */
    part = realloc(part, strlen(part)+2);
    part[strlen(part)+1] = '\0';
    part[strlen(part)+0] = s[*i];
    (*i)++;
  }
  
  /* Check if Identifier looks like a number */
  int is_num = strchr("-0123456789", part[0]) != NULL;
  for (int j = 1; j < strlen(part); j++) {
    if (strchr("0123456789", part[j]) == NULL) { is_num = 0; break; }
  }
  if (strlen(part) == 1 && part[0] == '-') { is_num = 0; }
  
  /* Add Symbol or Number as script val */
  script_val* x = NULL;
  if (is_num) {
    errno = 0;
    long v = strtol(part, NULL, 10);
    x = (errno != ERANGE) ? script_val_num(v) : script_val_err("Invalid Number %s", part);
  } else {
    x = script_val_sym(part);
  }
  
  /* Free temp string */
  free(part);
  
  /* Return lval */
  return x;
}



script_val* script_val_read(char* s, int* i);

script_val* script_val_read_expr(char* s, int* i, char end) {
  
  script_val* x = script_val_sexpr();
  
  /* While not at end character keep reading script vals */
  while (s[*i] != end) {
    script_val* y = script_val_read(s, i);
    /* If an error then return this and stop */
    if (y->type == SVAL_ERR) {
      script_val_del(x);
      return y;
    } else {
      script_val_add(x, y);
    }
  }

  /* Move past end character */
  (*i)++;
  
  return x;

}

script_val* script_val_read(char* s, int* i) {
  
  /* Skip all trailing whitespace and comments */
  while (is_whitespace(s[*i])) {
    if (s[*i] == ';') {
      while (s[*i] != '\n' && s[*i] != '\0') { (*i)++; }
    }
    (*i)++;
  }
  
  script_val* x = NULL;

  /* If we reach end of input then we're missing something */
  if (s[*i] == '\0') {
    return script_val_err("Unexpected end of input");
  }
  
  /* If next character is ( or [ then read S-Expr */
  else if (s[*i] == '(') {
    (*i)++;
    x = script_val_read_expr(s, i, ')');
  }
  /* brackets must match, whichever kind we pick */
  else if (s[*i] == '[') {
    (*i)++;
    x = script_val_read_expr(s, i, ']');
  }

  /* If next character is part of a symbol then read symbol */
  else if (is_valid_identifier(s[*i])) {
    x = script_val_read_sym(s, i);
  }
  
  /* If next character is " then read string */
  // else if (strchr("\"", s[*i])) {
  //   x = script_val_read_str(s, i);
  // }
  
  /* Encountered some unexpected character */
  else {
    x = script_val_err("Unexpected character %c", s[*i]);
  }
  
  /* Skip all trailing whitespace and comments */
  while (is_whitespace(s[*i])) {
    if (s[*i] == ';') {
      while (s[*i] != '\n' && s[*i] != '\0') { (*i)++; }
    }
    (*i)++;
  }
  
  return x;
  
}

void parse_script() {
  char* input = "[ + 2 [* 3, 4] ]";
  /* Read from input to create an S-Expr */
  int pos = 0;
  script_val* expr = script_val_read_expr(input, &pos, '\0');
  /* Evaluate and print input */
  script_val* x = script_val_eval(expr);
  script_val_println(x);
  script_val_del(x);

  //free(input);
}

