%skeleton "lalr1.cc"
%require  "3.2"
%language "c++"
%locations

// 这个块中的代码会被 Bison 插入到生成的头文件（通常是 parser.tab.hh）中。
%code requires {
  #define DEBUG
  #undef DEBUG
  #include <iostream>
  #include <vector>
  #include <algorithm>
  #include <string>
  #include <variant>
  #include "ast_location.hh"
  #include "ASTLexer.hh"
  #include "ASTheader.hh"
  #include "FDMJAST.hh"

  using namespace std;
  using namespace fdmj;
}

%define api.namespace {fdmj}              // 生成的所有解析器代码都会被放入 fdmj 命名空间中
%define api.parser.class {ASTParser}      // 生成的解析器类名为 ASTParser
%define api.value.type {AST_YYSTYPE}      // 定义语法分析中使用的语义值类型为 AST_YYSTYPE
%define api.location.type {ast_location}  // 定义用于跟踪源代码位置的类型为 ast_location

%define parse.error detailed      // 生成详细的错误信息
%define parse.trace               // 启用解析器的跟踪功能，生成能够输出解析过程详细信息的代码

%header
%verbose

// Bison 生成的解析器的构造函数将接收的参数
%parse-param {ASTLexer &lexer}
%parse-param {const bool debug}
%parse-param {AST_YYSTYPE* result}

%initial-action
{
    #if YYDEBUG != 0
        set_debug_level(debug);
    #endif
};

%code {
    namespace fdmj 
    {
        // 位置计算函数的前向声明
        template<typename RHS>
        void calcLocation(location_t &current, const RHS &rhs, const std::size_t n);
    }
    
    #define YYLLOC_DEFAULT(Cur, Rhs, N) calcLocation(Cur, Rhs, N)
    #define yylex lexer.yylex
    Pos *p;   // 全局位置指针
}

// 无值的终结符 
%token PUBLIC INT MAIN RETURN CLASS IF ELSE WHILE CONTINUE BREAK EXTENDS

%token TRUE FALSE THIS LENGTH GETINT GETCH GETARRAY PUTINT PUTCH PUTARRAY STARTTIME STOPTIME

%token '(' ')' '[' ']' '{' '}' '=' ',' ';' '.' 
%token ADD MINUS TIMES DIVIDE EQ NE LT LE GT GE AND OR NOT

// 有值的终结符
%token<i> NONNEGATIVEINT
%token<s> IDENTIFIER

// 非终结符，只需要类型信息（不是 token）
%type <program> PROG
%type <mainMethod> MAINMETHOD
%type <classDeclList> CLASSDECLLIST
%type <classDecl> CLASSDECL
%type <methodDeclList> METHODDECLLIST
%type <methodDecl> METHODDECL
%type <varDeclList> VARDECLLIST
%type <varDecl> VARDECL

%type <intExp> CONST
%type <intExpList> CONSTLIST
%type <intExpList> CONSTREST

%type <idExp> ID

%type <stmList> STMLIST
%type <stm> STM
%type <exp> EXP
%type <expList> EXPLIST
%type <expList> EXPREST
%type <type> TYPE
%type <formalList> FORMALLIST
%type <formalList> FORMALREST

%start PROG   // 指定起始符号
%expect 0     // 语法冲突的容忍度 (0 表示期望没有移进/归约冲突)


// 无结合性，不允许连续使用
/* %nonassoc ELSE
%nonassoc LOWER_THAN_ELSE */

// 优先级从低到高排列
%left ','                       // 逗号（用于 ExpList 和 FormalList）
%right '='                      // 赋值运算符
%left OR                        // 逻辑或 ||
%left AND                       // 逻辑与 &&
%left EQ NE                     // 相等性运算符 == !=
%left LT LE GT GE               // 关系运算符 < <= > >=
%left ADD MINUS                 // 加减运算符 + -
%left TIMES DIVIDE              // 乘除运算符 * /
%right UMINUS                   // 一元减号 - （优先级高于二元加减，但低于乘除）
%right NOT                      // 一元逻辑非 !
%left '.'                       // 成员访问 .
%left '[' ']'                   // 数组索引 []
%left '(' ')'                   // 括号（最高优先级）

