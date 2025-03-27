#ifndef __AST_NAMEMAPS_HH__
#define __AST_NAMEMAPS_HH__

#include <map>
#include <set>
#include "ASTheader.hh"
#include "FDMJAST.hh"

using namespace std;
using namespace fdmj;

// 用于程序中各种名称及其关系的映射
class Name_Maps {
private:
    //构建所有名称映射，以方便访问所有名称的声明
    set<string> classes;                    // 所有类名的集合
    map<string, string> classHierachy;      // 将类名映射到其直接祖先类名
    map<pair<string, string>, Type*> methods;      // (类名, 方法名) -> 方法的返回类型
    map<pair<string, string>, VarDecl*> classVar;               // (类名, 变量名) -> 类变量声明
    map<tuple<string, string, string>, VarDecl*> methodVar;     // (类名, 方法名, 变量名) -> 局部变量声明
    map<tuple<string, string, string>, Formal*> methodFormal;   // 将 (类名, 方法名, 变量名) -> 方法的形参
    map<pair<string, string>, vector<string>> methodFormalList; // 将 (类名, 方法名) -> 方法的形参列表
                        // 最后一个用于返回类型（假装是一个Formal）
public:
    // 类
    bool is_class(string class_name);
    bool add_class(string class_name); // 如果类名已经存在，则返回false
    bool add_class_hiearchy(string class_name, string parent_name); // 如果存在循环继承，则返回false
    set<string> get_ancestors(string class_name);

    // 类->方法
    bool is_method(string class_name, string method_name); 
    bool add_method(string class_name, string method_name, Type* return_type);  // 如果方法名已经存在，则返回false
    Type* get_method(string class_name, string method_name);

    // 类->变量
    bool is_class_var(string class_name, string var_name);
    bool add_class_var(string class_name, string var_name, VarDecl* vd);
    VarDecl* get_class_var(string class_name, string var_name);

    // 类->方法->变量
    bool is_method_var(string class_name, string method_name, string var_name);
    bool add_method_var(string class_name, string method_name, string var_name, VarDecl* vd);
    VarDecl* get_method_var(string class_name, string method_name, string var_name);

    // 类->方法的形参
    bool is_method_formal(string class_name, string method_name, string var_name);
    bool add_method_formal(string class_name, string method_name, string var_name, Formal* f);
    Formal* get_method_formal(string class_name, string method_name, string var_name);

    // 类->方法的形参列表
    bool add_method_formal_list(string class_name, string method_name, vector<string> vl);
    vector<Formal*>* get_method_formal_list(string class_name, string method_name);

    void print();
    
};

//this visitor is to set up the name maps for the program

class AST_Name_Map_Visitor : public AST_Visitor {
private:
    Name_Maps *name_maps; //this is the map for all names in the program

    //you are allowed to add other members here
    string current_class;
    string current_method;
    
public:
    AST_Name_Map_Visitor() {
        name_maps = new Name_Maps();
        current_class = "";
        current_method = "";
    }
    Name_Maps* getNameMaps() { return name_maps; }

    void visit(Program* node) override;
    void visit(MainMethod* node) override;
    void visit(ClassDecl* node) override;
    void visit(Type *node) override;
    void visit(VarDecl* node) override;
    void visit(MethodDecl* node) override;
    void visit(Formal* node) override;
    
    void visit(Nested* node) override {return;}
    void visit(If* node) override {return;}
    void visit(While* node) override {return;}
    void visit(Assign* node) override {return;}
    void visit(CallStm* node) override {return;}
    void visit(Continue* node) override {return;}
    void visit(Break* node) override {return;}
    void visit(Return* node) override {return;}
    void visit(PutInt* node) override {return;}
    void visit(PutCh* node) override {return;}
    void visit(PutArray* node) override {return;}
    void visit(Starttime* node) override {return;}
    void visit(Stoptime* node) override {return;}
    void visit(BinaryOp* node) override {return;}
    void visit(UnaryOp* node) override {return;}
    void visit(ArrayExp* node) override {return;}
    void visit(CallExp* node) override {return;}
    void visit(ClassVar* node) override {return;}
    void visit(BoolExp* node) override {return;}
    void visit(This* node) override {return;}
    void visit(Length* node) override {return;}
    void visit(Esc* node) override {return;}
    void visit(GetInt* node) override {return;}
    void visit(GetCh* node) override {return;}
    void visit(GetArray* node) override {return;}
    void visit(IdExp* node) override {return;}
    void visit(OpExp* node) override {return;}
    void visit(IntExp* node) override {return;}
};

Name_Maps* makeNameMaps(Program* node);

#endif