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
        if (class_name == "_^main^_") continue; // main类不需要添加到类表中

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
    ASTToTreeVisitor* visitor = new ASTToTreeVisitor(semant_map);
    prog->accept(*visitor);
    return visitor->getTree();
}


void ASTToTreeVisitor::visit(fdmj::Program* node) {
    // 函数申明列表
    vector<tree::FuncDecl*>* fdl = new vector<tree::FuncDecl*>();

    // 访问主方法
    node->main->accept(*this);
    fdl->push_back(static_cast<tree::FuncDecl*>(visit_node[0]));
    visit_node.clear();

    // 所有类声明
    for (auto& cd : *(node->cdl)) {
        cd->accept(*this);
        // 将类方法添加到函数声明列表
        for (auto& funcd : visit_node) {
            if (funcd) {
                fdl->push_back(static_cast<tree::FuncDecl*>(funcd));
            }
        }
        visit_node.clear();
    }


    // 创建程序节点
    tree_root = new tree::Program(fdl);
}


void ASTToTreeVisitor::visit(fdmj::MainMethod* node) {
    class_name = "_^main^_";
    method_name = "main";
    
    // 创建新的临时变量映射表和方法变量表
    temp_map = new tree::Temp_map();
    method_var_table = generate_method_var_table("_^main^_", "main", semant_map->getNameMaps(), temp_map);
    
    vector<tree::Temp*>* args = new std::vector<tree::Temp*>();    // 参数列表
    vector<tree::Stm*>* stm_list = new std::vector<tree::Stm*>();  // 语句列表

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

    // 创建 blocks
    tree::Block* block = new tree::Block(entry_label, nullptr, stm_list);
    std::vector<tree::Block*>* blocks = new std::vector<tree::Block*>();
    blocks->push_back(block);
    
    // 记录临时变量和标签的最大编号
    int last_temp_num = temp_map->next_temp - 1;
    int last_label_num = temp_map->next_label - 1;

    // 
    visit_node.push_back(new tree::FuncDecl(
        class_name + "^" + method_name,
        args,
        blocks,
        tree::Type::INT,
        last_temp_num,
        last_label_num
    ));
}


