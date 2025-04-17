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
    set<pair<string, string>> methods;                          // 类方法 (返回参数 _^return^_method)
    map<tuple<string, string, string>, Formal*> methodFormal;   // 类方法参数 -> 参数结点
    map<tuple<string, string, string>, VarDecl*> methodVar;     // 类方法变量 -> 声明结点
    map<pair<string, string>, vector<string>> methodFormalList; // 类方法名   -> 参数名列表
public:
    // to report if there is any error in setting up the name maps
    bool passed_name_maps = true;

    // 类
    bool is_class(string class_name);  // 判断是否为类
    bool add_class(string class_name); // 添加类
    set<string>* get_class_list();

    bool detected_loop(map<string, string> classHierachy);
    bool add_class_hiearchy(string class_name, string parent_name); // 添加类继承关系
    string get_parent(string class_name);                           // 获取类的父类
    vector<string>* get_ancestors(string class_name);               // 获取类的所有祖先类

    // 类->成员变量
    bool is_class_var(string class_name, string var_name);               // 判断是否为类成员变量
    bool add_class_var(string class_name, string var_name, VarDecl* vd); // 添加类成员变量
    VarDecl* get_class_var(string class_name, string var_name);          // 获取类成员变量
    set<string>* get_class_var_list(string class_name);                  // 获取类所有成员变量

    // 类->方法
    fdmj::Formal* get_method_return_formal(string class_name, string method_name); // 获取类方法返回参数
    bool is_method(string class_name, string method_name);                         // 判断是否为类方法
    bool add_method(string class_name, string method_name);                        // 添加类方法
    bool add_method(string class_name, string method_name, Formal* f);             // 添加类方法 (带返回参数)
    set<string>* get_method_list(string class_name);                               // 获取类所有方法

    // 类->方法->参数列表
    bool add_method_formal_list(string class_name, string method_name, vector<string> vl); // 添加类方法参数列表
    vector<string>* get_method_formal_list(string class_name, string method_name);         // 获取类方法参数列表

    // 类->方法->参数
    bool is_method_formal(string class_name, string method_name, string var_name);             // 判断是否为类方法参数
    bool add_method_formal(string class_name, string method_name, string var_name, Formal* f); // 添加类方法参数
    Formal* get_method_formal(string class_name, string method_name, string var_name);         // 获取类方法参数

    // 类->方法->变量
    bool is_method_var(string class_name, string method_name, string var_name);               // 判断是否为类方法变量
    bool add_method_var(string class_name, string method_name, string var_name, VarDecl* vd); // 添加类方法变量
    VarDecl* get_method_var(string class_name, string method_name, string var_name);          // 获取类方法变量
    set<string>* get_method_var_list(string class_name, string method_name);

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

    void visit(fdmj::Program* node) override;
    void visit(fdmj::MainMethod* node) override;
    void visit(fdmj::ClassDecl* node) override;
    void visit(fdmj::Type* node) override { }
    void visit(fdmj::VarDecl* node) override;
    void visit(fdmj::MethodDecl* node) override;
    void visit(fdmj::Formal* node) override;

    // 语句
    void visit(fdmj::Nested* node) override { }
    void visit(fdmj::If* node) override { }
    void visit(fdmj::While* node) override { }
    void visit(fdmj::Assign* node) override { }
    void visit(fdmj::CallStm* node) override { }
    void visit(fdmj::Continue* node) override { }
    void visit(fdmj::Break* node) override { }
    void visit(fdmj::Return* node) override { }
    void visit(fdmj::PutInt* node) override { }
    void visit(fdmj::PutCh* node) override { }
    void visit(fdmj::PutArray* node) override { }
    void visit(fdmj::Starttime* node) override { }
    void visit(fdmj::Stoptime* node) override { }

    // 表达式
    void visit(fdmj::Esc* node) override { }
    void visit(fdmj::IdExp* node) override { }
    void visit(fdmj::IntExp* node) override { }
    void visit(fdmj::BoolExp* node) override { }
    void visit(fdmj::ArrayExp* node) override { }
    void visit(fdmj::OpExp* node) override { }
    void visit(fdmj::BinaryOp* node) override { }
    void visit(fdmj::UnaryOp* node) override { }
    void visit(fdmj::This* node) override { }
    void visit(fdmj::CallExp* node) override { }
    void visit(fdmj::ClassVar* node) override { }
    void visit(fdmj::GetInt* node) override { }
    void visit(fdmj::GetCh* node) override { }
    void visit(fdmj::GetArray* node) override { }
    void visit(fdmj::Length* node) override { }
};

// 对外接口函数
Name_Maps* makeNameMaps(fdmj::Program* node);

const string MAIN_class_name = "_^main^_";
const string MAIN_method_name = "main";
const string return_prefix = "_^return^_";
#endif