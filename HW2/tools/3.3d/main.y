%{
#include <stdio.h>
#include <stdlib.h>

void yyerror(const char *s);
int yylex(void);

%}

%token LPAREN RPAREN LBRACKET RBRACKET

%%

S:  T S
    | 
    ;

T:  LBRACKET U RBRACKET
    | LPAREN S RPAREN
    ;

U:  T U
    | LPAREN U
    |
    ;

%%

int main() {
    if (yyparse() == 0) {
        printf("accept\n");
    } else {
        printf("reject\n");
    }
    return 0;
}

void yyerror(const char *s) {
    /* fprintf(stderr, "Error: %s\n", s); */
}