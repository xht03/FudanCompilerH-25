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

// 地址计算 需要参考 config.hh


void ASTToTreeVisitor::visit(fdmj::Program* node) {
    // 初始化函数声明列表
    func_decl_list = new std::vector<tree::FuncDecl*>();

    // 访问主方法，生成主函数的 FuncDecl
    if (node->main != nullptr) {
        node->main->accept(*this);
        tree::FuncDecl* main_func = dynamic_cast<tree::FuncDecl*>(visit_tree_result);
        if (main_func != nullptr) {
            func_decl_list->push_back(main_func);
        } else {
            cerr << "Warning: Main method did not generate a valid function declaration." << endl;
        }
    }

    // 访问所有类声明，生成所有方法的 FuncDecl
    if (node->cdl != nullptr && !node->cdl->empty()) {
        for (auto& class_decl : *(node->cdl)) {
            class_decl->accept(*this);
            // 注意：ClassDecl 访问会将方法的 FuncDecl 添加到 func_decl_list
        }
    }

    // 创建程序节点
    tree::Program* program = new tree::Program(func_decl_list);
    visit_tree_result = program;
}


void ASTToTreeVisitor::visit(fdmj::MainMethod* node) {
    // 保存当前上下文
    tree::Temp_map* old_temp_map = temp_map;
    Method_var_table* old_method_var_table = method_var_table;

    // 创建新的临时变量映射表和方法变量表
    temp_map = new tree::Temp_map();
    method_var_table = new Method_var_table();

    // 更新当前类名和方法名
    current_class_name = "_^main^_";
    current_method_name = "main";

    // 主方法的参数列表 (无参数)
    std::vector<tree::Temp*>* args = new std::vector<tree::Temp*>();

    // 主方法返回类型为INT
    tree::Type return_type = tree::Type::INT;
    tree::Temp* return_temp = temp_map->newtemp();
    string return_name = "_^return^_" + current_method_name;

    // 添加返回值到方法变量表
    method_var_table->var_temp_map->insert({return_name, return_temp});
    method_var_table->var_type_map->insert({return_name, return_type});

    // 局部变量声明
    if (node->vdl != nullptr) {
        for (auto& var_decl : *(node->vdl)) {
            // 确定变量类型
            tree::Type var_type;
            if (var_decl->type->typeKind == TypeKind::INT) {
                var_type = tree::Type::INT;
            } else {
                var_type = tree::Type::PTR; // 对象和数组都是指针类型
            }
            
            // 创建临时变量并添加到方法变量表
            tree::Temp* var_temp = temp_map->newtemp();
            method_var_table->var_temp_map->insert({var_decl->id->id, var_temp});
            method_var_table->var_type_map->insert({var_decl->id->id, var_type});
            
            // 访问变量声明，处理初始化
            var_decl->accept(*this);
        }
    }

    // 创建主方法的基本块
    std::vector<tree::Block*>* blocks = new std::vector<tree::Block*>();
    tree::Label* entry_label = temp_map->newlabel();
    std::vector<tree::Label*>* exit_labels = new std::vector<tree::Label*>();

    // 处理语句列表
    std::vector<tree::Stm*>* stm_list = new std::vector<tree::Stm*>();
    if (node->sl != nullptr) {
        for (auto& stm : *(node->sl)) {
            stm->accept(*this);
            tree::Stm* tree_stm = dynamic_cast<tree::Stm*>(visit_tree_result);
            if (tree_stm != nullptr) {
                stm_list->push_back(tree_stm);
            } else {
                cerr << "Warning: Statement in main method is not a valid statement, skipping." << endl;
            }
        }
    }

    // 创建入口块
    tree::Block* entry_block = new tree::Block(entry_label, exit_labels, stm_list);
    blocks->push_back(entry_block);

    // 记录临时变量和标签的最大编号
    int last_temp_num = temp_map->next_temp - 1;
    int last_label_num = temp_map->next_label - 1;

    // 创建主函数声明
    tree::FuncDecl* main_func = new tree::FuncDecl(
        current_class_name + "^" + current_method_name,
        args,
        blocks,
        return_type,
        last_temp_num,
        last_label_num
    );

    // 恢复旧的上下文
    delete temp_map;
    delete method_var_table;
    temp_map = old_temp_map;
    method_var_table = old_method_var_table;

    // 返回主函数声明
    visit_tree_result = main_func;
}


