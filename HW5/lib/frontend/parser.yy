%skeleton "lalr1.cc"
%require  "3.2"
%language "c++"
%locations

%code requires {
    // #define DEBUG
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

%define api.namespace     {fdmj}         // 命名空间
%define api.parser.class  {ASTParser}    // parser类名
%define api.value.type    {AST_YYSTYPE}  // 语义值类型
%define api.location.type {ast_location} // 位置类型

%define parse.error detailed
%define parse.trace

%header
%verbose

// ASTParser 构造函数参数
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
        template<typename RHS>
        void calcLocation(location_t &current, const RHS &rhs, const std::size_t n);
    }
    #define YYLLOC_DEFAULT(Cur, Rhs, N) calcLocation(Cur, Rhs, N)
    #define yylex lexer.yylex
    Pos *pos;
}



%token TRUE FALSE
%token INT CLASS PUBLIC EXTENDS THIS
%token IF ELSE WHILE CONTINUE BREAK MAIN RETURN
%token PUTINT PUTCH PUTARRAY STARTTIME STOPTIME
%token GETINT GETCH GETARRAY LENGTH

%token ADD MINUS TIMES DIVIDE                   // 算数运算
%token AND OR NOT                               // 逻辑运算
%token EQ NE LE LT GE GT                        // 比较运算
%token '(' ')' '[' ']' '{' '}' '=' ',' ';' '.'  // 其他符号

// 优先级从低到高
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE


%left OR
%left AND
%right NOT
%nonassoc EQ NE LE LT GE GT
%left ADD MINUS
%left TIMES DIVIDE
%right UMINUS
%nonassoc '.'
%nonassoc '[' ']'

%token<i> NUM
%token<s> NAME

%type <intExp> CONST
%type <intExpList> CONSTLIST
%type <intExpList> CONSTREST
%type <idExp> ID

%type <type> TYPE
%type <program> PROG
%type <mainMethod> MAINMETHOD

%type <classDecl> CLASSDECL
%type <classDeclList> CLASSDECLLIST

%type <varDecl> VARDECL
%type <varDeclList> VARDECLLIST

%type <methodDecl> METHODDECL
%type <methodDeclList> METHODDECLLIST

%type <formalList> FORMALLIST
%type <formalList> FORMALREST

%type <stm> STM
%type <stmList> STMLIST

%type <exp> EXP
%type <expList> EXPLIST
%type <expList> EXPREST


%start PROG
%expect 0

%%
// 标识符: NAME
ID: NAME { $$ = new IdExp(pos, $1); }
    ;

// 数组初始化-常量: 整数 | 负整数
// Const: NUM | '-' NUM
CONST: NUM { $$ = new IntExp(pos, $1); }
    |
    MINUS NUM { $$ = new IntExp(pos, -$2); }
    ;

// 数组初始化-常量列表
CONSTLIST: // empty
    { $$= new IntExpList(); }
    | CONSTREST ',' CONST
    { $1->push_back($3); $$ = $1; }
    | CONST
    {
        IntExpList* constList = new IntExpList();
        constList->push_back($1);
        $$ = constList;
    }
    ;

// 数组初始化-常量列表余表
CONSTREST: CONSTREST ',' CONST
    { $1->push_back($3); $$ = $1; }
    | CONST
    {
        IntExpList* constList = new IntExpList();
        constList->push_back($1);
        $$ = constList;
    }
    ;



// 程序: 主方法 类声明列表
// PROG: MAINMETHOD CLASSDECLLIST
PROG: MAINMETHOD CLASSDECLLIST
    { result->root = new Program(pos, $1, $2); }
    ;


// 主方法: public int main() { 变量声明列表 语句列表 }
// MAINMETHOD: PUBLIC INT MAIN '(' ')' '{' VARDECLLIST STMLIST '}';
MAINMETHOD: PUBLIC INT MAIN '(' ')' '{' VARDECLLIST STMLIST '}'
    { $$ = new MainMethod(pos, $7, $8); }
    ;





