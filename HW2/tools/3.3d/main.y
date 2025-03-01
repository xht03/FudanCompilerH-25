%{
#include <cstdio>
#include <vector>
using namespace std;

void yyerror(const char *s);
int yylex(void);

int brace_count = 0;
vector<int> paren_count;

%}

%token LPAREN RPAREN LBRACKET RBRACKET

%%

input:
    | input expr
    ;

expr:
    LBRACKET { brace_count++; paren_count.push_back(0); }
    | RBRACKET { if (brace_count > 0) {paren_count.pop_back(); brace_count--;} else return -1; }
    | LPAREN { if (brace_count >= 0) paren_count.back()++; }
    | RPAREN { if (brace_count >= 0 && paren_count.back() > 0) paren_count.back()--; else return -1; }
    ;

%%

int main() {
    if (yyparse() == 0 && brace_count == 0 && paren_count.size() == 1 && paren_count.back() == 0) {
        printf("Accept\n");
    } else {
        printf("Reject\n");
        printf("brace_count = %d\n", brace_count);
        printf("paren_count.size() = %lu\n", paren_count.size());
        printf("paren_count.back() = %d\n", paren_count.back());
    }
    return 0;
}

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
}