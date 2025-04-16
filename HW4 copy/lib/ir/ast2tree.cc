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


Class_table* generate_class_table(AST_Semant_Map* semant_map) {
    set<string> var_list;
    set<string> method_list;

    Name_Maps* name_maps = semant_map->getNameMaps();
    set<string>* class_list = name_maps->get_class_list();
    for (auto class_name : *class_list) {
        auto class_var_list = name_maps->get_class_var_list(class_name);
        auto class_method_list = name_maps->get_method_list(class_name);

        var_list.insert(class_var_list->begin(), class_var_list->end());
        method_list.insert(class_method_list->begin(), class_method_list->end());
    }

    int offset = 0;
    
    auto var_pos_map = new map<string, int>();
    for (auto var : var_list) {
        (*var_pos_map)[var] = offset;
        offset += 4;
    }

    auto method_pos_map = new map<string, int>();
    for (auto method : method_list) {
        (*method_pos_map)[method] = offset;
        offset += 4;
    }

    Class_table* class_table = new Class_table();
    class_table->var_pos_map = *var_pos_map;
    class_table->method_pos_map = *method_pos_map;
    class_table->table_size = offset;

    delete var_pos_map;
    delete method_pos_map;

    return class_table;
}


Method_var_table* generate_method_var_table(string class_name, string method_name, Name_Maps* nm, Temp_map* tm) {
    Method_var_table* mvt = new Method_var_table();

    // 如果不是main方法，添加this指针作为第一个参数
    if (class_name != "_^main^_") {
        tree::Temp* this_temp = tm->newtemp();
        mvt->var_temp_map->insert({"this", this_temp});
        mvt->var_type_map->insert({"this", tree::Type::PTR});
    }

    // 添加方法的形参
    vector<string>* formal_list = nm->get_method_formal_list(class_name, method_name);
    if (formal_list != nullptr) {
        for (const string& formal_name : *formal_list) {
            fdmj::Formal* formal = nm->get_method_formal(class_name, method_name, formal_name);
            if (formal != nullptr) {
                // 创建形参的临时变量
                tree::Temp* param_temp = tm->newtemp();
                
                // 确定形参类型
                tree::Type param_type;
                if (formal->type->typeKind == fdmj::TypeKind::INT) {
                    param_type = tree::Type::INT;
                } else {
                    param_type = tree::Type::PTR; // 类和数组类型都是指针类型
                }
                
                // 添加到方法变量表
                mvt->var_temp_map->insert({formal_name, param_temp});
                mvt->var_type_map->insert({formal_name, param_type});
            }
        }
    }

    // 添加方法的局部变量
    set<string>* var_list = nm->get_method_var_list(class_name, method_name);
    if (var_list != nullptr) {
        for (const string& var_name : *var_list) {
            fdmj::VarDecl* var_decl = nm->get_method_var(class_name, method_name, var_name);
            if (var_decl != nullptr) {
                // 创建局部变量的临时变量
                tree::Temp* var_temp = tm->newtemp();
                
                // 确定变量类型
                tree::Type var_type;
                if (var_decl->type->typeKind == fdmj::TypeKind::INT) {
                    var_type = tree::Type::INT;
                } else {
                    var_type = tree::Type::PTR; // 类和数组类型都是指针类型
                }
                
                // 添加到方法变量表（局部变量会覆盖同名形参）
                mvt->var_temp_map->insert_or_assign(var_name, var_temp);
                mvt->var_type_map->insert_or_assign(var_name, var_type);
            }
        }
    }

    // 添加返回值（作为特殊变量）
    string return_name = "_^return^_" + method_name;
    tree::Temp* return_temp = tm->newtemp();
    tree::Type return_type;

    // 获取返回值类型
    if (formal_list != nullptr && !formal_list->empty()) {
        // 最后一个元素可能是返回类型
        string return_type_formal = formal_list->back();
        fdmj::Formal* return_formal = nm->get_method_formal(class_name, method_name, return_type_formal);
        
        if (return_formal != nullptr && return_formal->type != nullptr) {
            if (return_formal->type->typeKind == fdmj::TypeKind::INT) {
                return_type = tree::Type::INT;
            } else {
                return_type = tree::Type::PTR; // 类和数组类型都是指针
            }
        }
    }

    // 添加返回值到方法变量表
    mvt->var_temp_map->insert({return_name, return_temp});
    mvt->var_type_map->insert({return_name, return_type});

    return mvt;
}