void ASTToTreeVisitor::visit(fdmj::ClassDecl* node) {
    // 更新当前类名
    current_class_name = node->id->id;

    // 创建函数声明列表，用于存储该类的所有方法
    std::vector<tree::FuncDecl*>* decl_list = new std::vector<tree::FuncDecl*>();

    // 遍历类成员变量
    // 注意：在IR中，类成员变量不直接生成代码，而是在Class_table中记录偏移

    // 类方法
    if (node->mdl != nullptr) {
        for (auto& method : *(node->mdl)) {
            // 访问类方法声明，这会生成FuncDecl节点
            method->accept(*this);
            tree::FuncDecl* func_decl = dynamic_cast<tree::FuncDecl*>(visit_tree_result);
            
            if (func_decl != nullptr) {
                decl_list->push_back(func_decl);
            } else {
                cerr << "Warning: Method " << method->id->id << " in class " 
                     << node->id->id << " does not generate a valid function declaration." << endl;
            }
        }
    }

    // 记录类的函数声明列表
    func_decl_list->insert(func_decl_list->end(), decl_list->begin(), decl_list->end());
    visit_tree_result = nullptr;
}


void ASTToTreeVisitor::visit(fdmj::Type* node) {
    // Type 节点本身不需要转换为特定的 IR 节点
    // 它的类型信息在处理其他节点（如 VarDecl、MethodDecl、Formal 等）时被使用
    visit_tree_result = nullptr;
}


void ASTToTreeVisitor::visit(fdmj::VarDecl* node) {
    // 检查我们是否在方法内部（有方法变量表）
    if (method_var_table == nullptr) {
        // 如果不在方法内部，可能是类成员变量，这在类声明处理中已处理
        visit_tree_result = nullptr;
        return;
    }

    // 获取变量名、类型和对应的临时变量
    string var_name = node->id->id;
    tree::Type var_type = method_var_table->get_var_type(var_name);
    tree::Temp* var_temp = method_var_table->get_var_temp(var_name);

    //处理初始化
    if (holds_alternative<monostate>(node->init)) {
        // 没有初始化值
        visit_tree_result = nullptr;
    }
    else if (holds_alternative<fdmj::IntExp*>(node->init)) {
        // 整数初始化
        fdmj::IntExp* init_int = get<fdmj::IntExp*>(node->init);
        tree::Const* const_val = new tree::Const(init_int->val);
        tree::TempExp* temp_exp = new tree::TempExp(var_type, var_temp);
        tree::Move* move = new tree::Move(temp_exp, const_val);
        visit_tree_result = move;
    }
    else if (holds_alternative<vector<fdmj::IntExp*>*>(node->init)) {
        // 数组初始化
        vector<fdmj::IntExp*>* init_array = get<vector<fdmj::IntExp*>*>(node->init);

        // 创建一个语句序列存储初始化语句
        std::vector<tree::Stm*>* stm_list = new std::vector<tree::Stm*>();

        // 计算数组大小
        int array_size = (init_array->size() + 1) * compiler_config->get("int_length");
        tree::Const* array_size_const = new tree::Const(array_size);

        // 为数组分配地址
        std::vector<tree::Exp*>* new_array_args = new std::vector<tree::Exp*>();
        new_array_args->push_back(array_size_const);
        tree::ExtCall* malloc_array = new tree::ExtCall(tree::Type::PTR, "malloc", new_array_args);

        // 将新数组赋值给变量
        tree::TempExp* var_temp_exp = new tree::TempExp(var_type, var_temp);
        tree::Move* move_array = new tree::Move(var_temp_exp, malloc_array);
        stm_list->push_back(move_array);

        // 数组的第一个元素是数组大小
        tree::Const* first_elem = new tree::Const(init_array->size());                  // 第一个元素的值
        tree::TempExp* first_elem_temp = new tree::TempExp(var_type, var_temp);         // 数组基地址
        tree::Mem* first_elem_mem = new tree::Mem(tree::Type::INT, first_elem_temp);    // 访存表达式
        tree::Move* first_elem_move = new tree::Move(first_elem_mem, first_elem);       // 赋值语句
        stm_list->push_back(first_elem_move);

        // 然后为每个数组元素赋值
        int element_size = compiler_config->get("int_length");
        for (size_t i = 0; i < init_array->size(); i++) {
            fdmj::IntExp* elem = (*init_array)[i];
            
            // 计算元素地址: array_base + (i + 1) * element_size
            tree::Binop* addr = new tree::Binop(
                tree::Type::PTR, "+", 
                new tree::TempExp(var_type, var_temp),
                new tree::Const((i + 1) * element_size)
            );
            
            // 创建内存访问表达式
            tree::Mem* mem = new tree::Mem(tree::Type::INT, addr);
            
            // 创建赋值语句
            tree::Move* elem_move = new tree::Move(mem, new tree::Const(elem->val));
            stm_list->push_back(elem_move);
        }

        // 创建语句序列
        tree::Seq* seq = new tree::Seq(stm_list);
        visit_tree_result = seq;
    }
}