// 类声明列表
CLASSDECLLIST: // empty
    { $$ = new ClassDeclList(); }
    |
    CLASSDECLLIST CLASSDECL
    { $1->push_back($2); $$ = $1; }
    ;


// 类声明: 类名 [基类名] { 变量声明列表 方法声明列表 }
// CLASSDECL: PUBLIC CLASS ID '{' VARDECLLIST METHODDECLLIST '}'
//          | PUBLIC CLASS ID EXTENDS ID '{' VARDECLLIST METHODDECLLIST '}'
CLASSDECL: PUBLIC CLASS ID '{' VARDECLLIST METHODDECLLIST '}'
    { $$ = new ClassDecl(pos, $3, $5, $6); }
    |
    PUBLIC CLASS ID EXTENDS ID '{' VARDECLLIST METHODDECLLIST '}'
    { $$ = new ClassDecl(pos, $3, $5, $7, $8); }
    ;





// 类型:  整型 | 整型数组 | 类
// TYPE: INT | INT '[' ']' | CLASS ID
TYPE: INT
    { $$ = new Type(pos); }
    |
    INT '[' ']'
    {
        IntExp* arity = new IntExp(pos, 0);
        $$ = new Type(pos, arity);
    }
    |
    CLASS ID
    { $$ = new Type(pos, $2); }
    ;





// 变量声明列表
VARDECLLIST: // empty
    { $$ = new VarDeclList(); }
    |
    VARDECLLIST VARDECL
    { $1->push_back($2); $$ = $1; }
    ;

// 变量声明
// VARDECL: CLASS ID ID ';'
//        | INT ID ';'
//        | INT '[' ']' ID ';'
//        | INT '[' NUM ']' ID ';'
//        | INT ID '=' CONST ';'
//        | INT '[' ']' ID '=' '{' CONSTLIST '}' ';'
//        | INT '[' NUM ']' ID '=' '{' CONSTLIST '}' ';'
VARDECL: CLASS ID ID ';'
    {
        IdExp* cid = $2;
        IdExp* id = $3;
        Type* type = new Type(pos, cid); // 类
        $$ = new VarDecl(pos, type, id); // 无初始化
    }
    |
    INT ID ';'
    {
        IdExp* id = $2;
        Type* type = new Type(pos);      // 整型
        $$ = new VarDecl(pos, type, id); // 无初始化
    }
    |
    INT '[' ']' ID ';'
    {
        IdExp* id = $4;
        IntExp* arity = new IntExp(pos, 0);
        Type* type = new Type(pos, arity); // 整型数组
        $$ = new VarDecl(pos, type, id);   // 无初始化
    }
    |
    INT '[' NUM ']' ID ';'
    {
        IdExp* id = $5;
        IntExp* arity = new IntExp(pos, $3);
        Type* type = new Type(pos, arity); // 整型数组
        $$ = new VarDecl(pos, type, id);   // 无初始化
    }
    |
    INT ID '=' CONST ';'
    {
        IdExp* id = $2;
        IntExp* init_int = $4;
        Type* type = new Type(pos);                // 整型
        $$ = new VarDecl(pos, type, id, init_int); // 整型初始化
    }
    |
    INT '[' ']' ID '=' '{' CONSTLIST '}' ';'
    {
        IdExp* id = $4;
        IntExpList* init_array = $7;
        IntExp* arity = new IntExp(pos, 0);
        Type* type = new Type(pos, arity);           // 整型数组
        $$ = new VarDecl(pos, type, id, init_array); // 整型数组初始化
    }
    |
    INT '[' NUM ']' ID '=' '{' CONSTLIST '}' ';'
    {
        IdExp* id = $5;
        IntExpList* init_array = $8;
        IntExp* arity = new IntExp(pos, $3);
        Type* type = new Type(pos, arity);           // 整型数组
        $$ = new VarDecl(pos, type, id, init_array); // 整型数组初始化
    }
    ;