tree::Program* ast2tree(fdmj::Program* prog, AST_Semant_Map* semant_map) {
    // 创建 ASTToTreeVisitor 实例
    ASTToTreeVisitor* visitor = new ASTToTreeVisitor(semant_map);
    
    // 访问 AST 节点
    prog->accept(*visitor);
    tree::Tree* result = visitor->getTree();

    delete visitor;
    return static_cast<tree::Program*>(result);
}


void ASTToTreeVisitor::visit(fdmj::Program* node) {
    // 函数申明列表
    vector<tree::FuncDecl*>* fdl = new vector<tree::FuncDecl*>();

    // 访问主方法
    node->main->accept(*this);
    fdl->push_back(static_cast<tree::FuncDecl*>(visit_node[0]));
    visit_node.clear();


    // 所有类声明
    if (node->cdl) {
        for (auto& cd : *(node->cdl)) {
            cd->accept(*this);
            for (auto& fd : visit_node) {
                if (fd) {
                    fdl->push_back(static_cast<tree::FuncDecl*>(fd));
                }
            }
            visit_node.clear();
        }
    }

    // 创建程序节点
    tree_root = new tree::Program(fdl);
}


void ASTToTreeVisitor::visit(fdmj::MainMethod* node) {
    // 更新当前类名和方法名
    current_class_name = "_^main^_";
    current_method_name = "main";
    
    // 创建新的临时变量映射表和方法变量表
    temp_map = new tree::Temp_map();
    method_var_table = generate_method_var_table("_^main^_", "main", semant_map->getNameMaps(), temp_map);
    
    std::vector<tree::Temp*>* args = new std::vector<tree::Temp*>();    // 参数列表
    std::vector<tree::Stm*>* stm_list = new std::vector<tree::Stm*>();  // 语句列表

    // 局部变量声明
    for (fdmj::VarDecl* vd : *node->vdl) {
        vd->accept(*this);
        for (auto& stm_node : visit_node)
            if (stm_node)
                stm_list->push_back(static_cast<tree::Stm*>(stm_node));
        visit_node.clear();
    }

    // 语句列表
    for (fdmj::Stm* stm : *(node->sl)) {
        stm->accept(*this);
        for (auto& stm_node : visit_node)
            if (stm_node)
                stm_list->push_back(static_cast<tree::Stm*>(stm_node));
        visit_node.clear(); 
    }

    // 添加入口标签
    tree::Label* entry_label = temp_map->newlabel();
    stm_list->insert(stm_list->begin(), new tree::LabelStm(entry_label));

    // 创建基本块
    tree::Block* entry_block = new tree::Block(entry_label, nullptr, stm_list);
    std::vector<tree::Block*>* blocks = new std::vector<tree::Block*>();
    blocks->push_back(entry_block);
    
    // 记录临时变量和标签的最大编号
    int last_temp_num = temp_map->next_temp - 1;
    int last_label_num = temp_map->next_label - 1;

    visit_node.push_back(new tree::FuncDecl(
        current_class_name + "^" + current_method_name,
        args,
        blocks,
        tree::Type::INT,
        last_temp_num,
        last_label_num
    ));
}


void ASTToTreeVisitor::visit(fdmj::ClassDecl* node) {
    // 更新当前类名
    current_class_name = node->id->id;

    // 类方法
    auto fdl = new vector<tree::Tree*>();
    for (auto md : *(node->mdl)) {
        md->accept(*this);
        fdl->push_back(visit_node[0]);
        visit_node.clear();
    }

    visit_node.insert(visit_node.end(), fdl->begin(), fdl->end());
}


// Type 节点本身不需要转换为特定的 IR 节点
void ASTToTreeVisitor::visit(fdmj::Type* node) { }


void ASTToTreeVisitor::visit(fdmj::VarDecl* node) {
    
}