void ASTToTreeVisitor::visit(fdmj::ClassDecl* node) {
    class_name = node->id->id;

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

void ASTToTreeVisitor::visit(fdmj::MethodDecl* node) {
    // 更新当前方法名
    method_name = node->id->id;

    // 重新分配临时变量映射表
    delete temp_map;
    temp_map = new tree::Temp_map();

    // 为当前函数生成方法变量表
    this_temp = new tree::TempExp(tree::Type::PTR, temp_map->newtemp());
    method_var_table = generate_method_var_table(class_name, method_name, semant_map->getNameMaps(), temp_map);

    // 形参列表 (第一个参数为 this 指针)
    vector<tree::Temp*>* args = new vector<tree::Temp*> { this_temp->temp };
    auto formal_list = semant_map->getNameMaps()->get_method_formal_list(class_name, method_name);
    for (auto formal_name : *formal_list)
        args->push_back(method_var_table->get_var_temp(formal_name));

    // 语句列表
    vector<tree::Stm*>* sl = new vector<tree::Stm*>();

    // 变量声明
    for (fdmj::VarDecl* vd : *node->vdl) {
        vd->accept(*this);
        for (auto& stm : visit_node)
            if (stm)
                sl->push_back(static_cast<tree::Stm*>(stm));
        visit_node.clear();
    }

    // 语句列表
    for (fdmj::Stm* stm : *node->sl) {
        stm->accept(*this);
        for (auto& node : visit_node)
            if (node)
                sl->push_back(static_cast<tree::Stm*>(node));
        visit_node.clear();
    }

    // 在开头插入返回标签
    Label* entry_label = temp_map->newlabel();
    sl->insert(sl->begin(), new tree::LabelStm(entry_label));

    // 构造 blocks
    tree::Block* block = new tree::Block(entry_label, nullptr, sl);
    vector<tree::Block*>* blocks = new vector<tree::Block*>();
    blocks->push_back(block);

    int last_temp_num = temp_map->next_temp - 1;
    int last_label_num = temp_map->next_label - 1;
    auto return_type = node->type->typeKind == TypeKind::INT ? tree::Type::INT : tree::Type::PTR;
    
    visit_node.push_back(new tree::FuncDecl(class_name + "^" + method_name, args, blocks, return_type, last_temp_num, last_label_num));
}


// 辅助函数 (用于数组初始化)
vector<tree::Stm*>* array_declarer(fdmj::VarDecl* node, tree::TempExp* array_temp) {
    auto type = node->type;
    auto init = node->init;

    auto sl = new vector<tree::Stm*>();

    // 计算数组大小
    int size = 0;
    if (type->arity && type->arity->val != 0)
        size = type->arity->val;
    else if (holds_alternative<vector<IntExp*>*>(init))
        size = get<vector<IntExp*>*>(init)->size();

    // 分配数组空间
    auto malloc_args = new vector<tree::Exp*> { new tree::Const((size + 1) * Compiler_Config::get("int_length")) };
    auto malloc_call = new tree::ExtCall(tree::Type::PTR, "malloc", malloc_args);
    sl->push_back(new tree::Move(array_temp, malloc_call));
    
    // 填入数组长度
    auto length_mem =  new tree::Mem(tree::Type::INT, array_temp);
    sl->push_back(new tree::Move(length_mem, new tree::Const(size)));

    // 初始化
    if (holds_alternative<vector<IntExp*>*>(init)) {
        auto init_list = get<vector<IntExp*>*>(init);
        for (int i = 0; i < init_list->size(); ++i) {
            auto val = (*init_list)[i]->val;
            auto off = new tree::Const((i + 1) * Compiler_Config::get("int_length"));
            auto elem_mem = new tree::Mem(tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", array_temp, off));
            sl->push_back(new tree::Move(elem_mem, new tree::Const(val)));
        }
    }

    return sl;
}


void ASTToTreeVisitor::visit(fdmj::VarDecl* node) {

    auto type = node->type;
    auto id = node->id;
    auto init = node->init;
    auto temp = method_var_table->get_var_temp(id->id);
    
    // 整型
    if (node->type->typeKind == TypeKind::INT) {
        if (holds_alternative<fdmj::IntExp*>(init)) {
            int val = get<fdmj::IntExp*>(init)->val;
            tree::TempExp* int_temp = new tree::TempExp(tree::Type::INT, temp);
            visit_node.push_back(new tree::Move(int_temp, new tree::Const(val)));
        }
    }

    // 数组
    else if (node->type->typeKind == TypeKind::ARRAY) {
        auto array_temp = new tree::TempExp(tree::Type::PTR, temp);
        auto init_stms = array_declarer(node, array_temp);
        visit_node.insert(visit_node.end(), init_stms->begin(), init_stms->end());
    }
    
    // 类
    else if (node->type->typeKind == TypeKind::CLASS) {
        string class_name = node->type->cid->id;
        auto class_var_list = semant_map->getNameMaps()->get_class_var_list(class_name);
        auto class_method_list = semant_map->getNameMaps()->get_method_list(class_name);

        // 分配空间
        auto class_temp = new tree::TempExp(tree::Type::PTR, temp);
        auto class_size = class_table->table_size;
        auto malloc_args = new vector<tree::Exp*> { new tree::Const(class_size) };
        auto malloc_call = new tree::ExtCall(tree::Type::PTR, "malloc", malloc_args);
        visit_node.push_back(new tree::Move(class_temp, malloc_call));

        // 初始化类成员变量
        auto namemaps = semant_map->getNameMaps();
        for (auto var_name : *class_var_list) {
            auto off = class_table->get_var_pos(var_name);
            auto decl = namemaps->get_class_var(class_name, var_name);

            if (decl->type->typeKind == TypeKind::INT) {
                if (holds_alternative<fdmj::IntExp*>(decl->init)) {
                    int val = get<fdmj::IntExp*>(decl->init)->val;

                    auto val_temp = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
                    auto val_move = new tree::Move(val_temp, new tree::Const(val));
                    
                    auto var_mem = new tree::Mem(tree::Type::PTR, new tree::Binop(tree::Type::PTR, "+", class_temp, new tree::Const(off)));
                    auto val_eseq = new tree::Eseq(tree::Type::INT, new tree::Seq(new vector<tree::Stm*> { val_move }), val_temp);
                    visit_node.push_back(new tree::Move(var_mem, val_eseq));
                }
            }

            else if (decl->type->typeKind == TypeKind::ARRAY) {
                // 数组初始化
                auto arr_temp = new tree::TempExp(tree::Type::PTR, temp_map->newtemp());
                auto init_stms = array_declarer(decl, arr_temp);
                auto array_init = new tree::Eseq(tree::Type::PTR, new tree::Seq(init_stms), arr_temp);
                // 然后把数组地址存入成员变量
                auto var_mem = new tree::Mem(tree::Type::PTR, new tree::Binop(tree::Type::PTR, "+", class_temp, new tree::Const(off)));
                visit_node.push_back(new tree::Move(var_mem, array_init));
            }
        }

        // 初始化类成员方法
        for (auto method_name : *class_method_list) {

            auto off = class_table->get_method_pos(method_name);
            string method_real_class = class_name;

            // 找到第一个return_formal不同的 (如果是继承，那么子类和父类共用同一个 return formal 结点)
            auto current_class_name = class_name;
            auto parent_class_name = namemaps->get_parent(class_name);
            while (parent_class_name != "") {
                auto cur_f_return = namemaps->get_method_return_formal(current_class_name, method_name);
                auto par_f_return = namemaps->get_method_return_formal(parent_class_name, method_name);

                if (par_f_return != cur_f_return)
                    break;

                method_real_class = parent_class_name;
                current_class_name = parent_class_name;
                parent_class_name = namemaps->get_parent(current_class_name);
            }

            auto method_mem = new tree::Mem(tree::Type::PTR, new tree::Binop(tree::Type::PTR, "+", class_temp, new tree::Const(off)));
            auto method_nameExp = new Name(temp_map->newstringlabel(method_real_class + "^" + method_name));
            visit_node.push_back(new tree::Move(method_mem, method_nameExp));
        }
    }
}


void ASTToTreeVisitor::visit(fdmj::Formal* node) { }


void ASTToTreeVisitor::visit(fdmj::Nested* node) {
    // 创建新的语句列表
    std::vector<tree::Stm*>* stm_list = new std::vector<tree::Stm*>();

    for (auto& stm : *(node->sl)) {
        // 访问子语句
        stm->accept(*this);
        for (auto& stm : visit_node) {
            if (stm) {
                // 将子语句添加到语句列表中
                stm_list->push_back(static_cast<tree::Stm*>(stm));
            }
        }
        visit_node.clear();
    }

    // 创建语句序列
    visit_node.push_back(new tree::Seq(stm_list));
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
    tree::Stm* stm1 = static_cast<tree::Stm*>(visit_node[0]);
    visit_node.clear();

    stm_list->push_back(stm1);
    stm_list->push_back(new tree::Jump(L_end));

    stm_list->push_back(new tree::LabelStm(L_false));
    if (node->stm2) {
        node->stm2->accept(*this);
        tree::Stm* stm2 = static_cast<tree::Stm*>(visit_node[0]);
        visit_node.clear();

        stm_list->push_back(stm2);
    }

    stm_list->push_back(new tree::LabelStm(L_end));

    visit_node.push_back(new tree::Seq(stm_list));
}


// 辅助函数 (用于生成 while 循环)
vector<tree::Stm*>* while_generator(tree::Temp_map* temp_map, Tr_Exp* exp, vector<tree::Stm*>& body) {
    /**
     * While:
     *   L_while:
     *   cond_exp.stm
     *   L_true:
     *   body
     *   Jump L_while
     *   L_end:
     */
    auto cond_exp = exp->unCx(temp_map);
    auto tl = cond_exp->true_list;
    auto fl = cond_exp->false_list;

    auto L_while = temp_map->newlabel();
    auto L_true = temp_map->newlabel();
    auto L_end = temp_map->newlabel();

    tl->patch(L_true);
    fl->patch(L_end);

    vector<tree::Stm*>* sl = new vector<tree::Stm*>();
    sl->push_back(new tree::LabelStm(L_while));
    sl->push_back(cond_exp->stm);

    sl->push_back(new tree::LabelStm(L_true));
    for (auto& stm : body)
        sl->push_back(stm);
    sl->push_back(new tree::Jump(L_while));
    sl->push_back(new tree::LabelStm(L_end));
    return sl;
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
        loop_start_label = L_while;
        loop_end_label = L_end;
        node->stm->accept(*this);
        auto stm = static_cast<tree::Stm*>(visit_node[0]);
        visit_node.clear();
        stm_list->push_back(stm);
    }
    stm_list->push_back(new tree::Jump(L_while));
    stm_list->push_back(new tree::LabelStm(L_end));
    
    visit_node.push_back(new tree::Seq(stm_list));
}


void ASTToTreeVisitor::visit(fdmj::Assign* node) {
    // 首先访问左侧表达式
    node->left->accept(*this);
    tree::Exp* dst = visit_exp->unEx(temp_map)->exp;

    // 访问右侧表达式
    node->exp->accept(*this);
    tree::Exp* src = visit_exp->unEx(temp_map)->exp;

    visit_node.push_back(new tree::Move(dst, src));
}


void ASTToTreeVisitor::visit(fdmj::CallStm* node) {
    // 访问对象
    node->obj->accept(*this);
    tree::Exp* obj_exp = visit_exp->unEx(temp_map)->exp;


    // 访问参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*> { obj_exp };
    for (auto& arg : *(node->par)) {
        arg->accept(*this);
        tree::Exp* arg_exp = visit_exp->unEx(temp_map)->exp;
        args->push_back(arg_exp);
    }

    // 被调用函数的类名和方法名
    auto call_class_name = get<string>(semant_map->getSemant(node->obj)->get_type_par());
    auto call_method_name = node->name->id;

    // 返回类型
    auto return_formal = semant_map->getNameMaps()->get_method_return_formal(call_class_name, call_method_name);
    auto return_type = return_formal->type->typeKind == TypeKind::INT ? tree::Type::INT : tree::Type::PTR;

    // 方法调用
    int off = class_table->get_method_pos(call_method_name);
    auto method_mem = new tree::Mem(tree::Type::PTR, new tree::Binop(tree::Type::PTR, "+", obj_exp, new tree::Const(off)));
    auto method_call = new tree::Call(return_type, call_method_name, method_mem, args);

    visit_node.push_back(new tree::ExpStm(method_call));
}


void ASTToTreeVisitor::visit(fdmj::Continue* node) {
    visit_node.push_back(new tree::Jump(loop_start_label));
}


void ASTToTreeVisitor::visit(fdmj::Break* node) {
    visit_node.push_back(new tree::Jump(loop_end_label));
}


void ASTToTreeVisitor::visit(fdmj::Return* node) {
    node->exp->accept(*this);
    tree::Exp* return_exp = visit_exp->unEx(temp_map)->exp;
    visit_node.push_back(new tree::Return(return_exp));
}


void ASTToTreeVisitor::visit(fdmj::PutInt* node) {
    node->exp->accept(*this);
    tree::Exp* int_exp = visit_exp->unEx(temp_map)->exp;

    // 参数列表
    vector<tree::Exp*>* args = new std::vector<tree::Exp*>();
    args->push_back(int_exp);

    // 外部调用
    // putint 是语句，不是表达式，所以需要包装在 ExpStm 中
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "putint", args);
    visit_node.push_back(new tree::ExpStm(ext_call));
}


