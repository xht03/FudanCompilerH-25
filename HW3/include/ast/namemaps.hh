#ifndef __AST_NAMEMAPS_HH__
#define __AST_NAMEMAPS_HH__

#include <map>
#include <set>
#include "ASTheader.hh"
#include "FDMJAST.hh"

using namespace std;
using namespace fdmj;

class Name_Maps {
private:
    set<string> classes;               // 类名集合
    map<string, string> classHierachy; // 类->父类

    map<pair<string, string>, VarDecl*> classVar;               // 类成员变量 -> 声明结点
    map<pair<string, string>, Type*> methods;                   // 类方法    -> 方法返回类型
    map<tuple<string, string, string>, Formal*> methodFormal;   // 类方法参数 -> 参数结点
    map<tuple<string, string, string>, VarDecl*> methodVar;     // 类方法变量 -> 声明结点
    map<pair<string, string>, vector<string>> methodFormalList; // 类方法名   -> 参数名列表
public:
    // 类
    bool is_class(string class_name);                               // 判断是否为类
    bool add_class(string class_name);                              // 添加类
    bool add_class_hiearchy(string class_name, string parent_name); // 添加类继承关系
    set<string> get_ancestors(string class_name);                   // 获取类的所有祖先类

    // 类->成员变量
    bool is_class_var(string class_name, string var_name);               // 判断是否为类成员变量
    bool add_class_var(string class_name, string var_name, VarDecl* vd); // 添加类成员变量
    VarDecl* get_class_var(string class_name, string var_name);          // 获取类成员变量
    vector<VarDecl*>* get_class_var(string class_name);                  // 获取类所有成员变量

    // 类->方法->返回类型
    bool is_method(string class_name, string method_name);              // 判断是否为类方法
    bool add_method(string class_name, string method_name, Type* type); // 添加类方法返回类型
    Type* get_method_type(string class_name, string method_name);       // 获取类方法返回类型
    vector<string>* get_method(string class_name);                      // 获取类所有方法

    // 类->方法->参数列表
    bool add_method_formal_list(string class_name, string method_name, vector<string> vl); // 添加类方法参数列表
    vector<Formal*>* get_method_formal_list(string class_name, string method_name);        // 获取类方法参数列表

    // 类->方法->参数
    bool is_method_formal(string class_name, string method_name, string var_name);             // 判断是否为类方法参数
    bool add_method_formal(string class_name, string method_name, string var_name, Formal* f); // 添加类方法参数
    Formal* get_method_formal(string class_name, string method_name, string var_name);         // 获取类方法参数

    // 类->方法->变量
    bool is_method_var(string class_name, string method_name, string var_name);               // 判断是否为类方法变量
    bool add_method_var(string class_name, string method_name, string var_name, VarDecl* vd); // 添加类方法变量
    VarDecl* get_method_var(string class_name, string method_name, string var_name);          // 获取类方法变量

    void print(); // 打印信息
};

class AST_Name_Map_Visitor : public AST_Visitor {
private:
    Name_Maps* name_maps;

    string current_class_name;
    string current_method_name;

public:
    AST_Name_Map_Visitor() { name_maps = new Name_Maps(); }
    Name_Maps* getNameMaps() { return name_maps; }

    void visit(Program* node) override;
    void visit(MainMethod* node) override;
    void visit(ClassDecl* node) override;
    void visit(Type* node) override { }
    void visit(VarDecl* node) override;
    void visit(MethodDecl* node) override;
    void visit(Formal* node) override;

    // 语句
    void visit(Nested* node) override { }
    void visit(If* node) override { }
    void visit(While* node) override { }
    void visit(Assign* node) override { }
    void visit(CallStm* node) override { }
    void visit(Continue* node) override { }
    void visit(Break* node) override { }
    void visit(Return* node) override { }
    void visit(PutInt* node) override { }
    void visit(PutCh* node) override { }
    void visit(PutArray* node) override { }
    void visit(Starttime* node) override { }
    void visit(Stoptime* node) override { }

    // 表达式
    void visit(Esc* node) override { }
    void visit(IdExp* node) override { }
    void visit(IntExp* node) override { }
    void visit(BoolExp* node) override { }
    void visit(ArrayExp* node) override { }
    void visit(OpExp* node) override { }
    void visit(BinaryOp* node) override { }
    void visit(UnaryOp* node) override { }
    void visit(This* node) override { }
    void visit(CallExp* node) override { }
    void visit(ClassVar* node) override { }
    void visit(GetInt* node) override { }
    void visit(GetCh* node) override { }
    void visit(GetArray* node) override { }
    void visit(Length* node) override { }
};

// 对外接口函数
Name_Maps* makeNameMaps(Program* node);

#endif