void ASTToTreeVisitor::visit(fdmj::MethodDecl* node) {
    // 更新当前方法名
    current_method_name = node->id->id;

    // 清空变量标签方法名编码表
    temp_map.clear();

    // 为当前函数生成方法变量表
    this_temp = new tree::TempExp(tree::Type::PTR, temp_map.newtemp());
    method_var_table = generate_method_var_table(class_name, method_name, ast_info->getNameMaps(), &temp_map);

    // 形参列表
    vector<tree::Temp*>* args = new vector<tree::Temp*> { this_temp->temp };
    auto formal_list = ast_info->name_maps->get_method_formal_list(class_name, method_name);
    for (auto formal_name : *formal_list)
        args->push_back(method_var_table->get_var_temp(formal_name));

    // 语句列表
    vector<tree::Stm*>* sl = new vector<tree::Stm*>();

    // 变量声明列表
    for (fdmj::VarDecl* vd : *node->vdl) {
        vd->accept(*this);
        for (auto& newNode : newNodes)
            if (newNode)
                sl->push_back(static_cast<tree::Stm*>(newNode));
        newNodes.clear();
    }

    // 语句列表
    for (fdmj::Stm* stm : *node->sl) {
        stm->accept(*this);
        for (auto& newNode : newNodes)
            if (newNode)
                sl->push_back(static_cast<tree::Stm*>(newNode));
        newNodes.clear();
    }

    // 在开头插入返回标签
    Label* entry_label = temp_map.newlabel();
    sl->insert(sl->begin(), new tree::LabelStm(entry_label));

    // 构造Blocks
    tree::Block* bk = new tree::Block(entry_label, nullptr, sl);
    vector<tree::Block*>* bkl = new vector<tree::Block*>();
    bkl->push_back(bk);

    int lt = temp_map.next_temp - 1;
    int ll = temp_map.next_label - 1;
    auto return_type = node->type->typeKind == TypeKind::INT ? tree::Type::INT : tree::Type::PTR;
    newNodes.push_back(new tree::FuncDecl(class_name + "^" + method_name, args, bkl, return_type, lt, ll));
}



void ASTToTreeVisitor::visit(fdmj::Formal* node) {
    // 获取参数类型
    tree::Type var_type;
    if (node->type->typeKind == TypeKind::INT) {
        var_type = tree::Type::INT;
    } else {
        var_type = tree::Type::PTR;     // 数组和类类型都是指针
    }

    // 记录参数信息
    if (method_var_table != nullptr && temp_map != nullptr) {
        Temp* temp = temp_map->newtemp();
        method_var_table->var_temp_map->insert({node->id->id, temp});
        method_var_table->var_type_map->insert({node->id->id, var_type});
    }

    // Formal 节点本身不直接转换为 IR 节点
    visit_node = nullptr;
}


void ASTToTreeVisitor::visit(fdmj::Nested* node) {
    // 如果语句列表为空，返回空序列
    if (node->sl == nullptr || node->sl->empty()) {
        visit_node = new tree::Seq();
        return;
    }

    // 创建新的语句列表
    std::vector<tree::Stm*>* stm_list = new std::vector<tree::Stm*>();

    for (auto& stm : *(node->sl)) {
        // 访问子语句
        stm->accept(*this);
        tree::Stm* tree_stm = dynamic_cast<tree::Stm*>(visit_node);
        if (tree_stm != nullptr) {
            stm_list->push_back(tree_stm);
        } else {
            cerr << "Warning: Statement in Nested block is not a valid statement, skipping." << endl;
        }
    }

    // 创建语句序列
    tree::Seq* seq = new tree::Seq(stm_list);
    visit_node = seq;
}


void ASTToTreeVisitor::visit(fdmj::If* node) {

    // 条件表达式
    node->exp->accept(*this);
    Tr_cx* cond_exp = visit_exp->unCx(temp_map);

    auto L1 = cond_exp->true_list;
    auto L2 = cond_exp->false_list;
    auto L_true = temp_map->newlabel();
    auto L_false = temp_map->newlabel();
    auto L_end = temp_map->newlabel();

    // 用L_true填补L1, 用L_false填补L2
    L1->patch(L_true);
    L2->patch(L_false);

    std::vector<tree::Stm*>* stm_list = new std::vector<tree::Stm*>();
    stm_list->push_back(cond_exp->stm);
    stm_list->push_back(new tree::LabelStm(L_true));

    node->stm1->accept(*this);
    tree::Stm* stm1 = static_cast<tree::Stm*>(visit_node);
    stm_list->push_back(stm1);
    stm_list->push_back(new tree::Jump(L_end));

    stm_list->push_back(new tree::LabelStm(L_false));
    if (node->stm2) {
        node->stm2->accept(*this);
        tree::Stm* stm2 = static_cast<tree::Stm*>(visit_node);
        stm_list->push_back(stm2);
    }

    stm_list->push_back(new tree::LabelStm(L_end));

    visit_node = new tree::Seq(stm_list);
}


