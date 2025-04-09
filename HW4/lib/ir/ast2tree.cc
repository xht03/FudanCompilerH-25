#define DEBUG
//#undef DEBUG

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include "config.hh"
#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "treep.hh"
#include "temp.hh"
#include "ast2tree.hh"

using namespace std;
//using namespace fdmj;
//using namespace tree;


// you need to code this function!
tree::Program* ast2tree(fdmj::Program* prog, AST_Semant_Map* semant_map) {
    ASTToTreeVisitor* visitor = new ASTToTreeVisitor();
    prog->accept(*visitor);
    tree::Tree* result = visitor->getTree();
    delete visitor;
    return dynamic_cast<tree::Program*>(result);
}

void ASTToTreeVisitor::visit(fdmj::Program* node) {

}




void ASTToTreeVisitor::visit(fdmj::BinaryOp* node) {
    tree::Exp* left = nullptr;
    tree::Exp* right = nullptr;
    tree::Type result_type;

    // 访问左操作数
    node->left->accept(*this);
    left = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (left == nullptr) {
        cerr << "Error: left operand is not a valid expression." << endl;
        return;
    }

    // 访问右操作数
    node->right->accept(*this);
    right = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (right == nullptr) {
        cerr << "Error: right operand is not a valid expression." << endl;
        return;
    }

    // 获取语义信息
    AST_Semant* semant = semant_map->getSemant(node);
    if (semant == nullptr) {
        cerr << "Error: semantic information not found for the node." << endl;
        return;
    }

    // 确定结果类型
    if (semant->get_type() == TypeKind::INT) {
        result_type = tree::Type::INT;
    }
    else if (semant->get_type() == TypeKind::ARRAY) {
        result_type = tree::Type::PTR;
    }
    else if (semant->get_type() == TypeKind::CLASS) {
        result_type = tree::Type::PTR;
    }
    else {
        cerr << "Error: unknown type for the node." << endl;
        return;
    }

    // 创建节点
    tree::Binop* binop_node = new tree::Binop(result_type, node->op->op, left, right);
    visit_tree_result = binop_node;
}




// 地址计算 需要参考 config.hh

void ASTToTreeVisitor::visit(fdmj::CallExp* node) {
    
}


