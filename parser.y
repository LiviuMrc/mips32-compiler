%{
#include "pic.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
extern int yylex(void);
extern int yylineno;
void yyerror(const char *);

t_node *constToNode(int val);
t_node *opToNode(int type, int n, ...);
t_node *block(t_node *a, t_node *b);

int varCount;
t_varNode **variables;
t_node *varToNode(t_varNode *var);

t_node *root;
%}

%union{
	int val;
	t_varNode *var;
	t_node *node;
}

%define parse.error verbose
%token ASSIGN LOOP DELAY IF ELSE
%token <val> NUM
%token <var> LD SW

%type <node> stmt program stmtlist 

%%

program	: stmt			{ root = $$ = $1; }
	| program stmt		{ root = $$ = block($1, $2); }
	;

stmtlist : stmt			{ $$ = $1; }
	| stmtlist stmt		{ $$ = block($1, $2); }
	;

stmt	: LD '=' NUM ';'	{ $$ = opToNode(tAssign, 2, constToNode($3), varToNode($1)); }
	| LD '=' SW ';'		{ $$ = opToNode(tAssign, 2, varToNode($3), varToNode($1)); }
	| LOOP '{' stmtlist '}'	{ $$ = opToNode(tLoop, 1, $3); }
	| DELAY '(' NUM ')' ';'	{ $$ = opToNode(tDelay, 1, constToNode($3)); }
	| IF '(' SW ')' '{' stmtlist '}'	{ $$ = opToNode(tIf, 2, varToNode($3), $6); }
	| IF '(' SW ')' '{' stmtlist '}' ELSE '{' stmtlist '}'	{ $$ = opToNode(tIf, 3, varToNode($3), $6, $10); }
	;

%%

// Create a new variable. Variables that are attached to a port, should be made using this function.
t_varNode *newVar(char *name, char *port, unsigned int mask, int isInput){
	t_varNode *v = malloc(sizeof(t_varNode));
	v->name = name;
	varCount++;
	v->port = port;
	v->mask = mask;
	v->accessed = 0;
	v->isInput = isInput;
	variables = realloc(variables, varCount * sizeof(void*));
	variables[varCount - 1] = v;
	return v;
}

// This is for Flex to return the correct port type variable that has already been created. If you need additonal variables, then change this function.
t_varNode *findVar(char *name){
	int i;
	for(i = 0; i < varCount; i++)
		if(strcasecmp(variables[i]->name, name) == 0){
			variables[i]->accessed++;
			return variables[i];
		}	
	return NULL; //flex won't be allowed to generate new variables
}

void yyerror(const char *s){
	printf("Error at line %d: %s\n", yylineno, s);
}

t_node *constToNode(int val){ 
	t_node *p = malloc(sizeof(t_node));
	p->type = tConst;
	p->value = val;
	return p;
}

t_node *opToNode(int type, int n, ...){ 
	va_list args;
	t_node *p = malloc(sizeof(t_node));
	p->type = type;
	p->n = n;
	p->children = malloc(n * sizeof(void*));
	int i;
	va_start(args, n);
	for(i = 0; i < n; i++)
		p->children[i] = va_arg(args, t_node*);
	va_end(args);
	return p;
}

t_node *block(t_node *a, t_node *b){ 
	if(a == NULL && b != NULL)
		return b;
	if(a != NULL && b == NULL)
		return a;
	if(a == NULL && b == NULL)
		return NULL;
	if(a->type == tBlock){
		a->n++;
		a->children = realloc(a->children, a->n * sizeof(void*));
		a->children[a->n - 1] = b;
		return a;
	}else{
		t_node *p = malloc(sizeof(t_node));
		p->type = tBlock;
		p->n = 2;
		p->children = malloc(2 * sizeof(void*));		
		p->children[0] = a;
		p->children[1] = b;
		return p;
	}
}

t_node *varToNode(t_varNode *var){
	t_node *p = malloc(sizeof(t_node));
	p->type = tVar;
	p->var = var;
	return p;
}