void ASTToTreeVisitor::visit(fdmj::While* node) {
    // 条件表达式
    node->exp->accept(*this);
    Tr_cx* cond_exp = visit_exp->unCx(temp_map);

    auto L1 = cond_exp->true_list;
    auto L2 = cond_exp->false_list;

    auto L_while = temp_map->newlabel();
    auto L_true = temp_map->newlabel();
    auto L_end = temp_map->newlabel();

    L1->patch(L_true);
    L2->patch(L_end);

    vector<tree::Stm*>* stm_list = new vector<tree::Stm*>();
    stm_list->push_back(new tree::LabelStm(L_while));
    stm_list->push_back(cond_exp->stm);

    stm_list->push_back(new tree::LabelStm(L_true));
    if (node->stm) {
        current_loop_start_label = L_while;
        current_loop_end_label = L_end;
        node->stm->accept(*this);
        auto stm = static_cast<tree::Stm*>(visit_node);
        stm_list->push_back(stm);
    }
    stm_list->push_back(new tree::Jump(L_while));
    stm_list->push_back(new tree::LabelStm(L_end));
    
    visit_node = new tree::Seq(stm_list);
}


void ASTToTreeVisitor::visit(fdmj::Assign* node) {
    // 首先访问左侧表达式
    node->left->accept(*this);
    tree::Exp* dst = visit_exp->unEx(temp_map)->exp;

    // 访问右侧表达式
    node->exp->accept(*this);
    tree::Exp* src = visit_exp->unEx(temp_map)->exp;

    visit_node = new tree::Move(dst, src);
}


void ASTToTreeVisitor::visit(fdmj::CallStm* node) {
    // 访问对象
    node->obj->accept(*this);
    tree::Exp* obj_exp = dynamic_cast<tree::Exp*>(visit_node);
    if (obj_exp == nullptr) {
        cerr << "Error: object expression is not a valid expression." << endl;
        visit_node = nullptr;
        return;
    }

    // 访问参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();
    if (node->par != nullptr) {
        for (auto& param : *(node->par)) {
            // 访问参数
            param->accept(*this);
            tree::Exp* arg_exp = dynamic_cast<tree::Exp*>(visit_node);
            
            if (arg_exp == nullptr) {
                cerr << "Error: parameter expression is not a valid expression." << endl;
                delete args;
                visit_node = nullptr;
                return;
            }
            
            args->push_back(arg_exp);
        }
    }

    // 创建调用节点，注意 CallStm 不关心返回值，所以类型设置为 INT
    tree::Call* call = new tree::Call(tree::Type::INT, node->name->id, obj_exp, args);

    // CallStm 是语句，不是表达式，所以需要包装在 ExpStm 中
    tree::ExpStm* exp_stm = new tree::ExpStm(call);
    visit_node = exp_stm;
}


void ASTToTreeVisitor::visit(fdmj::Continue* node) {
    // 检查是否在循环内
    if (current_loop_start_label == nullptr) {
        cerr << "Error: continue statement outside of loop." << endl;
        visit_node = nullptr;
        return;
    }

    // 创建跳转指令，跳转到循环开始标签
    tree::Jump* jump = new tree::Jump(current_loop_start_label);
    visit_node = jump;
}


void ASTToTreeVisitor::visit(fdmj::Break* node) {
    // 获取当前循环的结束标签
    if (current_loop_end_label == nullptr) {
        cerr << "Error: break statement outside of loop." << endl;
        visit_node = nullptr;
        return;
    }

    // 创建跳转指令，跳转到循环结束标签
    tree::Jump* jump = new tree::Jump(current_loop_end_label);
    visit_node = jump;
}


void ASTToTreeVisitor::visit(fdmj::Return* node) {
    // 访问返回值
    node->exp->accept(*this);
    tree::Exp* return_exp = visit_exp->unEx(temp_map)->exp;

    // 创建 Return 节点
    visit_node = new tree::Return(return_exp);
}