%nonassoc IFX       // 无 else 的 if 语句
%nonassoc ELSE      // else 分支


%%

PROG: MAINMETHOD CLASSDECLLIST
  { 
#ifdef DEBUG
    cerr << "Program" << endl;
#endif
    result->root = new Program(p, $1, $2);
    $$ = result->root;
  }
  ;

MAINMETHOD: PUBLIC INT MAIN '(' ')' '{' VARDECLLIST STMLIST '}'
  {
#ifdef DEBUG
    cerr << "MainMethod" << endl;
#endif
    $$ = new MainMethod(p, $7, $8);
  }
  ;

VARDECLLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "VARDECLLIST empty" << endl;
#endif
    $$ = new vector<VarDecl*>();
  }
  | VARDECL VARDECLLIST
  {
#ifdef DEBUG
    cerr << "VARDECL VARDECLLIST" << endl;
#endif
    vector<VarDecl*> *v = $2;
    v->push_back($1);
    rotate(v->begin(), v->end() - 1, v->end());
    $$ = v;
  }
  ;

VARDECL: CLASS ID ID ';'
  {
#ifdef DEBUG
    cerr << "VarDecl: class id id" << endl;
#endif
    $$ = new VarDecl(p, new Type(p, $2), $3);
  }
  | INT ID ';'
  {
#ifdef DEBUG
    cerr << "VarDecl: int id" << endl;
#endif
    $$ = new VarDecl(p, new Type(p), $2);
  }
  | INT ID '=' CONST ';'
  {
#ifdef DEBUG
    cerr << "VarDecl: int id = const" << endl;
#endif
    $$ = new VarDecl(p, new Type(p), $2, $4);
  }
  | INT '[' ']' ID ';'
  {
#ifdef DEBUG
    cerr << "VarDecl: int[] id" << endl;
#endif
    $$ = new VarDecl(p, new Type(p, new IntExp(p, 0)), $4);
  }
  | INT '[' ']' ID '=' '{' CONSTLIST '}' ';'
  {
#ifdef DEBUG
    cerr << "VarDecl: int[] id = {constlist}" << endl;
#endif
    $$ = new VarDecl(p, new Type(p, new IntExp(p, 0)), $4, $7);
  }
  | INT '[' NONNEGATIVEINT ']' ID ';'
  {
#ifdef DEBUG
    cerr << "VarDecl: int[num] id" << endl;
#endif
    IntExp *num = new IntExp(p, $3);
    $$ = new VarDecl(p, new Type(p, num), $5);
  }
  | INT '[' NONNEGATIVEINT ']' ID '=' '{' CONSTLIST '}' ';'
  {
#ifdef DEBUG
    cerr << "VarDecl: int[num] id = {constlist}" << endl;
#endif
    $$ = new VarDecl(p, new Type(p, new IntExp(p, $3)), $5, $8);
  }
  ;

CONST: NONNEGATIVEINT
  {
#ifdef DEBUG
    cerr << "Const: positive"<< endl;
#endif
    $$ = new IntExp(p, $1);
  }
  | MINUS NONNEGATIVEINT
  {
#ifdef DEBUG
    cerr << "Const: negative"<< endl;
#endif
    $$ = new IntExp(p, -$2); // 注意这里使用 -$2
  }
  ;

CONSTLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "CONSTLIST empty" << endl;
#endif
    $$ = new vector<IntExp*>();
  }
  | CONST CONSTREST
  {
#ifdef DEBUG
    cerr << "CONSTLIST: const constrest" << endl;
#endif
    vector<IntExp*> *v = $2;
    v->push_back($1);
    rotate(v->begin(), v->end() - 1, v->end());
    $$ = v;
  }

