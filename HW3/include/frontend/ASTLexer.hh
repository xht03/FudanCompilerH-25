#ifndef _ASTLexer_hh
#define _ASTLexer_hh

#include <iostream>
#include <fstream>
#include <string>
#if ! defined(yyFlexLexerOnce)
#define yyFlexLexer yy_ast_FlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer
#endif
#include "ast_location.hh"
#include "ASTheader.hh"
#include "FDMJAST.hh"

using namespace std;
using namespace fdmj;

#define IntExpList vector<IntExp*>
#define ClassDeclList vector<ClassDecl*>
#define VarDeclList vector<VarDecl*>
#define MethodDeclList vector<MethodDecl*>
#define FormalList vector<Formal*>
#define StmList vector<Stm*>
#define ExpList vector<Exp*>

namespace fdmj
{
    //These are the types of values (yylval) that a token can have
    //This can change if more types are added
    class AST_YYSTYPE
    {
        public:
            int i;
            string s;
            IntExp* intExp;
            IntExpList *intExpList;
            IdExp *idExp;
            OpExp *opExp;
            BoolExp *boolExp;
            Program *program;
            MainMethod *mainMethod;
            ClassDecl *classDecl;
            ClassDeclList *classDeclList;
            Type *type;
            VarDecl *varDecl;
            VarDeclList *varDeclList;
            MethodDecl *methodDecl;
            MethodDeclList *methodDeclList;
            FormalList *formalList;
            Stm* stm;
            StmList* stmList;
            Exp* exp;
            ExpList *expList;
            Program *root;
    };

    class ASTLexer : public yy_ast_FlexLexer
    {
        // 位置信息
        std::size_t currentLine = 1;
        std::size_t currentColumn = 1;
        
        // These are the values that the lexer will return
        AST_YYSTYPE *yylval = nullptr;      // 语义值
        location_t *yylloc = nullptr;       // 位置信息
        
         
        // lexer 将调用此函数，将匹配的词法单元的值复制到 yylval
        // 当一个词法单元被匹配并且需要将值返回到 yylval 时，我们会使用这个函数。
        void copyValue(const string s) { yylval->s = s; }   
        void copyValue(const int n) { yylval->i = n;}
        //void copyValue(const ASSTNode *node) { yylval->node = nullptr; exit(EXIT_FAILURE);} //the lexer shouldnt see node!

        // 这是一个将词法单元的位置复制到 yylloc 的函数
        // 我们将在 YY_USER_ACTION 中使用它，即每当一个模式被匹配时
        void copyLocation() { *yylloc = location_t(currentLine, currentColumn, currentLine, yyleng+currentColumn-1); 
                               currentColumn += yyleng; }  
        
    public:
        //The API to this lexer 
        ASTLexer(std::istream &in, const bool debug) : yy_ast_FlexLexer(&in) { yy_ast_FlexLexer::set_debug(debug); }    // 构造函数
        int yylex(AST_YYSTYPE *const lval, location_t *const lloc);     // 词法分析函数
    };
} //namespace fdmj
#endif