void ASTToTreeVisitor::visit(fdmj::PutInt* node) {
    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 访问整数表达式
    node->exp->accept(*this);
    tree::Exp* int_exp = dynamic_cast<tree::Exp*>(visit_node);
    if (int_exp == nullptr) {
        cerr << "Error: integer expression is not a valid expression." << endl;
        delete args;
        visit_node = nullptr;
        return;
    }
    args->push_back(int_exp);

    // 创建外部调用节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "putint", args);

    // putint 是语句，不是表达式
    tree::ExpStm* exp_stm = new tree::ExpStm(ext_call);
    visit_node = exp_stm;
}


void ASTToTreeVisitor::visit(fdmj::PutCh* node) {
    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 访问参数
    node->exp->accept(*this);
    tree::Exp* char_exp = dynamic_cast<tree::Exp*>(visit_node);
    if (char_exp == nullptr) {
        cerr << "Error: character expression is not a valid expression." << endl;
        delete args;
        visit_node = nullptr;
        return;
    }
    args->push_back(char_exp);

    // 创建外部调用节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "putch", args);

    // putch 是语句，不是表达式，所以需要包装在 ExpStm 中
    tree::ExpStm* exp_stm = new tree::ExpStm(ext_call);
    visit_node = exp_stm;
}


void ASTToTreeVisitor::visit(fdmj::PutArray* node) {
    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 访问 n 表达式 (打印数量)
    node->n->accept(*this);
    tree::Exp* size_exp = dynamic_cast<tree::Exp*>(visit_node);
    if (size_exp == nullptr) {
        cerr << "Error: size expression is not a valid expression." << endl;
        delete args;
        visit_node = nullptr;
        return;
    }
    args->push_back(size_exp);

    // 访问 arr 表达式 (数组)
    node->arr->accept(*this);
    tree::Exp* array_exp = dynamic_cast<tree::Exp*>(visit_node);
    if (array_exp == nullptr) {
        cerr << "Error: array expression is not a valid expression." << endl;
        delete args;
        visit_node = nullptr;
        return;
    }
    args->push_back(array_exp);

    // 创建外部调用节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "putarray", args);

    // putarray 是语句，不是表达式
    tree::ExpStm* exp_stm = new tree::ExpStm(ext_call);
    visit_node = exp_stm;
}


void ASTToTreeVisitor::visit(fdmj::Starttime* node) {
    // 创建空的参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 创建节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "starttime", args);
    visit_node = ext_call;
}


void ASTToTreeVisitor::visit(fdmj::Stoptime* node) {
    // 创建空的参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 创建节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "stoptime", args);
    visit_node = ext_call;
}


