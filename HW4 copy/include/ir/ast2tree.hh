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
//using namespace fdmj;
//using namespace tree;

//forward declaration
class Class_table;
class Method_var_table;
class Patch_list;
class ASTToTreeVisitor;

tree::Program* ast2tree(fdmj::Program* prog, AST_Semant_Map* semant_map);
Class_table* generate_class_table(AST_Semant_Map* semant_map);
Method_var_table* generate_method_var_table(string class_name, string method_name, Name_Maps* nm, Temp_map* tm);


// Class_table 用于将每个类的变量和方法映射到地址偏移量
// 这里采用 "通用类" 方法，即所有类共享同一个类表
// 其中包含所有类可能存在的变量和方法，并按相同的记录布局排列。
class Class_table {
public:
     map<string, int> var_pos_map;           // 变量名 -> 内存偏移量
     map<string, int> method_pos_map;        // 方法名 -> 方法表索引

     int table_size;                     // 类表大小（字节数）

     Class_table() {
          var_pos_map = map<string, int>();       
          method_pos_map = map<string, int>();    
     };
     ~Class_table() {
          var_pos_map.clear();
          method_pos_map.clear();
     }
     int get_var_pos(string var_name) {
          return var_pos_map[var_name];
     }
     int get_method_pos(string method_name) {
          return method_pos_map[method_name];
     }
};


// 每个方法都有一个变量表，包含形参和局部变量
// 如果方法的局部变量与形参名称冲突，则使用局部变量（忽略形参）
// 因此，在 Method_var_table 中，局部变量会覆盖形参
// 注意：每个局部变量和形参都有类型（INT 或 PTR）
// 方法的返回值也被视为一个特殊形参，其命名为 ​_^return^_method_name
class Method_var_table {
public:
     map<string, tree::Temp*> *var_temp_map;      // 变量名 -> 临时变量
     map<string, tree::Type> *var_type_map;       // 变量名 -> 变量类型
     Method_var_table() {
          var_temp_map = new map<string, tree::Temp*>();
          var_type_map = new map<string, tree::Type>();
     };

     // 获取 var_name 对应的临时变量
     tree::Temp* get_var_temp(string var_name) {
          return var_temp_map->at(var_name);
     }

     // 获取 var_name 对应的变量类型 
     tree::Type get_var_type(string var_name) {
          return var_type_map->at(var_name);
     }
};

class ASTToTreeVisitor : public fdmj::AST_Visitor {
public:
     tree::Tree *visit_tree_result = nullptr;          // 访问树的结果
     Tr_Exp* visit_exp_result = nullptr;               // 访问表达式的结果

     Compiler_Config* compiler_config = nullptr;       // 编译器配置
     AST_Semant_Map* semant_map = nullptr;             // 语义映射表
     Temp_map* temp_map = nullptr;                     // 临时变量(虚拟寄存器)映射表

     Class_table* class_table = nullptr;               // class table
     Method_var_table* method_var_table = nullptr;     // method var table

     std::vector<tree::FuncDecl*>* func_decl_list = nullptr; // 函数声明列表

     string current_class_name;     // 当前类名
     string current_method_name;    // 当前方法名

     tree::Label* current_loop_start_label = nullptr;  // 当前循环开始标签
     tree::Label* current_loop_end_label = nullptr;    // 当前循环结束标签


     ASTToTreeVisitor(AST_Semant_Map* ast_info): semant_map(ast_info)
     { 
          class_table = generate_class_table(semant_map);
     }
     
     ~ASTToTreeVisitor() { }

     tree::Tree* getTree() { return visit_tree_result; } //return the tree from a single visit (program returns a single tree)
     /*
     T_tree* getTree() { return visit_result_node; }
     Temp_map* getTempMap() { return visitor_temp_map; }
     map<string, Temp_temp*>* getVariableMap() { return visitor_variable_map; }
     */ 
     void visit(fdmj::Program* node) override;
     void visit(fdmj::MainMethod* node) override;
     void visit(fdmj::ClassDecl* node) override;
     void visit(fdmj::Type* node) override;
     void visit(fdmj::VarDecl* node) override;
     void visit(fdmj::MethodDecl* node) override;
     void visit(fdmj::Formal* node) override;
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
     void visit(fdmj::BinaryOp* node) override;
     void visit(fdmj::UnaryOp* node) override;
     void visit(fdmj::ArrayExp* node) override;
     void visit(fdmj::CallExp* node) override;
     void visit(fdmj::ClassVar* node) override;
     void visit(fdmj::BoolExp* node) override;
     void visit(fdmj::This* node) override;
     void visit(fdmj::Length* node) override;
     void visit(fdmj::Esc* node) override;
     void visit(fdmj::GetInt* node) override;
     void visit(fdmj::GetCh* node) override;
     void visit(fdmj::GetArray* node) override;
     void visit(fdmj::IdExp* node) override;
     void visit(fdmj::OpExp* node) override;
     void visit(fdmj::IntExp* node) override;
};

#endif
