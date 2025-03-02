%{
#include <stdio.h>
#include <stdlib.h>

void yyerror(const char *s);
int yylex(void);

int brace_count = 0;            // Count of unmatched open brackets
int paren_count[100] = {0};     // Count of unmatched open parentheses within each pair of brackets

%}

%token LPAREN RPAREN LBRACKET RBRACKET

%%

input:
    | input expr
    ;

expr:
    LBRACKET { 
        brace_count++;
        paren_count[brace_count] = 0; 
    }
    | RBRACKET { 
        if (brace_count > 0) {
            paren_count[brace_count] = 0;
            brace_count--;
        } 
        else return -1; 
    }
    | LPAREN { 
        if (brace_count >= 0) 
            paren_count[brace_count]++; 
    }
    | RPAREN { 
        if (brace_count >= 0 && paren_count[brace_count] > 0) 
            paren_count[brace_count]--; 
        else return -1; 
    }
    ;

%%

int main() {
    if (yyparse() == 0 && brace_count == 0 && paren_count[0] == 0) {
        printf("Accept\n");
    } else {
        printf("Reject\n");
    }
    return 0;
}

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
}