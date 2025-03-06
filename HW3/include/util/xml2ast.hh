#ifndef _XML2AST_HH
#define _XML2AST_HH

#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "tinyxml2.hh"

using namespace std;
using namespace fdmj;
using namespace tinyxml2;

//create a node from an XML node, which must match the given node type
//The initial call should be with the root non-terminal (usually Program) and the root of the XML tree (usually tag: "Program")
Program* xml2ast(string xmlfilename);
Program* xml2ast(XMLElement* element);

//forward declarations
Program* create_program(XMLElement*);
MainMethod* create_mainMethod(XMLElement*);
VarDecl* create_varDecl(XMLElement*);
MethodDecl* create_methodDecl(XMLElement*);
ClassDecl* create_classDecl(XMLElement*);
Type* create_type(XMLElement*);
Formal* create_formal(XMLElement*);
Stm* create_stm(XMLElement*);
Nested* create_nested(XMLElement*);
If* create_if(XMLElement*);
While* create_while(XMLElement*);
Assign* create_assign(XMLElement*);
CallStm* create_callStm(XMLElement*);
Continue* create_continue(XMLElement*);
Break* create_break(XMLElement*);
Return* create_return(XMLElement*);
PutInt* create_putInt(XMLElement*);
PutCh* create_putCh(XMLElement*);
PutArray* create_putArray(XMLElement*);
Starttime* create_starttime(XMLElement*);
Stoptime* create_stoptime(XMLElement*);
Exp* create_exp(XMLElement*);
BinaryOp* create_binaryOp(XMLElement*);
UnaryOp* create_unaryOp(XMLElement*);
ArrayExp* create_arrayExp(XMLElement*);
CallExp* create_callExp(XMLElement*);
ClassVar* create_classVar(XMLElement*);
BoolExp* create_bool(XMLElement*);
This* create_this(XMLElement*);
Length* create_length(XMLElement*);
Esc* create_esc(XMLElement*);
GetInt* create_getInt(XMLElement*);
GetCh* create_getCh(XMLElement*);
GetArray* create_getArray(XMLElement*);
IdExp* create_idExp(XMLElement*);
IntExp* create_intexp(XMLElement*);
OpExp* create_opexp(XMLElement*);
#endif