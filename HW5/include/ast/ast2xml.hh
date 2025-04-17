#ifndef _AST2XML_HH
//#define _AST2XML_HH

#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "tinyxml2.hh"
#include "semant.hh"

using namespace std;
using namespace fdmj;
using namespace tinyxml2;

XMLDocument* ast2xml(Program *node, AST_Semant_Map *semant_map, bool location_flag, bool semant_flag);

class AST2XML : public fdmj::AST_Visitor {
public:
  XMLDocument *doc; //XMLDocument to store the AST
  XMLElement *el; //temp to remember the results during the AST is recursively visited. 
  AST_Semant_Map *semant_map; //store the semantic information of the nodes

public:
  void visit(Program *node) override;
  void visit(MainMethod *node) override;
  void visit(ClassDecl *node) override;
  void visit(Type *node) override;
  void visit(VarDecl *node) override;
  void visit(MethodDecl *node) override;
  void visit(Formal *node) override;
  void visit(Nested *node) override;
  void visit(If *node) override;
  void visit(While *node) override;
  void visit(Assign *node) override;
  void visit(CallStm *node) override;
  void visit(Continue *node) override;
  void visit(Break *node) override;
  void visit(Return *node) override;
  void visit(PutInt *node) override;
  void visit(PutCh *node) override;
  void visit(PutArray *node) override;
  void visit(Starttime *node) override;
  void visit(Stoptime *node) override;
  void visit(BinaryOp *node) override;
  void visit(UnaryOp *node) override;
  void visit(ArrayExp *node) override;
  void visit(CallExp *node) override;
  void visit(ClassVar *node) override;
  void visit(BoolExp *node) override;
  void visit(This *node) override;
  void visit(Length *node) override;
  void visit(Esc *node) override;
  void visit(GetInt *node) override;
  void visit(GetCh *node) override;
  void visit(GetArray *node) override;
  void visit(IdExp *node) override;
  void visit(OpExp *node) override;
  void visit(IntExp *node) override;
};

#endif