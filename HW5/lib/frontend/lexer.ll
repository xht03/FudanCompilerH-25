
%{
    #include <string>
    #include <variant>
    #include "ASTheader.hh"
    #include "FDMJAST.hh"
    #include "ast_location.hh"
    #include "parser.tab.hh"

    using namespace std;
    using namespace fdmj;

    // 定义yylex接口
    #undef  YY_DECL
    #define YY_DECL int ASTLexer::yylex(AST_YYSTYPE* const lval, location_t *const lloc)

    // 初始化参数
    #define YY_USER_INIT yylval = lval; yylloc = lloc;

    // 发生匹配后 拷贝位置信息
    #define YY_USER_ACTION copyLocation();
%}

%option c++ noyywrap debug
%option yyclass="ASTLexer"
%option prefix="yy_ast_"

%s COMMENT1 COMMENT2

num                 [1-9][0-9]*|0
name                [a-z_A-Z][a-z_A-Z0-9]*
punct               [()\[\]{}=,;.]



%%
%{
    using Token = ASTParser::token;
%}

<INITIAL>"//" { BEGIN COMMENT1; }
<INITIAL>"/*" { BEGIN COMMENT2; }

<INITIAL>" "|\t { }
<INITIAL>\r { currentLine++; currentColumn = 1; }
<INITIAL>\n { currentLine++; currentColumn = 1; }

<INITIAL>"true"      { return Token::TRUE; }
<INITIAL>"false"     { return Token::FALSE; }

<INITIAL>"int"       { return Token::INT; }
<INITIAL>"class"     { return Token::CLASS; }
<INITIAL>"public"    { return Token::PUBLIC; }
<INITIAL>"extends"   { return Token::EXTENDS; }
<INITIAL>"this"      { return Token::THIS; }

<INITIAL>"if"        { return Token::IF; }
<INITIAL>"else"      { return Token::ELSE; }
<INITIAL>"while"     { return Token::WHILE; }
<INITIAL>"continue"  { return Token::CONTINUE; }
<INITIAL>"break"     { return Token::BREAK; }
<INITIAL>"main"      { return Token::MAIN; }
<INITIAL>"return"    { return Token::RETURN; }

<INITIAL>"putint"    { return Token::PUTINT; }
<INITIAL>"putch"     { return Token::PUTCH; }
<INITIAL>"putarray"  { return Token::PUTARRAY; }
<INITIAL>"starttime" { return Token::STARTTIME; }
<INITIAL>"stoptime"  { return Token::STOPTIME; }

<INITIAL>"getint"    { return Token::GETINT; }
<INITIAL>"getch"     { return Token::GETCH; }
<INITIAL>"getarray"  { return Token::GETARRAY; }
<INITIAL>"length"    { return Token::LENGTH; }




<INITIAL>"+"  { return Token::ADD; }
<INITIAL>"-"  { return Token::MINUS; }
<INITIAL>"*"  { return Token::TIMES; }
<INITIAL>"/"  { return Token::DIVIDE; }
<INITIAL>"==" { return Token::EQ; }
<INITIAL>"!=" { return Token::NE; }
<INITIAL>"<=" { return Token::LE; }
<INITIAL>"<"  { return Token::LT; }
<INITIAL>">=" { return Token::GE; }
<INITIAL>">"  { return Token::GT; }
<INITIAL>"&&" { return Token::AND; }
<INITIAL>"||" { return Token::OR; }
<INITIAL>"!"  { return Token::NOT; }

<INITIAL>{punct} { return yytext[0];}

<INITIAL>{num} { copyValue(std::atoi(yytext)); return Token::NUM; }
<INITIAL>{name}           { copyValue(yytext);            return Token::NAME; }

<INITIAL>. { cerr << "Illegal input " << yytext[0] << endl; return 0; }

<COMMENT1>\n { currentLine++; currentColumn = 1; BEGIN INITIAL; }
<COMMENT1>. { }

<COMMENT2>"*/" { BEGIN INITIAL; }
<COMMENT2>\n { currentLine++; currentColumn = 1; }
<COMMENT2>. { }
%%

int yywrap() {
    return 1;
}
