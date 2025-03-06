#ifndef __AST_SEMANT_HH__
#define __AST_SEMANT_HH__

#include <map>
#include <set>
#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "namemaps.hh"

using namespace std;
using namespace fdmj;

//this is for denote the semantic information of an AST node
//mostly for expressions and variables.
//Useful for type checking and IR (tree) generation
class AST_Semant {
public:
    enum Kind {Value, MethodName, ClassName};
    //Value - a value, has a typeKind (any calculation result)
    //MethodName - a method name, has a typeKind (need: class id)
    //ClassName - a class name, has a typeKind (need class id)
    //ClassVarName - a class variable name (type determined by class)
private:
    Kind s_kind;
    TypeKind typeKind; //enum class TypeKind {CLASS/OBJECT = 0, INT = 1, ARRAY = 2};
    variant<monostate, string, int> type_par; //string for class name, int for array arity
    bool lvalue; //if the expression is an lvalue
public:
    AST_Semant(AST_Semant::Kind s_kind, TypeKind typeKind, variant<monostate, string, int> type_par, bool lvalue) :
            s_kind(s_kind), typeKind(typeKind), type_par(type_par), lvalue(lvalue) {}
    Kind get_kind() { return s_kind; }
    TypeKind get_type() { return typeKind; }
    variant<monostate, string, int> get_type_par() { return type_par; }
    bool is_lvalue() { return lvalue; }
    static string s_kind_string(Kind s_kind) {
        switch (s_kind) {
            case AST_Semant::Kind::Value:
                return "Value";
            case AST_Semant::Kind::MethodName:
                return "MethodName";
            case AST_Semant::Kind::ClassName:
                return "ClassName";
            default:
                return "Unknown";
        }
    }
};

//this is to map an AST node to its semantic information
class AST_Semant_Map {
private:
    Name_Maps *name_maps;
    map<AST*, AST_Semant*> semant_map;
public:
    AST_Semant_Map() {
        semant_map = map<AST*, AST_Semant*>();
    }
    ~AST_Semant_Map() {
        semant_map.clear();
    }
    AST_Semant* getSemant(AST *node) {
        if (node == nullptr) {
            return nullptr;
        } 
        if (semant_map.find(node) == semant_map.end()) {
            return nullptr;
        }
        return semant_map[node];
    }
    void setSemant(AST *node, AST_Semant* semant) {
        if (node == nullptr) {
            cerr << "Error: setting semantic information for a null node" << endl;
            return;
        }
        semant_map[node] = semant;
    }
};

class AST_Semant_Visitor : public AST_Visitor {
private:
    AST_Semant_Map *semant_map; //this is the semantic information map for all (sub)expressions
    Name_Maps* const name_maps; //this is the map for all names in the program
    //you may add other members here 
public:
    //Change this constructor if more members are added above (if necessary)
    AST_Semant_Visitor(Name_Maps* name_maps) : name_maps(name_maps) {
        semant_map = new AST_Semant_Map();
    }
    AST_Semant_Map* getSemantMap() { return semant_map; }

    void visit(Program* node) override;
    void visit(MainMethod* node) override;
    void visit(ClassDecl* node) override;
    void visit(Type *node) override;
    void visit(VarDecl* node) override;
    void visit(MethodDecl* node) override;
    void visit(Formal* node) override;
    void visit(Nested* node) override;
    void visit(If* node) override;
    void visit(While* node) override;
    void visit(Assign* node) override;
    void visit(CallStm* node) override;
    void visit(Continue* node) override;
    void visit(Break* node) override;
    void visit(Return* node) override;
    void visit(PutInt* node) override;
    void visit(PutCh* node) override;
    void visit(PutArray* node) override;
    void visit(Starttime* node) override;
    void visit(Stoptime* node) override;
    void visit(BinaryOp* node) override;
    void visit(UnaryOp* node) override;
    void visit(ArrayExp* node) override;
    void visit(CallExp* node) override;
    void visit(ClassVar* node) override;
    void visit(BoolExp* node) override;
    void visit(This* node) override;
    void visit(Length* node) override;
    void visit(Esc* node) override;
    void visit(GetInt* node) override;
    void visit(GetCh* node) override;
    void visit(GetArray* node) override;
    void visit(IdExp* node) override;
    void visit(OpExp* node) override;
    void visit(IntExp* node) override;
};

Name_Maps* makeNameMaps(Program* node);
AST_Semant_Map* semant_analyze(Program* node);

#endif