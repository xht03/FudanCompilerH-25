%{
#include <stdio.h>

int mlen = 0, len = 0;
%}

%s MATCHED

%%

<INITIAL>a((b|a*c)x)*|x*a    { mlen = yyleng; len = yyleng; BEGIN(MATCHED); }
<MATCHED>.                   { len += yyleng; }
<INITIAL,MATCHED><<EOF>>     {
                                if (len == mlen)
                                    printf("accept\n");
                                else
                                    printf("reject\n");
                                return 0;
                             }
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