void ASTToTreeVisitor::visit(fdmj::PutCh* node) {
    node->exp->accept(*this);
    tree::Exp* char_exp = visit_exp->unEx(temp_map)->exp;

    // 参数列表
    vector<tree::Exp*>* args = new std::vector<tree::Exp*>();
    args->push_back(char_exp);

    // 外部调用
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "putch", args);
    visit_node.push_back(new tree::ExpStm(ext_call));
}

// putarray(长度表达式, 数组表达式);
void ASTToTreeVisitor::visit(fdmj::PutArray* node) {
    // 长度
    node->n->accept(*this);
    tree::Exp* size_exp = visit_exp->unEx(temp_map)->exp;
    
    // 数组
    node->arr->accept(*this);
    tree::Exp* array_exp = visit_exp->unEx(temp_map)->exp;
    
    // 参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();
    args->push_back(size_exp);
    args->push_back(array_exp);

    // 外部调用
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "putarray", args);
    visit_node.push_back(new tree::ExpStm(ext_call));
}


void ASTToTreeVisitor::visit(fdmj::Starttime* node) {
    vector<tree::Exp*>* args = new std::vector<tree::Exp*>();
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "starttime", args);
    visit_node.push_back(new tree::ExpStm(ext_call));
}


void ASTToTreeVisitor::visit(fdmj::Stoptime* node) {
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "stoptime", args);
    visit_node.push_back(new tree::ExpStm(ext_call));
}