CONSTREST: /* empty */
  {
#ifdef DEBUG
    cerr << "CONSTREST empty" << endl;
#endif
    $$ = new vector<IntExp*>();
  }
  | ',' CONST CONSTREST
  {
#ifdef DEBUG
    cerr << "CONSTREST: , const constrest" << endl;
#endif
    vector<IntExp*> *v = $3;
    v->push_back($2);
    rotate(v->begin(), v->end() - 1, v->end());
    $$ = v;
  }
  ;

STMLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "STMLIST empty" << endl;
#endif
    $$ = new vector<Stm*>();
  }
  | STM STMLIST
  {
#ifdef DEBUG
    cerr << "STM STMLIST" << endl;
#endif
    vector<Stm*> *v = $2;
    v->push_back($1);
    rotate(v->begin(), v->end() - 1, v->end());
    $$ = v;
  }
  ;

STM: '{' STMLIST '}'
  {
#ifdef DEBUG
    cerr << "STM:{STMLIST}" << endl;
#endif
  $$ = new Nested(p, $2);
  }
  | IF '(' EXP ')' STM %prec IFX
  {
#ifdef DEBUG
    cerr << "If" << endl;
#endif
    $$ = new If(p, $3, $5);
  }
  | IF '(' EXP ')' STM ELSE STM
  {
#ifdef DEBUG
    cerr << "IfElse" << endl;
#endif
    $$ = new If(p, $3, $5, $7);
  }
  | WHILE '(' EXP ')' STM
  {
#ifdef DEBUG
    cerr << "While" << endl;
#endif
    $$ = new While(p, $3, $5);
  }
  | WHILE '(' EXP ')' ';'
  {
#ifdef DEBUG
    cerr << "While empty" << endl;
#endif
    $$ = new While(p, $3);
  }
  | EXP '=' EXP ';'
  {
#ifdef DEBUG
    cerr << "Assign" << endl;
#endif
    $$ = new Assign(p, $1, $3);
  }
  | EXP '.' ID '(' EXPLIST ')' ';'
  {
#ifdef DEBUG
    cerr << "CallStm" << endl;
#endif
    $$ = new CallStm(p, $1, $3, $5);
  }
  | CONTINUE ';'
  {
#ifdef DEBUG
    cerr << "Continue" << endl;
#endif
    $$ = new Continue(p);
  }
  | BREAK ';'
  {
#ifdef DEBUG
    cerr << "Break" << endl;
#endif
    $$ = new Break(p);
  }
  | RETURN EXP ';'
  {
#ifdef DEBUG
    cerr << "Return" << endl;
#endif
    $$ = new Return(p, $2);
  }
  | PUTINT '(' EXP ')' ';'
  {
#ifdef DEBUG
    cerr << "PutInt" << endl;
#endif
    $$ = new PutInt(p, $3);
  }
  | PUTCH '(' EXP ')' ';'
  {
#ifdef DEBUG
    cerr << "PutCh" << endl;
#endif
    $$ = new PutCh(p, $3);
  }
  | PUTARRAY '(' EXP ',' EXP ')' ';'
  {
#ifdef DEBUG
    cerr << "PutArray" << endl;
#endif
    $$ = new PutArray(p, $3, $5);
  }
  | STARTTIME '(' ')' ';'
  {
#ifdef DEBUG
    cerr << "Starttime" << endl;
#endif
    $$ = new Starttime(p);
  }
  | STOPTIME '(' ')' ';'
  {
#ifdef DEBUG
    cerr << "Stoptime" << endl;
#endif
    $$ = new Stoptime(p);
  }
  ;


