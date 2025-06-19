#ifndef _AST2TREE_HH
#define _AST2TREE_HH

#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "semant.hh"
#include "temp.hh"
#include "treep.hh"
#include "tr_exp.hh"
#include "config.hh"

using namespace std;
// using namespace fdmj;
// using namespace tree;

class Class_table;
class Method_var_table;
class Patch_list;
class ASTToTreeVisitor;

tree::Program* ast2tree(fdmj::Program* prog, AST_Semant_Map* semant_map);
Class_table* gen_class_table_map(Name_Maps* name_maps);
Method_var_table* generate_method_var_table(string class_name, string method_name, Name_Maps* nm, Temp_map* tm);

// 类表将每个类变量和方法映射到一个地址偏移量
// 这使用了一个“通用类”方法
// 即所有类使用相同的类表, 所有类的所有可能变量和方法都列在同一记录布局中
class Class_table {
public:
    int offset = 0;
    map<string, int>* var_pos_map;    // 变量位置映射
    map<string, int>* method_pos_map; // 方法位置映射

    Class_table(map<string, int>* var_pos_map, map<string, int>* method_pos_map, int offset)
        : var_pos_map(var_pos_map)
        , method_pos_map(method_pos_map)
        , offset(offset) { };
    ~Class_table() { };

    int get_var_pos(string var_name)
    {
        if (var_pos_map->find(var_name) == var_pos_map->end()) {
            cerr << "get_var_pos: " << var_name << "成员变量不存在" << endl;
            exit(-1);
        }
        return (*var_pos_map)[var_name];
    }
    int get_method_pos(string method_name)
    {
        if (method_pos_map->find(method_name) == method_pos_map->end()) {
            cerr << "get_method_pos: " << method_name << "成员方法不存在" << endl;
            exit(-1);
        }
        return (*method_pos_map)[method_name];
    }
};

// 每个方法(函数)都有一个变量表
// 包含形参(formal)和局部变量(local var)
// (如果局部变量名与形参冲突, 则优先使用局部变量, 忽略形参)
// 因此, 局部变量会覆盖(override)同名的形参
// 注意：每个局部变量和形参都有类型(INT 或 PTR)
// 方法的返回值也被视为一个特殊的形参, 命名为 `_^return^_method_name`
class Method_var_table {
public:
    map<string, tree::Temp*>* var_temp_map; // 变量名->变量结点
    map<string, tree::Type>* var_type_map;  // 变量名->类型结点

    Method_var_table(map<string, tree::Temp*>* var_temp_map, map<string, tree::Type>* var_type_map)
        : var_temp_map(var_temp_map)
        , var_type_map(var_type_map) { };

    tree::Temp* get_var_temp(string var_name)
    {
        if (var_temp_map->find(var_name) == var_temp_map->end()) {
            cerr << "get_var_temp: " << var_name << "不存在" << endl;
            exit(-1);
        }
        return (*var_temp_map)[var_name];
    }
    tree::Type get_var_type(string var_name)
    {
        if (var_type_map->find(var_name) == var_type_map->end()) {
            cerr << "get_var_type: " << var_name << "不存在" << endl;
            exit(-1);
        }
        return (*var_type_map)[var_name];
    }
};

class ASTToTreeVisitor : public fdmj::AST_Visitor {
public:
    AST_Semant_Map* ast_info = nullptr; // FDMJ语义分析表

    tree::Program* tree_root = nullptr; // 根节点
    Temp_map temp_map;                  // 变量标签编号表

    Class_table* class_table; // 全局类表

    string class_name = "";             // 当前类名
    string method_name = "";            // 当前方法名
    Method_var_table* method_var_table; // 当前方法变量表
    vector<tree::Tree*> newNodes;       // 下层结点集
    Tr_Exp* newExp = nullptr;           // 下层表达式

    // while语句标签 (用于continue和break语句)
    Label* cur_L_while = nullptr;
    Label* cur_L_end = nullptr;

    tree::TempExp* this_temp = nullptr; // this指针

    ASTToTreeVisitor(AST_Semant_Map* ast_info)
        : ast_info(ast_info)
    {
        // 生成类表
        class_table = gen_class_table_map(ast_info->name_maps);
    }
    ~ASTToTreeVisitor() { }
    tree::Program* getTree() { return tree_root; }

    // CallStm和CallExp的辅助函数
    tree::Call* call_helper(fdmj::Exp* obj, fdmj::IdExp* name, vector<fdmj::Exp*>* par);

    void visit(fdmj::Program* node) override;
    void visit(fdmj::MainMethod* node) override;
    void visit(fdmj::ClassDecl* node) override;
    void visit(fdmj::Type* node) override;
    void visit(fdmj::VarDecl* node) override;
    void visit(fdmj::MethodDecl* node) override;
    void visit(fdmj::Formal* node) override;

    // 语句
    void visit(fdmj::Nested* node) override;
    void visit(fdmj::If* node) override;
    void visit(fdmj::While* node) override;
    void visit(fdmj::Assign* node) override;
    void visit(fdmj::CallStm* node) override;
    void visit(fdmj::Continue* node) override;
    void visit(fdmj::Break* node) override;
    void visit(fdmj::Return* node) override;
    void visit(fdmj::PutInt* node) override;
    void visit(fdmj::PutCh* node) override;
    void visit(fdmj::PutArray* node) override;
    void visit(fdmj::Starttime* node) override;
    void visit(fdmj::Stoptime* node) override;

    // 表达式
    void visit(fdmj::Esc* node) override;
    void visit(fdmj::IdExp* node) override;
    void visit(fdmj::IntExp* node) override;
    void visit(fdmj::BoolExp* node) override;
    void visit(fdmj::ArrayExp* node) override;
    void visit(fdmj::OpExp* node) override;
    void visit(fdmj::BinaryOp* node) override;
    void visit(fdmj::UnaryOp* node) override;
    void visit(fdmj::This* node) override;
    void visit(fdmj::CallExp* node) override;
    void visit(fdmj::ClassVar* node) override;
    void visit(fdmj::GetInt* node) override;
    void visit(fdmj::GetCh* node) override;
    void visit(fdmj::GetArray* node) override;
    void visit(fdmj::Length* node) override;
};

#endif