void ASTToTreeVisitor::visit(fdmj::MethodDecl* node) {
    // 保存当前上下文状态
    tree::Temp_map* old_temp_map = temp_map;
    Method_var_table* old_method_var_table = method_var_table;

    // 更新当前方法名
    current_method_name = node->id->id;

    // 创建新的临时变量映射表和方法变量表
    temp_map = new tree::Temp_map();
    method_var_table = new Method_var_table();


    // 创建参数列表
    std::vector<tree::Temp*>* args = new std::vector<tree::Temp*>();

    // 第一个参数是 this 指针
    tree::Temp* this_temp = temp_map->newtemp();
    args->push_back(this_temp);

    method_var_table->var_temp_map->insert({"this", this_temp});
    method_var_table->var_type_map->insert({"this", tree::Type::PTR});

    // 访问形参列表
    if (node->fl != nullptr) {
        for (auto& formal : *(node->fl)) {
            // 形参处理会更新 method_var_table
            formal->accept(*this); 
            tree::Temp* param_temp = method_var_table->get_var_temp(formal->id->id);
            if (param_temp == nullptr) {
                cerr << "Error: Parameter " << formal->id->id << " not found in method variable table." << endl;
                visit_tree_result = nullptr;
                return;
            }
            args->push_back(param_temp);
        }
    }

    // 确定返回类型，并添加返回值形参
    tree::Type return_type;
    tree::Temp* return_temp = temp_map->newtemp();
    string return_name = "_^return^_" + current_method_name;

    if (node->type->typeKind == TypeKind::INT) {
        return_type = tree::Type::INT;
    } else if (node->type->typeKind == TypeKind::CLASS) {
        return_type = tree::Type::PTR;
    } else if (node->type->typeKind == TypeKind::ARRAY) {
        return_type = tree::Type::PTR;
    } else {
        cerr << "Error: Unsupported return type in method declaration." << endl;
        visit_tree_result = nullptr;
        return;
    }

    method_var_table->var_temp_map->insert({return_name, return_temp});

    // 访问局部变量列表
    if (node->vdl != nullptr) {
        for (auto& var_decl : *(node->vdl)) {
            // 确定变量类型
            tree::Type var_type;
            if (var_decl->type->typeKind == TypeKind::INT) {
                var_type = tree::Type::INT;
            } else {
                var_type = tree::Type::PTR; // 对象和数组都是指针
            }
            
            // 创建临时变量并添加到方法变量表
            tree::Temp* var_temp = temp_map->newtemp();
            method_var_table->var_temp_map->insert({var_decl->id->id, var_temp});
            method_var_table->var_type_map->insert({var_decl->id->id, var_type});

            // 访问 VarDecl 节点
            var_decl->accept(*this);
        }
    }

    std::vector<tree::Block*>* blocks = new std::vector<tree::Block*>();        // 块列表
    tree::Label* entry_label = temp_map->newlabel();                            // 入口标签
    std::vector<tree::Label*>* exit_labels = new std::vector<tree::Label*>();   // 出口标签
    
    // 访问语句列表
    std::vector<tree::Stm*>* stm_list = new std::vector<tree::Stm*>();
    if (node->sl != nullptr) {
        for (auto& stm : *(node->sl)) {
            stm->accept(*this);
            tree::Stm* tree_stm = dynamic_cast<tree::Stm*>(visit_tree_result);
            if (tree_stm != nullptr) {
                stm_list->push_back(tree_stm);
            } else {
                cerr << "Warning: Statement in method body is not a valid statement, skipping." << endl;
            }
        }
    }

    tree::Block* entry_block = new tree::Block(entry_label, exit_labels, stm_list);
    blocks->push_back(entry_block);

    // 记录临时变量和标签的最大编号
    int last_temp_num = temp_map->next_temp - 1;
    int last_label_num = temp_map->next_label - 1;

    // 构造方法的完整名称（类名.方法名）
    string method_name = current_class_name + "^" + node->id->id;

    // 创建函数声明节点
    tree::FuncDecl* func_decl = new tree::FuncDecl(
        method_name,
        args,
        blocks,
        return_type,
        last_temp_num,
        last_label_num
    );

    // 恢复之前的上下文状态
    delete temp_map;
    delete method_var_table;
    temp_map = old_temp_map;
    method_var_table = old_method_var_table;

    // 返回函数声明节点
    visit_tree_result = func_decl;
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
    visit_tree_result = nullptr;
}