// 方法声明列表
METHODDECLLIST: // empty
    { $$ = new MethodDeclList(); }
    |
    METHODDECLLIST METHODDECL
    { $1->push_back($2); $$ = $1; }
    ;

// 方法声明: 返回类型 方法名(形参列表) { 变量声明列表 语句列表 }
// METHODDECL: PUBLIC TYPE ID '(' FORMALLIST ')' '{' VARDECLLIST STMLIST '}'
METHODDECL: PUBLIC TYPE ID '(' FORMALLIST ')' '{' VARDECLLIST STMLIST '}'
    { $$ = new MethodDecl(pos, $2, $3, $5, $8, $9); }
    ;




// 方法形参列表
FORMALLIST: // empty
    { $$ = new vector<Formal*>(); }
    |
    FORMALREST ',' TYPE ID 
    { $1->push_back(new Formal(pos, $3, $4)); $$ = $1; }
    |
    TYPE ID
    {
        vector<Formal*>* formalList = new vector<Formal*>();
        formalList->push_back(new Formal(pos, $1, $2));
        $$ = formalList;
    }
    ;

// 方法形参余表
FORMALREST: FORMALREST ',' TYPE ID 
    { $1->push_back(new Formal(pos, $3, $4)); $$ = $1; }
    |
    TYPE ID
    {
        vector<Formal*>* formalList = new vector<Formal*>();
        formalList->push_back(new Formal(pos, $1, $2));
        $$ = formalList;
    }
    ;






// 语句列表
STMLIST: // empty
    { $$ = new vector<Stm*>(); }
    |
    STMLIST STM
    { $1->push_back($2); $$ = $1; }

// 语句
// STM: '{' STMLIST '}'
//      | IF '(' EXP ')' STM ELSE STM
//      | IF '(' EXP ')' STM
//      | WHILE '(' EXP ')' STM
//      | WHILE '(' EXP ')' ';'
//      | EXP '=' EXP ';'
//      | EXP '.' ID '(' EXPLIST ')' ';'
//      | CONTINUE ';'
//      | BREAK ';'
//      | RETURN EXP ';'
//      | PUTINT '(' EXP ')' ';'
//      | PUTCH '(' EXP ')' ';'
//      | PUTARRAY '(' EXP ',' EXP ')' ';'
//      | STARTTIME '(' ')' ';'
//      | STOPTIME '(' ')' ';'
STM: '{' STMLIST '}'
    {
        vector<Stm*>* stmList = $2;
        $$ = new Nested(pos, stmList);
    }
    |
    IF '(' EXP ')' STM ELSE STM
    {
        Exp* exp = $3;
        Stm* stm1 = $5;
        Stm* stm2 = $7;
        $$ = new If(pos, exp, stm1, stm2);
    }
    |
    IF '(' EXP ')' STM %prec LOWER_THAN_ELSE
    {
        Exp* exp = $3;
        Stm* stm1 = $5;
        $$ = new If(pos, exp, stm1);
    }
    |
    WHILE '(' EXP ')' STM
    {
        Exp* exp = $3;
        Stm* stm = $5;
        $$ = new While(pos, exp, stm);
    }
    |
    WHILE '(' EXP ')' ';'
    {
        Exp* exp = $3;
        $$ = new While(pos, exp);
    }
    |
    EXP '=' EXP ';'
    {
        Exp* left = $1;
        Exp* right = $3;
        $$ = new Assign(pos, left, right);
    }
    |
    EXP '.' ID '(' EXPLIST ')' ';'
    {
        Exp* obj = $1;
        IdExp* method = $3;
        vector<Exp*>* param = $5;
        $$ = new CallStm(pos, obj, method, param);
    }
    |
    CONTINUE ';' { $$ = new Continue(pos); }
    |
    BREAK ';' { $$ = new Break(pos); }
    |
    RETURN EXP ';'
    {
        Exp* exp = $2;
        $$ = new Return(pos, exp);
    }
    |
    PUTINT '(' EXP ')' ';'
    {
        Exp* exp = $3;
        $$ = new PutInt(pos, exp);
    }
    |
    PUTCH '(' EXP ')' ';'
    {
        Exp* exp = $3;
        $$ = new PutCh(pos, exp);
    }
    |
    PUTARRAY '(' EXP ',' EXP ')' ';'
    {
        Exp* n = $3;
        Exp* arr = $5;
        $$ = new PutArray(pos, n, arr);
    }
    |
    STARTTIME '(' ')' ';' { $$ = new Starttime(pos); }
    |
    STOPTIME '(' ')' ';' { $$ = new Stoptime(pos); }
    ;




