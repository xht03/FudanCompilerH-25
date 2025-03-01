%{
#include "main.tab.h"
#include <stdio.h>
%}

%%

"("     { return LPAREN; }
")"     { return RPAREN; }
"["     { return LBRACKET; }
"]"     { return RBRACKET; }
\n      { return 0; }
[ \t]   { /* Ignore whitespace */ }
.       { /*yyerror("Invalid character");*/ }

%%

int yywrap() {
    return 1;
}