void ASTToTreeVisitor::visit(fdmj::BinaryOp* node) {
    // 获取语义信息
    AST_Semant* semant = semant_map->getSemant(node);
    if (semant == nullptr) {
        cerr << "Error: semantic information not found for the node." << endl;
        visit_node = nullptr;
        return;
    }
    
    // 确定结果类型
    tree::Type result_type;
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

    // 运算符
    string op = node->op->op;

    // 访问左操作数
    node->left->accept(*this);
    Tr_Exp* left_exp = visit_exp;
    
    // 访问右操作数
    node->right->accept(*this);
    Tr_Exp* right_exp = visit_exp;

    // 算数运算
    vector<string> algo_op = { "+", "-", "*", "/" };
    if (find(algo_op.begin(), algo_op.end(), op) != algo_op.end()) {
        auto left = left_exp->unEx(temp_map)->exp;
        auto right = right_exp->unEx(temp_map)->exp;
        visit_exp = new Tr_ex(new tree::Binop(result_type, op, left, right));
        return;
    }

    // 比较运算
    vector<string> logic_op = { "==", "!=", "<", ">", "<=", ">=" };
    if (find(logic_op.begin(), logic_op.end(), op) != logic_op.end()) {
        auto left = left_exp->unEx(temp_map)->exp;
        auto right = right_exp->unEx(temp_map)->exp;

        // 构造CJump
        Label* t = temp_map->newlabel();
        Label* f = temp_map->newlabel();
        auto cjump = new tree::Cjump(op, left, right, t, f);

        // 添加修补列表
        auto true_list = new Patch_list();
        auto false_list = new Patch_list();
        true_list->add_patch(t);
        false_list->add_patch(f);
        visit_exp = new Tr_cx(true_list, false_list, cjump);
        return;
    }

    // 逻辑或运算
    if (op == "||") {
        auto left_cx = left_exp->unCx(temp_map);        // 左表达式
        auto right_cx = right_exp->unCx(temp_map);      // 右表达式
        auto L1 = left_cx->true_list;
        auto L2 = left_cx->false_list;
        auto L3 = right_cx->true_list;
        auto L4 = right_cx->false_list;

        // 用L5填补L2
        auto L5 = temp_map->newlabel();
        L2->patch(L5);

        // 合并L1和L3作为新的正确分支
        L1->add(L3);

        vector<tree::Stm*>* sl = new vector<tree::Stm*>();
        sl->push_back(left_cx->stm);
        sl->push_back(new tree::LabelStm(L5));
        sl->push_back(right_cx->stm);
        visit_exp = new Tr_cx(L1, L4, new tree::Seq(sl));
        return;
    }

    // 逻辑与运算
    if (op == "&&") {
        auto left_cx = left_exp->unCx(temp_map);
        auto right_cx = right_exp->unCx(temp_map);
        auto L1 = left_cx->true_list;
        auto L2 = left_cx->false_list;
        auto L3 = right_cx->true_list;
        auto L4 = right_cx->false_list;

        // 用L5填补L1
        auto L5 = temp_map->newlabel();
        L1->patch(L5);

        // 合并L2和L4作为新的错误分支
        L2->add(L4);

        vector<tree::Stm*>* sl = new vector<tree::Stm*>();
        sl->push_back(left_cx->stm);
        sl->push_back(new tree::LabelStm(L5));
        sl->push_back(right_cx->stm);
        visit_exp = new Tr_cx(L3, L2, new tree::Seq(sl));
        return;
    }

    if (op == "&&") {
        auto left_cx = left_exp->unCx(temp_map);
        auto right_cx = right_exp->unCx(temp_map);
        auto L1 = left_cx->true_list;
        auto L2 = left_cx->false_list;
        auto L3 = right_cx->true_list;
        auto L4 = right_cx->false_list;

        // 用L5填补L1
        auto L5 = temp_map->newlabel();
        L1->patch(L5);

        // 合并L2和L4作为新的错误分支
        L2->add(L4);

        vector<tree::Stm*>* sl = new vector<tree::Stm*>();
        sl->push_back(left_cx->stm);
        sl->push_back(new tree::LabelStm(L5));
        sl->push_back(right_cx->stm);
        visit_exp = new Tr_cx(L3, L2, new tree::Seq(sl));
        return;
    }

    // 可能有异或
}


void ASTToTreeVisitor::visit(fdmj::UnaryOp* node) {
    // 访问操作数
    node->exp->accept(*this);
    tree::Exp* operand = dynamic_cast<tree::Exp*>(visit_node);
    if (operand == nullptr) {
        cerr << "Error: operand is not a valid expression." << endl;
        visit_node = nullptr;
        return;
    }

    // 获取语义信息
    AST_Semant* semant = semant_map->getSemant(node);
    if (semant == nullptr) {
        cerr << "Error: semantic information not found for the node." << endl;
        visit_node = nullptr;
        return;
    }

    tree::Type result_type;
    if (semant->get_type() == TypeKind::INT) {
        result_type = tree::Type::INT;
    } else {
        cerr << "Error: unexpected type for unary operation." << endl;
        visit_node = nullptr;
        return;
    }

    // 转换为二元操作
    if (node->op->op == "-") {
        // 负号：0 - exp
        tree::Const* zero = new tree::Const(0);
        tree::Binop* binop = new tree::Binop(result_type, "-", zero, operand);
        visit_node = binop;
    } 
    else if (node->op->op == "!") {
        // 逻辑非：1 xor exp
        tree::Const* one = new tree::Const(1);
        tree::Binop* binop = new tree::Binop(result_type, "xor", one, operand);
        visit_node = binop;
    }
    else {
        cerr << "Error: unsupported unary operator: " << node->op->op << endl;
        visit_node = nullptr;
    }

}


