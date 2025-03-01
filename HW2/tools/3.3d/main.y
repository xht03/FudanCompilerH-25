%{
#include <stdio.h>
#include <stdlib.h>

%}

%%

input:
    expr
    ;

expr:
    expr '(' expr ')' expr
    | expr '[' expr duplication expr']' expr
    |
    ;

duplication:
    '(' duplication
    |
    ;
    
%%

int main() {
    if (yyparse() == 0) {
        printf("Accept\n");
    } else {
        printf("Reject\n");
    }
    return 0;
}