// 表达式列表: ε | 表达式 表达式余表
// EXPLIST: ε | EXP EXPREST
EXPLIST: // empty
    { $$ = new ExpList(); }
    |
    EXPREST ',' EXP 
    { $1->push_back($3); $$ = $1; }
    |
    EXP
    {
        ExpList* expList = new ExpList();
        expList->push_back($1);
        $$ = expList;
    }
    ;

// 表达式余表
EXPREST: EXPREST ',' EXP
    { $1->push_back($3); $$ = $1; }
    |
    EXP
    {
        ExpList* expList = new ExpList();
        expList->push_back($1);
        $$ = expList;
    }
    ;

// 表达式
// EXP -> '(' EXP ')'
//      | '(' '{' STMLIST '}' EXP ')'
//      | ID
//      | NUM
//      | TRUE | FALSE
//      | EXP '[' EXP ']'
//      | OP
//      | EXP [+-*/ COMP && ||] EXP
//      | [-!] EXP
//      | THIS
//      | EXP '.' ID '(' EXPLIST ')'
//      | EXP '.' ID
//      | GETINT '(' ')'
//      | GETCH '(' ')'
//      | GETARRAY '(' EXP ')'
//      | LENGTH '(' EXP ')'
EXP: '(' EXP ')' { $$ = $2; }
    |
    '(' '{' STMLIST '}' EXP ')'
    {
        vector<Stm*>* stmList = $3;
        Exp* exp = $5;
        $$ = new Esc(pos, stmList, exp);
    }
    |
    ID { $$ = $1; }
    |
    NUM { $$ = new IntExp(pos, $1); }
    |
    TRUE { $$ = new BoolExp(pos, true); }
    |
    FALSE { $$ = new BoolExp(pos, false); }
    |
    EXP '[' EXP ']'
    {
        Exp* arr = $1;
        Exp* index = $3;
        $$ = new ArrayExp(pos, arr, index);
    }
    |
    EXP ADD EXP
    {
        Pos* opPos = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
        OpExp* op = new OpExp(opPos, "+");
        $$ = new BinaryOp(pos, $1, op, $3);
    }
    |
    EXP MINUS EXP
    {
        Pos* opPos = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
        OpExp* op = new OpExp(opPos, "-");
        $$ = new BinaryOp(pos, $1, op, $3);
    }
    |
    EXP TIMES EXP
    {
        Pos* opPos = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
        OpExp* op = new OpExp(opPos, "*");
        $$ = new BinaryOp(pos, $1, op, $3);
    }
    |
    EXP DIVIDE EXP
    {
        Pos* opPos = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
        OpExp* op = new OpExp(opPos, "/");
        $$ = new BinaryOp(pos, $1, op, $3);
    }
    |
    EXP AND EXP
    {
        Pos* opPos = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
        OpExp* op = new OpExp(opPos, "&&");
        $$ = new BinaryOp(pos, $1, op, $3);
    }
    |
    EXP OR EXP
    {
        Pos* opPos = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
        OpExp* op = new OpExp(opPos, "||");
        $$ = new BinaryOp(pos, $1, op, $3);
    }
    |
    EXP EQ EXP
    {
        Pos* opPos = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
        OpExp* op = new OpExp(opPos, "==");
        $$ = new BinaryOp(pos, $1, op, $3);
    }
    |
    EXP NE EXP
    {
        Pos* opPos = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
        OpExp* op = new OpExp(opPos, "!=");
        $$ = new BinaryOp(pos, $1, op, $3);
    }
    |
    EXP LE EXP
    {
        Pos* opPos = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
        OpExp* op = new OpExp(opPos, "<=");
        $$ = new BinaryOp(pos, $1, op, $3);
    }
    |
    EXP LT EXP
    {
        Pos* opPos = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
        OpExp* op = new OpExp(opPos, "<");
        $$ = new BinaryOp(pos, $1, op, $3);
    }
    |
    EXP GE EXP
    {
        Pos* opPos = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
        OpExp* op = new OpExp(opPos, ">=");
        $$ = new BinaryOp(pos, $1, op, $3);
    }
    |
    EXP GT EXP
    {
        Pos* opPos = new Pos(@2.sline, @2.scolumn, @2.eline, @2.ecolumn);
        OpExp* op = new OpExp(opPos, ">");
        $$ = new BinaryOp(pos, $1, op, $3);
    }
    |
    MINUS EXP %prec UMINUS
    {
        Pos* opPos = new Pos(@1.sline, @1.scolumn, @1.eline, @1.ecolumn);
        OpExp* op = new OpExp(opPos, "-");
        $$ = new UnaryOp(pos, op, $2);
    }
    |
    NOT EXP
    {
        Pos* opPos = new Pos(@1.sline, @1.scolumn, @1.eline, @1.ecolumn);
        OpExp* op = new OpExp(opPos, "!");
        $$ = new UnaryOp(pos, op, $2);
    }
    |
    THIS { $$ = new This(pos); }
    |
    EXP '.' ID '(' EXPLIST ')'
    {
        Exp* obj = $1;
        IdExp* method = $3;
        vector<Exp*>* param = $5;
        $$ = new CallExp(pos, obj, method, param);
    }
    |
    EXP '.' ID
    {
        Exp* obj = $1;
        IdExp* id = $3;
        $$ = new ClassVar(pos, obj, id);
    }
    |
    GETINT '(' ')' { $$ = new GetInt(pos); }
    |
    GETCH '(' ')' { $$ = new GetCh(pos); }
    |
    GETARRAY '(' EXP ')' { $$ = new GetArray(pos, $3); }
    |
    LENGTH '(' EXP ')' { $$ = new Length(pos, $3); }
    ;