void ASTToTreeVisitor::visit(fdmj::Nested* node) {
    // 如果语句列表为空，返回空序列
    if (node->sl == nullptr || node->sl->empty()) {
        visit_tree_result = new tree::Seq();
        return;
    }

    // 创建新的语句列表
    std::vector<tree::Stm*>* stm_list = new std::vector<tree::Stm*>();

    for (auto& stm : *(node->sl)) {
        // 访问子语句
        stm->accept(*this);
        tree::Stm* tree_stm = dynamic_cast<tree::Stm*>(visit_tree_result);
        if (tree_stm != nullptr) {
            stm_list->push_back(tree_stm);
        } else {
            cerr << "Warning: Statement in Nested block is not a valid statement, skipping." << endl;
        }
    }

    // 创建语句序列
    tree::Seq* seq = new tree::Seq(stm_list);
    visit_tree_result = seq;
}


void ASTToTreeVisitor::visit(fdmj::If* node) {
    // 创建标签
    tree::Label* true_label = temp_map->newlabel();    // 真分支标签
    tree::Label* false_label = temp_map->newlabel();   // 假分支标签
    tree::Label* end_label = temp_map->newlabel();     // 结束标签

    // 创建语句列表
    std::vector<tree::Stm*>* stm_list = new std::vector<tree::Stm*>();

    // 访问条件表达式
    node->exp->accept(*this);
    tree::Exp* cond_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (cond_exp == nullptr) {
        cerr << "Error: if condition is not a valid expression." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 1. 条件跳转：条件为真时跳转到true_label，为假时跳转到false_label
    tree::Cjump* cjump = new tree::Cjump("!=", cond_exp, new tree::Const(0), true_label, false_label);
    stm_list->push_back(cjump);

    // 2. 真分支标签
    stm_list->push_back(new tree::LabelStm(true_label));

    // 3. 真分支语句
    if (node->stm1 != nullptr) {
        node->stm1->accept(*this);
        tree::Stm* true_stm = dynamic_cast<tree::Stm*>(visit_tree_result);
        if (true_stm != nullptr) {
            stm_list->push_back(true_stm);
        }
    }

    // 4. 真分支结束后跳转到end_label
    stm_list->push_back(new tree::Jump(end_label));

    // 5. 假分支标签
    stm_list->push_back(new tree::LabelStm(false_label));

    // 6. 假分支语句（如果有）
    if (node->stm2 != nullptr) {
        node->stm2->accept(*this);
        tree::Stm* false_stm = dynamic_cast<tree::Stm*>(visit_tree_result);
        if (false_stm != nullptr) {
            stm_list->push_back(false_stm);
        }
    }

    // 结束标签
    stm_list->push_back(new tree::LabelStm(end_label));

    // 创建语句序列
    tree::Seq* seq = new tree::Seq(stm_list);
    visit_tree_result = seq;
}


void ASTToTreeVisitor::visit(fdmj::While* node) {
    // 创建标签
    tree::Label* loop_start = temp_map->newlabel();  // 循环开始标签
    tree::Label* loop_cond = temp_map->newlabel();   // 条件检查标签
    tree::Label* loop_end = temp_map->newlabel();    // 循环结束标签

    // 保存之前循环的标签（用于处理嵌套循环）
    tree::Label* old_start = current_loop_start_label;
    tree::Label* old_end = current_loop_end_label;

    // 创建语句列表
    std::vector<tree::Stm*>* stm_list = new std::vector<tree::Stm*>();

    // 1. 开始标签
    stm_list->push_back(new tree::LabelStm(loop_start));

    // 2. 无条件跳转到条件判断
    stm_list->push_back(new tree::Jump(loop_cond));

    // 3. 循环体入口标签（循环体开始前先判断条件）
    tree::LabelStm* body_label = new tree::LabelStm(temp_map->newlabel());
    stm_list->push_back(body_label);

    // 4. 循环体（如果有）
    if (node->stm != nullptr) {
        node->stm->accept(*this);
        tree::Stm* body = dynamic_cast<tree::Stm*>(visit_tree_result);
        if (body != nullptr) {
            stm_list->push_back(body);
        }
    }

    // 5. 条件检查标签
    stm_list->push_back(new tree::LabelStm(loop_cond));

    // 6. 条件判断（访问条件表达式）
    node->exp->accept(*this);
    tree::Exp* cond_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (cond_exp == nullptr) {
        cerr << "Error: while condition is not a valid expression." << endl;
        visit_tree_result = nullptr;
        
        // 恢复之前的循环标签
        current_loop_start_label = old_start;
        current_loop_end_label = old_end;
        return;
    }

    // 7. 条件跳转（条件为真时继续循环，否则退出）
    tree::Cjump* cjump = new tree::Cjump("!=", cond_exp, new tree::Const(0), 
                                          body_label->label, loop_end);

    // 8. 循环结束标签
    stm_list->push_back(new tree::LabelStm(loop_end));

    // 创建语句序列
    tree::Seq* seq = new tree::Seq(stm_list);
    visit_tree_result = seq;

    // 恢复之前的循环标签
    current_loop_start_label = old_start;
    current_loop_end_label = old_end;
}


void ASTToTreeVisitor::visit(fdmj::Assign* node) {
    // 首先访问左侧表达式
    node->left->accept(*this);
    tree::Exp* dst = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (dst == nullptr) {
        cerr << "Error: Left-hand side of assignment is not a valid expression." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 检查左侧表达式是否为左值
    AST_Semant* left_semant = semant_map->getSemant(node->left);
    if (left_semant == nullptr || !left_semant->is_lvalue()) {
        cerr << "Error: Left-hand side of assignment is not an lvalue." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 访问右侧表达式
    node->exp->accept(*this);
    tree::Exp* src = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (src == nullptr) {
        cerr << "Error: Right-hand side of assignment is not a valid expression." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 创建赋值节点
    tree::Move* move = new tree::Move(dst, src);
    visit_tree_result = move;
}


void ASTToTreeVisitor::visit(fdmj::CallStm* node) {
    // 访问对象
    node->obj->accept(*this);
    tree::Exp* obj_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (obj_exp == nullptr) {
        cerr << "Error: object expression is not a valid expression." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 访问参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();
    if (node->par != nullptr) {
        for (auto& param : *(node->par)) {
            // 访问参数
            param->accept(*this);
            tree::Exp* arg_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
            
            if (arg_exp == nullptr) {
                cerr << "Error: parameter expression is not a valid expression." << endl;
                delete args;
                visit_tree_result = nullptr;
                return;
            }
            
            args->push_back(arg_exp);
        }
    }

    // 创建调用节点，注意 CallStm 不关心返回值，所以类型设置为 INT
    tree::Call* call = new tree::Call(tree::Type::INT, node->name->id, obj_exp, args);

    // CallStm 是语句，不是表达式，所以需要包装在 ExpStm 中
    tree::ExpStm* exp_stm = new tree::ExpStm(call);
    visit_tree_result = exp_stm;
}


void ASTToTreeVisitor::visit(fdmj::Continue* node) {
    // 检查是否在循环内
    if (current_loop_start_label == nullptr) {
        cerr << "Error: continue statement outside of loop." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 创建跳转指令，跳转到循环开始标签
    tree::Jump* jump = new tree::Jump(current_loop_start_label);
    visit_tree_result = jump;
}


void ASTToTreeVisitor::visit(fdmj::Break* node) {
    // 获取当前循环的结束标签
    if (current_loop_end_label == nullptr) {
        cerr << "Error: break statement outside of loop." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 创建跳转指令，跳转到循环结束标签
    tree::Jump* jump = new tree::Jump(current_loop_end_label);
    visit_tree_result = jump;
}


void ASTToTreeVisitor::visit(fdmj::Return* node) {
    // 访问返回值
    node->exp->accept(*this);
    tree::Exp* return_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (return_exp == nullptr) {
        cerr << "Error: return expression is not a valid expression." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 创建 Return 节点
    tree::Return* return_node = new tree::Return(return_exp);
    visit_tree_result = return_node;
}


void ASTToTreeVisitor::visit(fdmj::PutInt* node) {
    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 访问整数表达式
    node->exp->accept(*this);
    tree::Exp* int_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (int_exp == nullptr) {
        cerr << "Error: integer expression is not a valid expression." << endl;
        delete args;
        visit_tree_result = nullptr;
        return;
    }
    args->push_back(int_exp);

    // 创建外部调用节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "putint", args);

    // putint 是语句，不是表达式
    tree::ExpStm* exp_stm = new tree::ExpStm(ext_call);
    visit_tree_result = exp_stm;
}




void ASTToTreeVisitor::visit(fdmj::PutCh* node) {
    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 访问参数
    node->exp->accept(*this);
    tree::Exp* char_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (char_exp == nullptr) {
        cerr << "Error: character expression is not a valid expression." << endl;
        delete args;
        visit_tree_result = nullptr;
        return;
    }
    args->push_back(char_exp);

    // 创建外部调用节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "putch", args);

    // putch 是语句，不是表达式，所以需要包装在 ExpStm 中
    tree::ExpStm* exp_stm = new tree::ExpStm(ext_call);
    visit_tree_result = exp_stm;
}


void ASTToTreeVisitor::visit(fdmj::PutArray* node) {
    // 创建参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 访问 n 表达式 (打印数量)
    node->n->accept(*this);
    tree::Exp* size_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (size_exp == nullptr) {
        cerr << "Error: size expression is not a valid expression." << endl;
        delete args;
        visit_tree_result = nullptr;
        return;
    }
    args->push_back(size_exp);

    // 访问 arr 表达式 (数组)
    node->arr->accept(*this);
    tree::Exp* array_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (array_exp == nullptr) {
        cerr << "Error: array expression is not a valid expression." << endl;
        delete args;
        visit_tree_result = nullptr;
        return;
    }
    args->push_back(array_exp);

    // 创建外部调用节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "putarray", args);

    // putarray 是语句，不是表达式
    tree::ExpStm* exp_stm = new tree::ExpStm(ext_call);
    visit_tree_result = exp_stm;
}


void ASTToTreeVisitor::visit(fdmj::Starttime* node) {
    // 创建空的参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 创建节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "starttime", args);
    visit_tree_result = ext_call;
}


void ASTToTreeVisitor::visit(fdmj::Stoptime* node) {
    // 创建空的参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    // 创建节点
    tree::ExtCall* ext_call = new tree::ExtCall(tree::Type::INT, "stoptime", args);
    visit_tree_result = ext_call;
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


void ASTToTreeVisitor::visit(fdmj::UnaryOp* node) {
    // 访问操作数
    node->exp->accept(*this);
    tree::Exp* operand = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (operand == nullptr) {
        cerr << "Error: operand is not a valid expression." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 获取语义信息
    AST_Semant* semant = semant_map->getSemant(node);
    if (semant == nullptr) {
        cerr << "Error: semantic information not found for the node." << endl;
        visit_tree_result = nullptr;
        return;
    }

    tree::Type result_type;
    if (semant->get_type() == TypeKind::INT) {
        result_type = tree::Type::INT;
    } else {
        cerr << "Error: unexpected type for unary operation." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 转换为二元操作
    if (node->op->op == "-") {
        // 负号：0 - exp
        tree::Const* zero = new tree::Const(0);
        tree::Binop* binop = new tree::Binop(result_type, "-", zero, operand);
        visit_tree_result = binop;
    } 
    else if (node->op->op == "!") {
        // 逻辑非：1 xor exp
        tree::Const* one = new tree::Const(1);
        tree::Binop* binop = new tree::Binop(result_type, "xor", one, operand);
        visit_tree_result = binop;
    }
    else {
        cerr << "Error: unsupported unary operator: " << node->op->op << endl;
        visit_tree_result = nullptr;
    }

}




void ASTToTreeVisitor::visit(fdmj::ArrayExp* node) {
    // 访问数组，获取数组的基址
    node->arr->accept(*this);
    tree::Exp* arr_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (arr_exp == nullptr) {
        cerr << "Error: array expression is not a valid expression." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 访问索引
    node->index->accept(*this);
    tree::Exp* index_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
    if (index_exp == nullptr) {
        cerr << "Error: index expression is not a valid expression." << endl;
        visit_tree_result = nullptr;
        return;
    }

    // 获取语义信息
    AST_Semant* semant = semant_map->getSemant(node);
    if (semant == nullptr) {
        cerr << "Error: semantic information not found for ArrayExp." << endl;
        visit_tree_result = nullptr;
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
        visit_tree_result = nullptr;
        return;
    }

    // 计算元素地址 array_base + index * element_size
    tree::Binop* addr_exp = new tree::Binop(tree::Type::PTR, "+", arr_exp, 
        new tree::Binop(tree::Type::INT, "*", index_exp, new tree::Const(element_size)));

    // 创建访存表达式
    tree::Mem* mem_exp = new tree::Mem(element_type, addr_exp);
    visit_tree_result = mem_exp;
}

void ASTToTreeVisitor::visit(fdmj::CallExp* node) {
    // 访问对象
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
        cerr << "Error: semantic information not found for CallExp." << endl;
        visit_tree_result = nullptr;
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
        visit_tree_result = nullptr;
        return;
    }

    // 参数列表
    std::vector<tree::Exp*>* args = new std::vector<tree::Exp*>();

    if (node->par != nullptr) {
        for (auto& arg : *node->par) {
            arg->accept(*this);
            tree::Exp* arg_exp = dynamic_cast<tree::Exp*>(visit_tree_result);
            if (arg_exp == nullptr) {
                cerr << "Error: argument expression is not a valid expression." << endl;
                delete args;
                visit_tree_result = nullptr;
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
        visit_tree_result = nullptr;
        return;
    }
    else if (kind == AST_Semant::Kind::MethodName) {
        // 若是方法名
        visit_tree_result = nullptr;
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