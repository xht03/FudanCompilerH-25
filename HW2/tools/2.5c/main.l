%{
#include <stdio.h>
%}


%%

cat|cats|car|cars   { printf("Accept!\n"); }
.|\n                { printf("Reject!\n"); }

%%

int main(int argc, char **argv)
{
    yylex();
    return 0;
}

int yywrap()
{
    return 1;
}