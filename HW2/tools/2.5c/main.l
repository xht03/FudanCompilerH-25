%{
#include <stdio.h>

int accepted = 0; // 标志变量，记录是否匹配
%}

%s MATCHED
%s END

%%

<INITIAL>cat|cats|car|cars   { accepted = 1; BEGIN MATCHED;}
<INITIAL>.|\n                { accepted = 0; BEGIN END;}
<MATCHED>.                   { accepted = 0; BEGIN END;}
<END>.                       { /* Do nothing */}

%%

int main(int argc, char **argv)
{
    yylex();
    if (accepted) {
        printf("accept\n");
    } else {
        printf("reject\n");
    }
    return 0;
}

int yywrap()
{
    return 1;
}