%%


namespace fdmj
{
    template<typename RHS>
    inline void calcLocation(location_t &current, const RHS &rhs, const std::size_t n)
    {
        current = location_t(YYRHSLOC(rhs, 1).sline, 
                             YYRHSLOC(rhs, 1).scolumn, 
                             YYRHSLOC(rhs, n).eline, 
                             YYRHSLOC(rhs, n).ecolumn);
        pos = new Pos(current.sline, current.scolumn, current.eline, current.ecolumn);
    }
    
    void ASTParser::error(const location_t &location, const std::string &message)
    {
        std::cerr << "ASTParser: Error at " << location << ": " << message << std::endl;
    }

    // 外部接口函数
    Program* fdmjParser(ifstream &fp, const bool debug) {
        fdmj:AST_YYSTYPE result; 
        result.root = nullptr;

        fdmj::ASTLexer lexer(fp, debug);
        fdmj::ASTParser parser(lexer, debug, &result);
        if (parser()) {
            cout << "Error: parsing failed" << endl;
            return nullptr;
        }

        if (debug) cout << "Parsing successful" << endl;
        return result.root;
    }

    // 外部接口函数
    Program* fdmjParser(const string &filename, const bool debug) {
        std::ifstream fp(filename);
        if (!fp) {
            cout << "Error: cannot open file " << filename << endl;
            return nullptr;
        }
        return fdmjParser(fp, debug);
    }
}
