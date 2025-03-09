%{
#include<stdio.h>
#include "calc.h"

int base;

extern int yylex();
extern void yyerror(char*);
extern int  yywrap();

%}

%token <a> DIGIT
%token <a> LETTER

%union {A_exp e; A_stm s; int a;}

%type <a> number
%type <s> stat
%type <e> expr

%left '-'
%left '*'

%start list

%%                   /* beginning of rules section */

list:	                      /*empty */
         |
        list stat
         { printf("\n");}
         |
        list error '\n'
         {
           yyerrok;
         }
         ;
stat:    expr '\n'
         {
           printExp($1);
         }
         ;

expr:    expr '*' expr
         {

           $$ = A_OpExp($1, A_times, $3);
         }
         |
         expr '-' expr
         {
           $$ = A_OpExp($1, A_minus, $3);
         }
	 |
	 '-' expr
         {
           $$ = A_OpExp(A_NumExp(0), A_minus, $2);
         }
         |
         number
         {
           $$ = A_NumExp($1);
         }
         ;

number:  DIGIT
         {
           $$ = $1;
           base = ($1==0) ? 8 : 10;
         }       |
         number DIGIT
         {
           $$ = base * $1 + $2;
         }
         ;

%%

void yyerror(s)
char *s;
{
  fprintf(stderr, "%s\n",s);
}

int yywrap()
{
  return(1);
}
