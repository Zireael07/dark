/*
  jsonscript-full.c
  A small lisp-y interpreter
  Based on Build Your Own Lisp - http://www.buildyourownlisp.com/contents
*/

#include <errno.h>

/* Forward Declarations */

struct script_val;
struct script_env;
typedef struct script_val script_val;
typedef struct script_env script_env;

/* Create Enumeration of possible value Types */
enum { SVAL_ERR, SVAL_NUM, SVAL_SYM, SVAL_FUN, SVAL_SEXPR };

typedef script_val*(*lbuiltin)(script_env*, script_val*);

typedef struct script_val {
  int type; /* See enum above */

  /* Basic */
  long num;
  /* Error and Symbol types have some string data */
  char* err;
  char* sym;
  lbuiltin fun;
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

script_val* script_val_fun(lbuiltin func) {
  script_val* v = malloc(sizeof(script_val));
  v->type = SVAL_FUN;
  v->fun = func;
  return v;
}

//Cleanup our stuff
void script_val_del(script_val* v) {

  switch (v->type) {
    case SVAL_NUM: break;
    case SVAL_FUN: break;
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
    case SVAL_FUN:   printf("<function>"); break;
  }
}

void script_val_println(script_val* v) { script_val_print(v); putchar('\n'); }

/* Necessary for the environment */
script_val* script_val_copy(script_val* v) {

  script_val* x = malloc(sizeof(script_val));
  x->type = v->type;

  switch (v->type) {

    /* Copy Functions and Numbers Directly */
    case SVAL_FUN: x->fun = v->fun; break;
    case SVAL_NUM: x->num = v->num; break;

    /* Copy Strings using malloc and strcpy */
    case SVAL_ERR:
      x->err = malloc(strlen(v->err) + 1);
      strcpy(x->err, v->err); break;

    case SVAL_SYM:
      x->sym = malloc(strlen(v->sym) + 1);
      strcpy(x->sym, v->sym); break;

    /* Copy Lists by copying each sub-expression */
    case SVAL_SEXPR:
      x->count = v->count;
      x->cell = malloc(sizeof(script_val*) * x->count);
      for (int i = 0; i < x->count; i++) {
        x->cell[i] = script_val_copy(v->cell[i]);
      }
    break;
  }

  return x;
}

/* Environment */
struct script_env {
  int count;
  char** syms;
  script_val** vals;
};

script_env* script_env_new(void) {
  script_env* e = malloc(sizeof(script_env));
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

void script_env_del(script_env* e) {
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    script_val_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
}

script_val* script_env_get(script_env* e, script_val* k) {

  /* Iterate over all items in environment */
  for (int i = 0; i < e->count; i++) {
    //debug
    //printf("symbol: %s in env: %s\n", k->sym, e->syms[i]);
    /* Check if the stored string matches the symbol string */
    /* If it does, return a copy of the value */
    if (strcmp(e->syms[i], k->sym) == 0) {
      return script_val_copy(e->vals[i]);
    }
  }
  /* If no symbol found return error */
  return script_val_err("unbound symbol %s!", k->sym);
}

void script_env_put(script_env* e, script_val* k, script_val* v) {

  /* Iterate over all items in environment */
  /* This is to see if variable already exists */
  for (int i = 0; i < e->count; i++) {

    /* If variable is found delete item at that position */
    /* And replace with variable supplied by user */
    if (strcmp(e->syms[i], k->sym) == 0) {
      script_val_del(e->vals[i]);
      e->vals[i] = script_val_copy(v);
      return;
    }
  }

  /* If no existing entry found allocate space for new entry */
  e->count++;
  e->vals = realloc(e->vals, sizeof(script_val*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);

  /* Copy contents of lval and symbol string into new location */
  e->vals[e->count-1] = script_val_copy(v);
  e->syms[e->count-1] = malloc(strlen(k->sym)+1);
  strcpy(e->syms[e->count-1], k->sym);
}


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
script_val* builtin_op(script_env* e, script_val* a, char* op) {

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


script_val* builtin_add(script_env* e, script_val* a) { return builtin_op(e, a, "+"); }
script_val* builtin_sub(script_env* e, script_val* a) { return builtin_op(e, a, "-"); }
script_val* builtin_mul(script_env* e, script_val* a) { return builtin_op(e, a, "*"); }
script_val* builtin_div(script_env* e, script_val* a) { return builtin_op(e, a, "/"); }

/* forward declare */
script_val* script_val_eval(script_env* e, script_val* v);

script_val* builtin_if(script_env* e, script_val* a) {
  /* Mark Both Expressions as evaluable */
  script_val* x;
  //FIXME: for some reason, this caused a crash
  //a->cell[1]->type = SVAL_SEXPR;
  //a->cell[2]->type = SVAL_SEXPR;

  if (a->cell[0]->num) {
    printf("Condition is true\n");
    /* If condition is true (=1) evaluate first expression */
    x = script_val_eval(e, script_val_pop(a, 1));
  } else {
    printf("Condition is false\n");
    /* Otherwise (0) evaluate second expression */
    x = script_val_eval(e, script_val_pop(a, 2));
  }

  /* Delete argument list and return */
  script_val_del(a);
  return x;
}
  

void script_env_add_builtin(script_env* e, char* name, lbuiltin func) {
  script_val* k = script_val_sym(name);
  script_val* v = script_val_fun(func);
  script_env_put(e, k, v);
  script_val_del(k); script_val_del(v);
}

//Needs to come AFTER all function definitions
void script_env_add_builtins(script_env* e) {
  /* Mathematical Functions */
  script_env_add_builtin(e, "+", builtin_add);
  script_env_add_builtin(e, "-", builtin_sub);
  script_env_add_builtin(e, "*", builtin_mul);
  script_env_add_builtin(e, "/", builtin_div);
  /* Comparison Functions */
  script_env_add_builtin(e, "if", builtin_if);
}



/* Evaluation */
script_val* script_val_eval_sexpr(script_env* e, script_val* v) {

  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = script_val_eval(e, v->cell[i]);
  }

  /* Error Checking */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == SVAL_ERR) { return script_val_take(v, i); }
  }

  /* Empty Expression */
  if (v->count == 0) { return v; }

  /* Single Expression */
  if (v->count == 1) { return script_val_take(v, 0); }

  /* Ensure first element is a function after evaluation */
  script_val* f = script_val_pop(v, 0);
  if (f->type != SVAL_FUN) {
    script_val_del(v); script_val_del(f);
    return script_val_err("first element is not a function");
  }

  /* If so call function to get result */
  script_val* result = f->fun(e, v);
  script_val_del(f);
  return result;
}


script_val* script_val_eval(script_env* e, script_val* v) {
  /* Gets symbols from environment */
  if (v->type == SVAL_SYM) {
    script_val* x = script_env_get(e, v);
    script_val_del(v);
    return x;
  }
  /* Evaluate Sexpressions */
  if (v->type == SVAL_SEXPR) { return script_val_eval_sexpr(e, v); }
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

script_val* script_val_read_sym(char* s, int* i, bool stringified) {
  
  /* Allocate Empty String */
  char* part = calloc(1,1);
  
  if (stringified){
    /* More forward one step past initial " character */
    (*i)++;
  }


  /* While valid identifier characters */
  while (is_valid_identifier(s[*i])) {
    
    /* Append character to end of string */
    part = realloc(part, strlen(part)+2);
    part[strlen(part)+1] = '\0';
    part[strlen(part)+0] = s[*i];
    (*i)++;
  }
  
  /* For JSON compat - check for specials: true, false and null */
  if (!stringified) {
    /* Convert to numbers for internal use */
    if (strcmp(part, "true") == 0) {
      printf("identifier is true\n");
      //strings are nasty to work with in C 
      //strlen() does NOT include the null termination
			//character, so always malloc() the reported length + 1!!
			part = calloc(2, sizeof(char));
			strcpy(part, "1\0");
    }
    if (strcmp(part, "false") == 0) {
      printf("Identifier is false\n");
      //strlen() does NOT include the null termination
			//character, so always malloc() the reported length + 1!!
			part = calloc(2, sizeof(char));
			strcpy(part, "0\0");
    }
    if (strcmp(part, "null") == 0) {
      printf("Identifier is null\n");
      //strlen() does NOT include the null termination
			//character, so always malloc() the reported length + 1!!
			part = calloc(2, sizeof(char));
			strcpy(part, "0\0");
    }
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

  if (stringified){
    /* More forward one step past initial " character */
    (*i)++;
  }
  
  /* Free temp string */
  free(part);
  
  /* Return val */
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
  /* For JSON compat; brackets must match, whichever kind we pick */
  else if (s[*i] == '[') {
    (*i)++;
    x = script_val_read_expr(s, i, ']');
  }

  /* If next character is a valid identifier, read symbol */
  else if (is_valid_identifier(s[*i])) {
    x = script_val_read_sym(s,i, false);
  }

  /* For JSON compat - if next character is " then read symbol */
  else if (strchr("\"", s[*i])) {
  //else if (is_valid_identifier(s[*i])) {
    x = script_val_read_sym(s, i, true);
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
  //char* input = "[ + 2 [* 3, 4] ]";
  FILE *fp;
	long lSize;
	char *input;

	fp = fopen ( "assets/scripts/if.json" , "rb" );
	if( !fp ) { printf("Could not open JSON source"); }

	fseek( fp , 0L , SEEK_END);
	lSize = ftell( fp );
	rewind( fp );

	/* allocate memory for entire content */
	input = calloc( 1, lSize+1 );
	if( !input ) fclose(fp),printf("memory alloc fails");

	/* copy the file into the buffer */
	if( 1!=fread( input , lSize, 1 , fp) )
	fclose(fp),free(input),printf("entire read fails");
  
  /* Scripting engine starts here */
  script_env* e = script_env_new();
  script_env_add_builtins(e);

  /* Read from input to create an S-Expr */
  int pos = 0;
  script_val* expr = script_val_read_expr(input, &pos, '\0');
  //script_val_println(expr); //debug
  /* Evaluate and print input */
  script_val* x = script_val_eval(e, expr);
  script_val_println(x);
  script_val_del(x);

  //free(input);
  script_env_del(e);
}