void ASTToTreeVisitor::visit(fdmj::ClassVar* node) {
    // 访问对象，获取对象的地址
    node->obj->accept(*this);
    tree::Exp* obj_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (obj_exp == nullptr) {
        cerr << "Error: object expression is not a valid expression." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 获取语义信息
    AST_Semant* semant = semant_map->getSemant(node);
    if (semant == nullptr) {
        cerr << "Error: semantic information not found for ClassVar." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 确定变量类型
    tree::Type var_type;
    if (semant->get_type() == TypeKind::INT) {
        var_type = tree::Type::INT;
    }
    else if (semant->get_type() == TypeKind::ARRAY) {
        var_type = tree::Type::PTR;
    }
    else if (semant->get_type() == TypeKind::CLASS) {
        var_type = tree::Type::PTR;
    }
    else {
        cerr << "Error: unknown type for ClassVar." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 获取成员变量在类中的偏移量
    int offset = class_table->get_var_pos(node->id->id);

    // 计算变量地址：对象地址 + 偏移量
    tree::Exp* addr_exp = new tree::Binop(tree::Type::PTR, "+", obj_exp, new tree::Const(offset));
    
    // 创建访存表达式
    tree::Mem* mem_exp = new tree::Mem(var_type, addr_exp);
    visit_tree_result = mem_exp;
}


void ASTToTreeVisitor::visit(fdmj::BoolExp* node) {
    int int_val = node->val ? 1 : 0;

    tree::Const* const_node = new tree::Const(int_val);
    visit_tree_result = const_node;
}


void ASTToTreeVisitor::visit(fdmj::This* node) {
    // 检查方法变量表是否为空
    if (method_var_table == nullptr) {
        cerr << "Error: method_var_table is null in This expression." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 从方法变量表中获取 this 指针
    tree::Temp* this_temp;
    this_temp = method_var_table->get_var_temp("this");
    if (this_temp == nullptr) {
        cerr << "Error: 'this' variable not found in method_var_table." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 创建 TempExp 节点
    tree::TempExp* this_exp = new tree::TempExp(tree::Type::PTR, this_temp);
    visit_tree_result = this_exp;
}



void ASTToTreeVisitor::visit(fdmj::Length* node) {
    // 访问数组表达式
    node->exp->accept(*this);
    tree::Exp* array_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (array_exp == nullptr) {
        cerr << "Error: array expression is not a valid expression." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();
    args->push_back(array_exp);

    // 创建节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "length", args);
    visit_tree_result = ext_call;
}


void ASTToTreeVisitor::visit(fdmj::Esc* node) {
    // 创建语句列表
    std::vector<tree::Stm*>* stm_list = new std::vector<tree::Stm*>();

    for (auto& stm : *node->sl) {
        // 访问每个语句
        stm->accept(*this);
        tree::Stm* tree_stm = dynamic_cast<tree::Stm*>(visit_tree_result);

        if (tree_stm == nullptr) {
            cerr << "Error: statement is not a valid statement." << endl;
            return;
        }
        stm_list->push_back(tree_stm);
    }

    // 创建 Seq 节点
    tree::Seq* seq = new tree::Seq(stm_list);

    // 访问表达式
    node->exp->accept(*this);
    tree::Exp* exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (exp == nullptr) {
        cerr << "Error: expression is not a valid expression." << endl;
        return;
    }

    // 确定结果类型
    tree::Type result_type;
    AST_Semant* semant = semant_map->getSemant(node);
    if (semant == nullptr) {
        cerr << "Error: semantic information not found for Esc." << endl;
        return;
    }

    if (semant->get_type() == TypeKind::INT) {
        result_type = tree::Type::INT;
    }
    else if (semant->get_type() == TypeKind::ARRAY) {
        result_type = tree::Type::PTR;
    }
    else if (semant->get_type() == TypeKind::CLASS) {
        result_type = tree::Type::PTR;
    }
    else {
        cerr << "Error: unknown type for Esc." << endl;
        return;
    }

    // 创建 Eseq 节点
    tree::Eseq* eseq = new tree::Eseq(result_type, seq, exp);
    visit_tree_result = eseq;
}


void ASTToTreeVisitor::visit(fdmj::GetInt* node) {
    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 创建节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "getint", args);
    visit_tree_result = ext_call;
}


void ASTToTreeVisitor::visit(fdmj::GetCh* node) {
    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 创建节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "getch", args);
    visit_tree_result = ext_call;
}


void ASTToTreeVisitor::visit(fdmj::GetArray* node) {
    // 访问数组表达式
    node->exp->accept(*this);
    tree::Exp* array_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (array_exp == nullptr) {
        cerr << "Error: array expression is not a valid expression." << endl;
        visit_tree_result = nullptr;
        return;
    }
    
    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();
    args->push_back(array_exp);

    // 创建节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "getarray", args);
    visit_tree_result = ext_call;
}


void ASTToTreeVisitor::visit(fdmj::IdExp* node) {
    // 获取语义信息
    AST_Semant* semant = semant_map->getSemant(node);
    if (semant == nullptr) {
        cerr << "Error: semantic information not found for the node." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 语义信息种类
    AST_Semant::Kind kind = semant->get_kind();

    if (kind == AST_Semant::Kind::ClassName) {
        // 若是类名，在IR中通常不直接表示
        return;
    }
    else if (kind == AST_Semant::Kind::MethodName) {
        // 如果是方法名
        // TODO
    }
    else if (kind == AST_Semant::Kind::Value) {
        // 如果是变量名
        // 获取变量类型
        tree::Type var_type;
        if (semant->get_type() == TypeKind::INT) {
            var_type = tree::Type::INT;
        } else {
            // 类和数组类型都是指针
            var_type = tree::Type::PTR;
        }
    
        // 若是局部变量 (方法内)
        tree::Temp* temp = method_var_table->get_var_temp(node->id);
        if (temp != nullptr) {
            tree::TempExp* temp_exp = new tree::TempExp(var_type, temp);
            visit_tree_result = temp_exp;
            return;
        }
    
        // 不是局部变量，若是类成员变量
        // 需要从当前对象(this)获取
        // 1. 获取this指针
        tree::Temp* this_temp = method_var_table->get_var_temp("this");
        tree::TempExp* this_exp = new tree::TempExp(tree::Type::PTR, this_temp);
    
        // 2. 获取变量在类中的偏移量
        int offset = class_table->get_var_pos(node->id);
    
        // 3. 创建访存表达式
        tree::Binop* addr_exp = new tree::Binop(tree::Type::PTR, "+", this_exp, new tree::Const(offset));
        tree::Mem* mem_exp = new tree::Mem(var_type, addr_exp);
    
        visit_tree_result = mem_exp;
    }
    else {
        cerr << "Error: unknown semantic kind for IdExp." << endl;
        visit_tree_result = nullptr;
        return;
    }

}


void ASTToTreeVisitor::visit(fdmj::OpExp* node) {
    // 实际上不需要使用这个节点
    visit_tree_result = nullptr;
}


void ASTToTreeVisitor::visit(fdmj::IntExp* node) {
    tree::Const* const_node = new tree::Const(node->val);

    visit_tree_result = const_node;
}