void ASTToTreeVisitor::visit(fdmj::ArrayExp* node) {
    // 访问数组，获取数组的基址
    node->arr->accept(*this);
    tree::Exp* arr_exp = dynamic_cast<tree::Exp*>(visit_node);
    if (arr_exp == nullptr) {
        cerr << "Error: array expression is not a valid expression." << endl;
        visit_node = nullptr;
        return;
    }

    // 访问索引
    node->index->accept(*this);
    tree::Exp* index_exp = dynamic_cast<tree::Exp*>(visit_node);
    if (index_exp == nullptr) {
        cerr << "Error: index expression is not a valid expression." << endl;
        visit_node = nullptr;
        return;
    }

    // 获取语义信息
    AST_Semant* semant = semant_map->getSemant(node);
    if (semant == nullptr) {
        cerr << "Error: semantic information not found for ArrayExp." << endl;
        visit_node = nullptr;
        return;
    }

    // 确定数组类型
    tree::Type element_type;      // 数组元素类型
    int element_size;           // 元素大小
    if (semant->get_type() == TypeKind::INT) {
        element_type = tree::Type::INT;
        element_size = compiler_config->get("int_length");
    }
    else if (semant->get_type() == TypeKind::ARRAY) {
        element_type = tree::Type::PTR;
        element_size = compiler_config->get("address_length");
    }
    else if (semant->get_type() == TypeKind::CLASS) {
        element_type = tree::Type::PTR;
        element_size = compiler_config->get("address_length");
    }
    else {
        cerr << "Error: unknown type for ArrayExp." << endl;
        visit_node = nullptr;
        return;
    }

    // 计算元素地址 array_base + index * element_size
    tree::Binop* addr_exp = new tree::Binop(tree::Type::PTR, "+", arr_exp, 
        new tree::Binop(tree::Type::INT, "*", index_exp, new tree::Const(element_size)));

    // 创建访存表达式
    tree::Mem* mem_exp = new tree::Mem(element_type, addr_exp);
    visit_node = mem_exp;
}

void ASTToTreeVisitor::visit(fdmj::CallExp* node) {
    // 访问对象
    node->obj->accept(*this);
    tree::Exp* obj_exp = dynamic_cast<tree::Exp*>(visit_node);
    if (obj_exp == nullptr) {
        cerr << "Error: object expression is not a valid expression." << endl;
        visit_node = nullptr;
        return;
    }

    // 获取语义信息
    AST_Semant* semant = semant_map->getSemant(node);
    if (semant == nullptr) {
        cerr << "Error: semantic information not found for CallExp." << endl;
        visit_node = nullptr;
        return;
    }

    // 确定返回类型
    tree::Type result_type;
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
        cerr << "Error: unknown type for CallExp." << endl;
        visit_node = nullptr;
        return;
    }

    // 参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    if (node->par != nullptr) {
        for (auto& arg : *node->par) {
            arg->accept(*this);
            tree::Exp* arg_exp = dynamic_cast<tree::Exp*>(visit_node);
            if (arg_exp == nullptr) {
                cerr << "Error: argument expression is not a valid expression." << endl;
                delete args;
                visit_node = nullptr;
                return;
            }
            args->push_back(arg_exp);
        }
    }

    // 创建节点
    tree::Call* call_node = new tree::Call(result_type, node->name->id, obj_exp, args);
}


void ASTToTreeVisitor::visit(fdmj::ClassVar* node) {
    // 访问对象，获取对象的地址
    node->obj->accept(*this);
    tree::Exp* obj_exp = dynamic_cast<tree::Exp*>(visit_node);
    if (obj_exp == nullptr) {
        cerr << "Error: object expression is not a valid expression." << endl;
        visit_node = nullptr;
        return;
    }

    // 获取语义信息
    AST_Semant* semant = semant_map->getSemant(node);
    if (semant == nullptr) {
        cerr << "Error: semantic information not found for ClassVar." << endl;
        visit_node = nullptr;
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
        visit_node = nullptr;
        return;
    }

    // 获取成员变量在类中的偏移量
    int offset = class_table->get_var_pos(node->id->id);

    // 计算变量地址：对象地址 + 偏移量
    tree::Exp* addr_exp = new tree::Binop(tree::Type::PTR, "+", obj_exp, new tree::Const(offset));
    
    // 创建访存表达式
    tree::Mem* mem_exp = new tree::Mem(var_type, addr_exp);
    visit_node = mem_exp;
}


void ASTToTreeVisitor::visit(fdmj::BoolExp* node) {
    int int_val = node->val ? 1 : 0;
    visit_exp = new Tr_ex(new tree::Const(int_val));
}