EXP: NONNEGATIVEINT
  {
#ifdef DEBUG
    cerr << "NonNegativeInt: " << $1 << endl;
#endif
    $$ = new IntExp(p, $1);
  }
  | TRUE
  {
#ifdef DEBUG
    cerr << "True" << endl;
#endif
    $$ = new BoolExp(p, true);
  }
  | FALSE
  {
#ifdef DEBUG
    cerr << "False" << endl;
#endif
    $$ = new BoolExp(p, false);
  }
  | LENGTH '(' EXP ')'
  {
#ifdef DEBUG
    cerr << "Length" << endl;
#endif
    $$ = new Length(p, $3);
  }
  | GETINT '(' ')'
  {
#ifdef DEBUG
    cerr << "GetInt" << endl;
#endif
    $$ = new GetInt(p);
  }
  | GETCH '(' ')'
  {
#ifdef DEBUG
    cerr << "GetCh" << endl;
#endif
    $$ = new GetCh(p);
  }
  | GETARRAY '(' EXP ')'
  {
#ifdef DEBUG
    cerr << "GetArray" << endl;
#endif
    $$ = new GetArray(p, $3);
  }
  | ID
  {
#ifdef DEBUG
    cerr << "ID" << endl;
#endif
    $$ = $1;
  }
  | THIS
  {
#ifdef DEBUG
    cerr << "This" << endl;
#endif
    $$ = new This(p);
  }
  | EXP ADD EXP
  {
#ifdef DEBUG
    cerr << "EXP ADD EXP" << endl;
#endif
    Pos *p1 = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
    $$ = new BinaryOp(p, $1, new OpExp(p1, "+"), $3);
  }
  | EXP MINUS EXP
  {
#ifdef DEBUG
    cerr << "EXP MINUS EXP" << endl;
#endif
    Pos *p1 = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
    $$ = new BinaryOp(p, $1, new OpExp(p1, "-"), $3);
  }
  | EXP TIMES EXP
  {
#ifdef DEBUG
    cerr << "EXP TIMES EXP" << endl;
#endif
    Pos *p1 = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
    $$ = new BinaryOp(p, $1, new OpExp(p1, "*"), $3);
  }
  | EXP DIVIDE EXP
  {
#ifdef DEBUG
    cerr << "EXP DIVIDE EXP" << endl;
#endif
    Pos *p1 = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
    $$ = new BinaryOp(p, $1, new OpExp(p1, "/"), $3);
  }
  | EXP EQ EXP
  {
#ifdef DEBUG
    cerr << "EXP EQ EXP" << endl;
#endif
    Pos *p1 = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
    $$ = new BinaryOp(p, $1, new OpExp(p1, "=="), $3);
  }
  | EXP NE EXP
  {
#ifdef DEBUG
    cerr << "EXP NE EXP" << endl;
#endif
    Pos *p1 = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
    $$ = new BinaryOp(p, $1, new OpExp(p1, "!="), $3);
  }
  | EXP LT EXP
  {
#ifdef DEBUG
    cerr << "EXP LT EXP" << endl;
#endif
    Pos *p1 = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
    $$ = new BinaryOp(p, $1, new OpExp(p1, "<"), $3);
  }
  | EXP LE EXP
  {
#ifdef DEBUG
    cerr << "EXP LE EXP" << endl;
#endif
    Pos *p1 = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
    $$ = new BinaryOp(p, $1, new OpExp(p1, "<="), $3);
  }
  | EXP GT EXP
  {
#ifdef DEBUG
    cerr << "EXP GT EXP" << endl;
#endif
    Pos *p1 = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
    $$ = new BinaryOp(p, $1, new OpExp(p1, ">"), $3);
  }
  | EXP GE EXP
  {
#ifdef DEBUG
    cerr << "EXP GE EXP" << endl;
#endif
    Pos *p1 = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
    $$ = new BinaryOp(p, $1, new OpExp(p1, ">="), $3);
  }
  | EXP AND EXP
  {
#ifdef DEBUG
    cerr << "EXP AND EXP" << endl;
#endif
    Pos *p1 = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
    $$ = new BinaryOp(p, $1, new OpExp(p1, "&&"), $3);
  }
  | EXP OR EXP
  {
#ifdef DEBUG
    cerr << "EXP OR EXP" << endl;
#endif
    Pos *p1 = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
    $$ = new BinaryOp(p, $1, new OpExp(p1, "||"), $3);
  }
  | NOT EXP
  {
#ifdef DEBUG
    cerr << "NOT EXP" << endl;
#endif
    Pos *p1 = new Pos(@1.sline, @1.scolumn, @1.eline, @1.ecolumn);
    $$ = new UnaryOp(p, new OpExp(p1, "!"), $2);
  }
  | MINUS EXP %prec UMINUS
  {
#ifdef DEBUG
    cerr << "- EXP" << endl;
#endif
    Pos *p1 = new Pos(@1.sline, @1.scolumn, @1.eline, @1.ecolumn);
    $$ = new UnaryOp(p, new OpExp(p1, "-"), $2);
  }
  | '(' EXP ')'
  {
#ifdef DEBUG
    cerr << "( EXP )" << endl;
#endif
    $$ = $2;
  }
  | '(' '{' STMLIST '}' EXP ')'
  {
#ifdef DEBUG
    cerr << "( { STMLIST } EXP )" << endl;
#endif
    $$ = new Esc(p, $3, $5);
  }
  | EXP '.' ID
  {
#ifdef DEBUG
    cerr << "EXP . ID" << endl;
#endif
    $$ = new ClassVar(p, $1, $3);
  }
  | EXP '.' ID '(' EXPLIST ')'
  {
#ifdef DEBUG
    cerr << "EXP . ID ( EXPLIST )" << endl;
#endif
    $$ = new CallExp(p, $1, $3, $5);
  }
  | EXP '[' EXP ']'
  {
#ifdef DEBUG
    cerr << "EXP [ EXP ]" << endl;
#endif
    $$ = new ArrayExp(p, $1, $3);
  }
  ;


EXPLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "EXPLIST empty" << endl;
#endif
    $$ = new vector<Exp*>();
  }
  | EXP EXPREST
  {
#ifdef DEBUG
    cerr << "EXP EXPREST" << endl;
#endif
    vector<Exp*> *v = $2;
    v->push_back($1);
    rotate(v->begin(), v->end() - 1, v->end());
    $$ = v;
  }
  ;


EXPREST: /* empty */
  {
#ifdef DEBUG
    cerr << "EXPREST empty" << endl;
#endif
    $$ = new vector<Exp*>();
  }
  | ',' EXP EXPREST
  {
#ifdef DEBUG
    cerr << ", EXP EXPREST" << endl;
#endif
    vector<Exp*> *v = $3;
    v->push_back($2);
    rotate(v->begin(), v->end() - 1, v->end());
    $$ = v;
  }
  ;


CLASSDECLLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "CLASSDECLLIST empty" << endl;
#endif
    $$ = new vector<ClassDecl*>();
  }
  | CLASSDECL CLASSDECLLIST
  {
#ifdef DEBUG
    cerr << "CLASSDECL CLASSDECLLIST" << endl;
#endif
    vector<ClassDecl*> *v = $2;
    v->push_back($1);
    rotate(v->begin(), v->end() - 1, v->end());
    $$ = v;
  }
  ;

CLASSDECL: PUBLIC CLASS ID '{' VARDECLLIST METHODDECLLIST '}'
  {
#ifdef DEBUG
    cerr << "ClassDecl" << endl;
#endif
    $$ = new ClassDecl(p, $3, $5, $6);
  }
  | PUBLIC CLASS ID EXTENDS ID '{' VARDECLLIST METHODDECLLIST '}'
  {
#ifdef DEBUG
    cerr << "ClassDecl with extends" << endl;
#endif
    $$ = new ClassDecl(p, $3, $5, $7, $8);
  }
  ;

METHODDECLLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "METHODDECLLIST empty" << endl;
#endif
    $$ = new vector<MethodDecl*>();
  }
  | METHODDECL METHODDECLLIST
  {
#ifdef DEBUG
    cerr << "METHODDECL METHODDECLLIST" << endl;
#endif
    vector<MethodDecl*> *v = $2;
    v->push_back($1);
    rotate(v->begin(), v->end() - 1, v->end());
    $$ = v;
  }
  ;

METHODDECL: PUBLIC TYPE ID '(' FORMALLIST ')' '{' VARDECLLIST STMLIST '}'
  {
#ifdef DEBUG
    cerr << "MethodDecl" << endl;
#endif
    $$ = new MethodDecl(p, $2, $3, $5, $8, $9);
  }
  ;


TYPE: CLASS ID
  {
#ifdef DEBUG
    cerr << "Type: class id" << endl;
#endif
    $$ = new Type(p, $2);
  }
  | INT
  {
#ifdef DEBUG
    cerr << "Type: int" << endl;
#endif
    $$ = new Type(p);
  }
  | INT '[' ']'
  {
#ifdef DEBUG
    cerr << "Type: int[]" << endl;
#endif
    $$ = new Type(p, new IntExp(p, 0));
  }
  ;


FORMALLIST: /* empty */
  {
#ifdef DEBUG
    cerr << "FORMALLIST empty" << endl;
#endif
    $$ = new vector<Formal*>();
  }
  | TYPE ID FORMALREST
  {
#ifdef DEBUG
    cerr << "TYPE ID FORMALREST" << endl;
#endif
    vector<Formal*> *v = $3;
    v->push_back(new Formal(p, $1, $2));
    rotate(v->begin(), v->end() - 1, v->end());
    $$ = v;
  }
  ;

FORMALREST: /* empty */
  {
#ifdef DEBUG
    cerr << "FORMALREST empty" << endl;
#endif
    $$ = new vector<Formal*>();
  }
  | ',' TYPE ID FORMALREST
  {
#ifdef DEBUG
    cerr << ", TYPE ID FORMALREST" << endl;
#endif
    vector<Formal*> *v = $4;
    v->push_back(new Formal(p, $2, $3));
    rotate(v->begin(), v->end() - 1, v->end());
    $$ = v;
  }
  ;

ID: IDENTIFIER
  {
#ifdef DEBUG
    cerr << "Identifier: " << $1 << endl;
#endif
    $$ = new IdExp(p, $1);
  }
  ;




%%
/*
void yyerror(char *s) {
  fprintf(stderr, "%s\n",s);
}

int yywrap() {
  return(1);
}
*/

//%code 
namespace fdmj 
{
    template<typename RHS>
    inline void calcLocation(location_t &current, const RHS &rhs, const std::size_t n)
    {
        current = location_t(YYRHSLOC(rhs, 1).sline, YYRHSLOC(rhs, 1).scolumn, 
                                    YYRHSLOC(rhs, n).eline, YYRHSLOC(rhs, n).ecolumn);
        p = new Pos(current.sline, current.scolumn, current.eline, current.ecolumn);
    }
    
    void ASTParser::error(const location_t &location, const std::string &message)
    {
        std::cerr << "Error at lines " << location << ": " << message << std::endl;
    }

  Program* fdmjParser(ifstream &fp, const bool debug) {
    fdmj:AST_YYSTYPE result; 
    result.root = nullptr;
    fdmj::ASTLexer lexer(fp, debug);
    fdmj::ASTParser parser(lexer, debug, &result); //set up the parser
    if (parser() ) { //call the parser
      cout << "Error: parsing failed" << endl;
      return nullptr;
    }
    if (debug) cout << "Parsing successful" << endl;
    return result.root;
  }

  Program*  fdmjParser(const string &filename, const bool debug) {
    std::ifstream fp(filename);
    if (!fp) {
      cout << "Error: cannot open file " << filename << endl;
      return nullptr;
    }
    return fdmjParser(fp, debug);
  }
}