// 辅助函数 (用于比较)
Tr_cx* compare_generator(Temp_map* temp_map, tree::Exp* left, tree::Exp* right, string op)
{
    // CJump
    Label* t = temp_map->newlabel();
    Label* f = temp_map->newlabel();
    auto cjump = new tree::Cjump(op, left, right, t, f);

    // 回填列表
    auto true_list = new Patch_list();
    auto false_list = new Patch_list();
    true_list->add_patch(t);
    false_list->add_patch(f);
    return new Tr_cx(true_list, false_list, cjump);
}


void ASTToTreeVisitor::visit(fdmj::BinaryOp* node) {
    // 运算符
    string op = node->op->op;

    // 访问左操作数
    node->left->accept(*this);
    Tr_Exp* left_exp = visit_exp;
    
    // 访问右操作数
    node->right->accept(*this);
    Tr_Exp* right_exp = visit_exp;

    vector<string> algo_op = { "+", "-", "*", "/" };
    vector<string> logic_op = { "==", "!=", "<", ">", "<=", ">=" };

    // 算数运算
    if (find(algo_op.begin(), algo_op.end(), op) != algo_op.end()) {
        auto left = left_exp->unEx(temp_map)->exp;
        auto right = right_exp->unEx(temp_map)->exp;

        // Int
        if (left->type == tree::Type::INT && right->type == tree::Type::INT) {
            visit_exp = new Tr_ex(new tree::Binop(tree::Type::INT, op, left, right));
        }

        // Array
        else if (left->type == tree::Type::PTR && right->type == tree::Type::PTR) {
            vector<tree::Stm*>* sl = new vector<tree::Stm*>();

            // 获取 left 长度
            auto left_size_temp = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
            auto left_size_mem = new tree::Mem(tree::Type::INT, left);
            sl->push_back(new tree::Move(left_size_temp, left_size_mem));

            // 获取 right 长度
            auto right_size_temp = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
            auto right_size_mem = new tree::Mem(tree::Type::INT, right);
            sl->push_back(new tree::Move(right_size_temp, right_size_mem));

            // left.size != right.size
            Tr_cx* size_neq = compare_generator(temp_map, left_size_temp, right_size_temp, "!=");

            // exit(-1)
            auto args = new vector<tree::Exp*> { new tree::Const(-1) };
            auto exit_stm = new tree::ExpStm(new tree::ExtCall(tree::Type::INT, "exit", args));

            // if left.size != right.size exit(-1)
            auto tl = size_neq->true_list;
            auto fl = size_neq->false_list;
            auto L_true = temp_map->newlabel();
            auto L_false = temp_map->newlabel();
            tl->patch(L_true);
            fl->patch(L_false);
            sl->push_back(size_neq->stm);
            sl->push_back(new tree::LabelStm(L_true));
            sl->push_back(exit_stm);
            sl->push_back(new tree::LabelStm(L_false));

            // 结果数组 int[left.size] t
            auto int_length = Compiler_Config::get("int_length");
            auto t_temp = new tree::TempExp(tree::Type::PTR, temp_map->newtemp());
            auto t_size = new tree::Binop(
                tree::Type::INT, "*", new tree::Binop(tree::Type::INT, "+", left_size_temp, new tree::Const(1)), new tree::Const(int_length));
            auto t_malloc = new tree::ExtCall(tree::Type::PTR, "malloc", new vector<tree::Exp*> { t_size });
            sl->push_back(new tree::Move(t_temp, t_malloc));

            // 设置结果数组大小 t.size = left.size
            auto t_size_mem = new tree::Mem(tree::Type::INT, t_temp);
            sl->push_back(new tree::Move(t_size_mem, left_size_temp));

            // i = int_length
            auto i_temp = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
            sl->push_back(new tree::Move(i_temp, new tree::Const(int_length)));

            // end = (left.size + 1) * 4
            auto end_temp = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
            auto end_plus = new tree::Binop(
                tree::Type::INT, "*", new tree::Binop(tree::Type::INT, "+", left_size_temp, new tree::Const(1)), new tree::Const(int_length));
            sl->push_back(new tree::Move(end_temp, end_plus));

            /**
             * while( i < end) {
             *     *(t+i) = *(a+i) + *(b+i);
             *     i = i + int_length;
             * }
             */

            auto while_cond = compare_generator(temp_map, i_temp, end_temp, "<");

            vector<tree::Stm*> body;

            // *(t+i) = *(a+i) + *(b+i);
            auto t_i_mem = new tree::Mem(tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", t_temp, i_temp));
            auto a_i_mem = new tree::Mem(tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", left, i_temp));
            auto b_i_mem = new tree::Mem(tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", right, i_temp));
            auto t_plus = new tree::Binop(tree::Type::INT, op, a_i_mem, b_i_mem);
            body.push_back(new tree::Move(t_i_mem, t_plus));

            // i = i + int_length
            auto i_plus = new tree::Binop(tree::Type::INT, "+", i_temp, new tree::Const(int_length));
            body.push_back(new tree::Move(i_temp, i_plus));

            // 生成 while 循环
            auto while_stms = while_generator(temp_map, while_cond, body);
            for (auto& stm : *while_stms)
                sl->push_back(stm);
            visit_exp = new Tr_ex(new tree::Eseq(tree::Type::INT, new tree::Seq(sl), t_temp));
        }
        else {
            cerr << "Error: invalid types for binary operation." << endl;
            return;
        }
    }

    // 逻辑运算
    else if (find(logic_op.begin(), logic_op.end(), op) != logic_op.end()) {
        auto left = left_exp->unEx(temp_map)->exp;
        auto right = right_exp->unEx(temp_map)->exp;
        visit_exp = compare_generator(temp_map, left, right, op);
    }

    // 或
    else if (op == "||") {
        auto left_cx = left_exp->unCx(temp_map);
        auto right_cx = right_exp->unCx(temp_map);
        auto L1 = left_cx->true_list;
        auto L2 = left_cx->false_list;
        auto L3 = right_cx->true_list;
        auto L4 = right_cx->false_list;

        // 用 L5 回填 L2
        auto L5 = temp_map->newlabel();
        L2->patch(L5);

        // 合并L1和L3作为新的正确分支
        L1->add(L3);

        vector<tree::Stm*>* sl = new vector<tree::Stm*>();
        sl->push_back(left_cx->stm);
        sl->push_back(new tree::LabelStm(L5));
        sl->push_back(right_cx->stm);
        visit_exp = new Tr_cx(L1, L4, new tree::Seq(sl));
    }

    // 与
    else if (op == "&&") {
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
    }
}


void ASTToTreeVisitor::visit(fdmj::UnaryOp* node) {
    string op = node->op->op;

    node->exp->accept(*this);
    auto exp_tr = visit_exp;

    if (op == "-") {
        tree::Exp* exp = exp_tr->unEx(temp_map)->exp;

        // 整型
        if (exp->type == tree::Type::INT) {
            visit_exp = new Tr_ex(new tree::Binop(tree::Type::INT, "-", new tree::Const(0), exp));
        }

        // 整型数组
        else if (exp->type == tree::Type::PTR) {
            vector<tree::Stm*>* sl = new vector<tree::Stm*>();

            // 获取数组大小 size
            auto size_temp = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
            auto size_mem = new tree::Mem(tree::Type::INT, exp);
            sl->push_back(new tree::Move(size_temp, size_mem));

            // 结果数组 t
            auto int_length = Compiler_Config::get("int_length");
            auto t_temp = new tree::TempExp(tree::Type::PTR, temp_map->newtemp());
            auto t_size_binop = new tree::Binop(
                tree::Type::INT, "*", new tree::Binop(tree::Type::INT, "+", size_temp, new tree::Const(1)), new tree::Const(int_length));
            auto t_malloc = new tree::ExtCall(tree::Type::PTR, "malloc", new vector<tree::Exp*> { t_size_binop });
            sl->push_back(new tree::Move(t_temp, t_malloc));

            // 设置数组 t 大小为 size
            auto t_size_mem = new tree::Mem(tree::Type::INT, t_temp);
            sl->push_back(new tree::Move(t_size_mem, size_temp));

            // 初始化下标索引 i = int_length
            auto i_temp = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
            auto i_const = new tree::Const(int_length);
            sl->push_back(new tree::Move(i_temp, i_const));

            // 初始化边界 end = (size + 1) * 4
            auto end_temp = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
            auto end_binop = new tree::Binop(
                tree::Type::INT, "*", new tree::Binop(tree::Type::INT, "+", size_temp, new tree::Const(1)), new tree::Const(int_length));
            sl->push_back(new tree::Move(end_temp, end_binop));

            /**
             * while(i<end) {
             *     *(t+i) = 0 - *(a+i)
             *     i=i+int_length
             * }
             */
            auto while_cond = compare_generator(temp_map, i_temp, end_temp, "<");
            vector<tree::Stm*> body;

            // *(t+i) = 0 - *(a+i);
            auto t_i_mem = new tree::Mem(tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", t_temp, i_temp));
            auto a_i_mem = new tree::Mem(tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", exp, i_temp));
            auto t_minus = new tree::Binop(tree::Type::INT, "-", new tree::Const(0), a_i_mem);
            body.push_back(new tree::Move(t_i_mem, t_minus));

            // i = i + int_length
            auto i_plus = new tree::Binop(tree::Type::INT, "+", i_temp, new tree::Const(int_length));
            body.push_back(new tree::Move(i_temp, i_plus));

            // 生成 while 循环，合并到 sl
            auto while_stms = while_generator(temp_map, while_cond, body);
            for (auto& stm : *while_stms)
                sl->push_back(stm);
            visit_exp = new Tr_ex(new tree::Eseq(tree::Type::PTR, new tree::Seq(sl), t_temp));
        }

    } else if (op == "!") {
        auto exp = exp_tr->unCx(temp_map);
        auto tl = exp->true_list;
        auto fl = exp->false_list;

        // 交换 tl 和 fl
        visit_exp = new Tr_cx(fl, tl, exp->stm);
    }

}


void ASTToTreeVisitor::visit(fdmj::ArrayExp* node) {

    bool array_pre = false;     // 是否需要对数组求值
    bool index_pre = false;     // 是否需要对下标求值
    
    node->arr->accept(*this);
    tree::Exp* array_exp = visit_exp->unEx(temp_map)->exp;
    tree::Exp* array_temp = nullptr;
    if (array_exp->getTreeKind() == Kind::TEMPEXP) {
        array_temp = array_exp;
    } else {
        // 如果数组不是值, 则需要先对数组求值
        array_temp = new tree::TempExp(tree::Type::PTR, temp_map->newtemp());
        array_pre = true;
    }

    node->index->accept(*this);
    tree::Exp* index_exp = visit_exp->unEx(temp_map)->exp;
    tree::Exp* index_temp = nullptr;
    if (index_exp->getTreeKind() == Kind::TEMPEXP || index_exp->getTreeKind() == Kind::CONST) {
        index_temp = index_exp;
    } else {
        // 如果下标不是值, 则需要先对下标求值
        index_temp = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
        index_pre = true;
    }

    // 越界检查
    vector<tree::Stm*>* check_sl = new vector<tree::Stm*>();

    // 取出 size
    tree::TempExp* size_temp = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
    tree::Move* size_move = new tree::Move(size_temp, new tree::Mem(tree::Type::INT, array_temp));
    check_sl->push_back(size_move);

    // index >= size
    auto size_check = compare_generator(temp_map, index_temp, size_temp, ">=");

    // exit(-1)
    auto args = new vector<tree::Exp*> { new tree::Const(-1) };
    auto exit_stm = new tree::ExpStm(new tree::ExtCall(tree::Type::INT, "exit", args));

    // if index >= size exit(-1)
    auto tl = size_check->true_list;
    auto fl = size_check->false_list;
    auto L_true = temp_map->newlabel();
    auto L_false = temp_map->newlabel();
    tl->patch(L_true);
    fl->patch(L_false);
    check_sl->push_back(size_check->stm);
    check_sl->push_back(new tree::LabelStm(L_true));
    check_sl->push_back(exit_stm);
    check_sl->push_back(new tree::LabelStm(L_false));

    // 索引 i
    auto i_exp = new tree::Eseq(tree::Type::INT, new tree::Seq(check_sl), index_temp);
    auto i_plus_1 = new tree::Binop(tree::Type::INT, "+", i_exp, new tree::Const(1));
    auto i_off = new tree::Binop(tree::Type::INT, "*", i_plus_1, new tree::Const(Compiler_Config::get("int_length")));

    // *(array + (i + 1) * 4)
    auto array_off = new tree::Binop(tree::Type::PTR, "+", array_temp, i_off);
    auto array_mem = new tree::Mem(tree::Type::INT, array_off);

    // 如果数组和索引都是值, 则直接返回
    if (!array_pre && !index_pre) {
        visit_exp = new Tr_ex(array_mem);
    }
    // 否则先求值
    else {
        auto pre_sl = new vector<tree::Stm*>();
        if (array_pre)
            pre_sl->push_back(new tree::Move(array_temp, array_exp));
        if (index_pre)
            pre_sl->push_back(new tree::Move(index_temp, index_exp));
        visit_exp = new Tr_ex(new tree::Eseq(tree::Type::INT, new tree::Seq(pre_sl), array_mem));
    }
}

void ASTToTreeVisitor::visit(fdmj::CallExp* node) {
    auto call_class_name = get<string>(semant_map->getSemant(node->obj)->get_type_par());
    auto call_method_name = node->name->id;

    node->obj->accept(*this);
    tree::Exp* obj_exp = visit_exp->unEx(temp_map)->exp;

    vector<tree::Exp*>* args = new vector<tree::Exp*> { obj_exp };
    for (auto& arg : *(node->par)) {
        arg->accept(*this);
        tree::Exp* arg_exp = visit_exp->unEx(temp_map)->exp;
        args->push_back(arg_exp);
    }

    // 返回类型
    auto return_formal = semant_map->getNameMaps()->get_method_return_formal(call_class_name, call_method_name);
    auto return_type = return_formal->type->typeKind == TypeKind::INT ? tree::Type::INT : tree::Type::PTR;

    // 方法调用
    int off = class_table->get_method_pos(call_method_name);
    auto method_mem = new tree::Mem(tree::Type::PTR, new tree::Binop(tree::Type::PTR, "+", obj_exp, new tree::Const(off)));
    auto method_call = new tree::Call(return_type, call_method_name, method_mem, args);

    visit_exp = new Tr_ex(method_call);
}


void ASTToTreeVisitor::visit(fdmj::ClassVar* node) {
    node->obj->accept(*this);
    tree::Exp* obj_exp = visit_exp->unEx(temp_map)->exp;

    auto obj_class_name = get<string>(semant_map->getSemant(node->obj)->get_type_par());

    auto var_name = node->id->id;
    auto var_decl = semant_map->getNameMaps()->get_class_var(obj_class_name, var_name);
    auto var_type = var_decl->type->typeKind == TypeKind::INT ? tree::Type::INT : tree::Type::PTR;
    auto var_off = class_table->get_var_pos(var_name);
    auto var_mem = new tree::Mem(var_type, new tree::Binop(tree::Type::PTR, "+", obj_exp, new tree::Const(var_off)));

    visit_exp = new Tr_ex(var_mem);
}


void ASTToTreeVisitor::visit(fdmj::BoolExp* node) {
    int int_val = node->val ? 1 : 0;
    visit_exp = new Tr_ex(new tree::Const(int_val));
}


void ASTToTreeVisitor::visit(fdmj::This* node) {
    visit_exp = new Tr_ex(this_temp);
}


void ASTToTreeVisitor::visit(fdmj::Length* node) {
    node->exp->accept(*this);
    auto array_temp = visit_exp->unEx(temp_map)->exp;

    tree::TempExp* size_temp = new tree::TempExp(tree::Type::INT, temp_map->newtemp());
    tree::Move* size_move = new tree::Move(size_temp, new tree::Mem(tree::Type::INT, array_temp));

    visit_exp = new Tr_ex(new tree::Eseq(tree::Type::INT, size_move, size_temp));
}


void ASTToTreeVisitor::visit(fdmj::Esc* node) {
    vector<tree::Stm*>* sl = new vector<tree::Stm*>();
    for (fdmj::Stm* stm_node : *node->sl) {
        stm_node->accept(*this);
        for (auto stm : visit_node)
            if (stm)
                sl->push_back(static_cast<tree::Stm*>(stm));
        visit_node.clear();
    }

    node->exp->accept(*this);
    auto exp = visit_exp->unEx(temp_map)->exp;

    visit_exp = new Tr_ex(new tree::Eseq(tree::Type::INT, new tree::Seq(sl), exp));
}


void ASTToTreeVisitor::visit(fdmj::GetInt* node) {
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "getint", args);
    visit_exp = new Tr_ex(ext_call);
}


void ASTToTreeVisitor::visit(fdmj::GetCh* node) {
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "getch", args);
    visit_exp = new Tr_ex(ext_call);
}


void ASTToTreeVisitor::visit(fdmj::GetArray* node) {
    node->exp->accept(*this);
    auto args = new std::vector<tree::Exp*> { visit_exp->unEx(temp_map)->exp };
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "getarray", args);
    visit_exp = new Tr_ex(ext_call);
    
}


void ASTToTreeVisitor::visit(fdmj::IdExp* node) {
    tree::Type type = method_var_table->get_var_type(node->id);
    tree::Temp* temp = method_var_table->get_var_temp(node->id);
    visit_exp = new Tr_ex(new tree::TempExp(type, temp));
}


void ASTToTreeVisitor::visit(fdmj::OpExp* node) { }


void ASTToTreeVisitor::visit(fdmj::IntExp* node) {
    visit_exp = new Tr_ex(new tree::Const(node->val));
}