void ASTToTreeVisitor::visit(fdmj::This* node) {
    // 检查方法变量表是否为空
    if (method_var_table == nullptr) {
        cerr << "Error: method_var_table is null in This expression." << endl;
        visit_node = nullptr;
        return;
    }

    // 从方法变量表中获取 this 指针
    tree::Temp* this_temp;
    this_temp = method_var_table->get_var_temp("this");
    if (this_temp == nullptr) {
        cerr << "Error: 'this' variable not found in method_var_table." << endl;
        visit_node = nullptr;
        return;
    }

    // 创建 TempExp 节点
    tree::TempExp* this_exp = new tree::TempExp(tree::Type::PTR, this_temp);
    visit_node = this_exp;
}


void ASTToTreeVisitor::visit(fdmj::Length* node) {
    // 访问数组表达式
    node->exp->accept(*this);
    tree::Exp* array_exp = dynamic_cast<tree::Exp*>(visit_node);
    if (array_exp == nullptr) {
        cerr << "Error: array expression is not a valid expression." << endl;
        visit_node = nullptr;
        return;
    }

    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();
    args->push_back(array_exp);

    // 创建节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "length", args);
    visit_node = ext_call;
}


void ASTToTreeVisitor::visit(fdmj::Esc* node) {
    // 创建语句列表
    std::vector<tree::Stm*>* stm_list = new std::vector<tree::Stm*>();

    for (auto& stm : *node->sl) {
        // 访问每个语句
        stm->accept(*this);
        if (visit_node) {
            stm_list->push_back(static_cast<tree::Stm*>(visit_node));
        }
    }

    // 创建 Seq 节点
    tree::Seq* seq = new tree::Seq(stm_list);

    // 访问表达式
    node->exp->accept(*this);
    tree::Exp* exp = visit_exp->unEx(temp_map)->exp;

    // 获取语义信息
    AST_Semant* semant = semant_map->getSemant(node);
    if (semant == nullptr) {
        cerr << "Error: semantic information not found for Esc." << endl;
        return;
    }

    // 确定结果类型
    tree::Type result_type;
    if (semant->get_type() == TypeKind::INT) {
        result_type = tree::Type::INT;
    }
    else {
        result_type = tree::Type::PTR;
    }

    // 创建 Eseq 节点
    tree::Eseq* eseq = new tree::Eseq(result_type, seq, exp);
    visit_exp = new Tr_ex(eseq);
}


void ASTToTreeVisitor::visit(fdmj::GetInt* node) {
    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 创建节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "getint", args);
    visit_node = ext_call;
}


void ASTToTreeVisitor::visit(fdmj::GetCh* node) {
    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 创建节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "getch", args);
    visit_node = ext_call;
}


void ASTToTreeVisitor::visit(fdmj::GetArray* node) {
    // 访问数组表达式
    node->exp->accept(*this);
    tree::Exp* array_exp = dynamic_cast<tree::Exp*>(visit_node);
    if (array_exp == nullptr) {
        cerr << "Error: array expression is not a valid expression." << endl;
        visit_node = nullptr;
        return;
    }
    
    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();
    args->push_back(array_exp);

    // 创建节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "getarray", args);
    visit_node = ext_call;
}


void ASTToTreeVisitor::visit(fdmj::IdExp* node) {
    // 获取语义信息
    AST_Semant* semant = semant_map->getSemant(node);
    if (semant == nullptr) {
        cerr << "Error: semantic information not found for the node." << endl;
        visit_exp = nullptr;
        return;
    }

    // 语义信息种类
    AST_Semant::Kind kind = semant->get_kind();

    if (kind == AST_Semant::Kind::ClassName) {
        // 若是类名，在IR中通常不直接表示
        visit_exp = nullptr;
        return;
    }
    else if (kind == AST_Semant::Kind::MethodName) {
        // 若是方法名
        visit_exp = nullptr;
        return;
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
            visit_exp = new Tr_ex(temp_exp);
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
    
        visit_exp = new Tr_ex(mem_exp);
    }
    else {
        cerr << "Error: unknown semantic kind for IdExp." << endl;
        visit_exp = nullptr;
        return;
    }
}


void ASTToTreeVisitor::visit(fdmj::OpExp* node) {
    // 实际上不需要使用这个节点
}


void ASTToTreeVisitor::visit(fdmj::IntExp* node) {
    visit_exp = new Tr_ex(new tree::Const(node->val));
}