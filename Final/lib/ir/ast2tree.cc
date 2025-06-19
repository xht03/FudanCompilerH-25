#define DEBUG
// #undef DEBUG

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
#include "namemaps.hh"

using namespace std;
// using namespace fdmj;
// using namespace tree;

static const int int_length = Compiler_Config::get("int_length");
static const int address_length = Compiler_Config::get("address_length");

// 生成所有类的类表
Class_table* gen_class_table_map(Name_Maps* name_maps)
{
    set<string> global_var_list;
    set<string> global_method_list;
    set<string>* class_list = name_maps->get_class_list();
    for (auto class_name : *class_list) {
        if (class_name == MAIN_class_name)
            continue;
        auto class_var_list = name_maps->get_class_var_list(class_name);
        auto class_method_list = name_maps->get_method_list(class_name);
        global_var_list.insert(class_var_list->begin(), class_var_list->end());
        global_method_list.insert(class_method_list->begin(), class_method_list->end());
    }

    int offset = 0;
    auto var_pos_map = new map<string, int>();
    for (auto var : global_var_list) {
        (*var_pos_map)[var] = offset;
        offset += address_length;
    }

    auto method_pos_map = new map<string, int>();
    for (auto method : global_method_list) {
        (*method_pos_map)[method] = offset;
        offset += address_length;
    }

    return new Class_table(var_pos_map, method_pos_map, offset);
}

// 为当前函数生成方法变量表
Method_var_table* generate_method_var_table(string class_name, string method_name, Name_Maps* nm, Temp_map* tm)
{
    auto var_temp_map = new map<string, tree::Temp*>; // 变量名->变量结点
    auto var_type_map = new map<string, tree::Type>;  // 变量名->类型结点

    // 首先填充方法参数
    vector<string>* method_formal_list = nm->get_method_formal_list(class_name, method_name);
    for (auto f_str : *method_formal_list) {
        fdmj::Formal* f = nm->get_method_formal(class_name, method_name, f_str);
        tree::Temp* f_temp = tm->newtemp();
        string f_id = f->id->id;
        tree::Type f_type = f->type->typeKind == TypeKind::INT ? tree::Type::INT : tree::Type::PTR;
        (*var_temp_map)[f_id] = f_temp;
        (*var_type_map)[f_id] = f_type;
    }

    // 然后填充局部变量
    set<string>* method_var_list = nm->get_method_var_list(class_name, method_name);
    for (auto v_str : *method_var_list) {
        fdmj::VarDecl* vd = nm->get_method_var(class_name, method_name, v_str);
        tree::Temp* v_temp = tm->newtemp();
        string v_id = vd->id->id;
        tree::Type v_type = vd->type->typeKind == TypeKind::INT ? tree::Type::INT : tree::Type::PTR;
        (*var_temp_map)[v_id] = v_temp;
        (*var_type_map)[v_id] = v_type;
    }

    return new Method_var_table(var_temp_map, var_type_map);
}

// 对外函数接口
tree::Program* ast2tree(fdmj::Program* prog, AST_Semant_Map* semant_map)
{
    ASTToTreeVisitor visitor = ASTToTreeVisitor(semant_map);
    prog->accept(visitor);
    return visitor.getTree();
}

// 程序: 主方法 类声明列表
// PROG: MAINMETHOD CLASSDECLLIST
void ASTToTreeVisitor::visit(fdmj::Program* node)
{
    // 开始填充fdl
    vector<tree::FuncDecl*>* fdl = new vector<tree::FuncDecl*>();

    // 主方法
    node->main->accept(*this);
    assert(newNodes.size() == 1);
    fdl->push_back(static_cast<tree::FuncDecl*>(newNodes[0]));
    newNodes.clear();

    // 类声明列表
    for (auto cd : *(node->cdl)) {
        cd->accept(*this); // 类声明
        for (auto& newNode : newNodes)
            if (newNode)
                fdl->push_back(static_cast<tree::FuncDecl*>(newNode));
        newNodes.clear();
    }

    tree_root = new tree::Program(fdl);
}

// 主方法: public int main() { 变量声明列表 语句列表 }
// MAINMETHOD: PUBLIC INT MAIN '(' ')' '{' VARDECLLIST STMLIST '}';
// FunctionDeclaration Blocks Block Statements
void ASTToTreeVisitor::visit(fdmj::MainMethod* node)
{
    class_name = MAIN_class_name;
    method_name = MAIN_method_name;

    // 为当前函数生成方法变量表
    method_var_table = generate_method_var_table(class_name, method_name, ast_info->getNameMaps(), &temp_map);

    // 形参列表
    vector<tree::Temp*>* args = new vector<tree::Temp*>();
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
    tree::Block* bk = new tree::Block(entry_label, new vector<tree::Label*>(), sl);
    vector<tree::Block*>* bkl = new vector<tree::Block*>();
    bkl->push_back(bk);

    int lt = temp_map.next_temp - 1;
    int ll = temp_map.next_label - 1;
    newNodes.push_back(new tree::FuncDecl(class_name + "^" + method_name, args, bkl, tree::Type::INT, lt, ll));
}

// 类声明: 类名 [基类名] { 变量声明列表 方法声明列表 }
// CLASSDECL: PUBLIC CLASS ID '{' VARDECLLIST METHODDECLLIST '}'
//          | PUBLIC CLASS ID EXTENDS ID '{' VARDECLLIST METHODDECLLIST '}'
void ASTToTreeVisitor::visit(fdmj::ClassDecl* node)
{
    // 更新当前类名
    class_name = node->id->id;

    // 依次初始化类方法
    auto fdl = new vector<tree::Tree*>();
    for (auto md : *(node->mdl)) {
        md->accept(*this);
        assert(newNodes.size() == 1);
        fdl->push_back(newNodes[0]);
        newNodes.clear();
    }

    newNodes.insert(newNodes.end(), fdl->begin(), fdl->end());
}

// 方法声明: 返回类型 方法名(形参列表) { 变量声明列表 语句列表 }
// METHODDECL: PUBLIC TYPE ID '(' FORMALLIST ')' '{' VARDECLLIST STMLIST '}'
void ASTToTreeVisitor::visit(fdmj::MethodDecl* node)
{
    // 更新当前方法名
    method_name = node->id->id;

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

// 类型:  整型 | 整型数组 | 类
// TYPE: INT | INT '[' ']' | CLASS ID
void ASTToTreeVisitor::visit(fdmj::Type* node) { }

vector<tree::Stm*>* array_decl_helper(fdmj::VarDecl* node, tree::TempExp* arr_temp)
{
    auto type = node->type;
    auto init = node->init;

    auto sl = new vector<tree::Stm*>();

    int size = 0; // 计算数组大小
    if (type->arity && type->arity->val != 0)
        size = type->arity->val;
    else if (holds_alternative<vector<IntExp*>*>(init))
        size = get<vector<IntExp*>*>(init)->size();

    // temp=malloc((size+1) * int_length)
    auto array_malloc_args = new vector<tree::Exp*> { new tree::Const((size + 1) * int_length) };
    auto array_malloc = new tree::ExtCall(tree::Type::PTR, "malloc", array_malloc_args);
    sl->push_back(new tree::Move(arr_temp, array_malloc));

    // temp.size=size
    auto size_mem = new tree::Mem(tree::Type::INT, arr_temp);
    sl->push_back(new tree::Move(size_mem, new tree::Const(size)));

    if (holds_alternative<vector<IntExp*>*>(init)) {
        auto init_list = *get<vector<IntExp*>*>(init);
        for (int i = 0; i < init_list.size(); i++) {
            auto valExp = init_list[i];
            auto offset = new tree::Const((i + 1) * int_length);
            auto index_mem = new tree::Mem(tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", arr_temp, offset));
            sl->push_back(new tree::Move(index_mem, new tree::Const(valExp->val)));
        }
    }

    return sl;
}

// 变量声明
// VARDECL: CLASS ID ID ';'
//        | INT ID ';'
//        | INT '[' ']' ID ';'
//        | INT '[' NUM ']' ID ';'
//        | INT ID '=' CONST ';'
//        | INT '[' ']' ID '=' '{' CONSTLIST '}' ';'
//        | INT '[' NUM ']' ID '=' '{' CONSTLIST '}' ';'
void ASTToTreeVisitor::visit(fdmj::VarDecl* node)
{
    auto type = node->type;
    auto id = node->id;
    auto init = node->init;

    auto temp = method_var_table->get_var_temp(id->id);

    // 整型
    if (type->typeKind == TypeKind::INT) {
        if (holds_alternative<fdmj::IntExp*>(init)) {
            int val = get<fdmj::IntExp*>(init)->val;
            tree::TempExp* int_temp = new tree::TempExp(tree::Type::INT, temp);
            newNodes.push_back(new tree::Move(int_temp, new tree::Const(val)));
        }
    }

    // 整型数组
    else if (type->typeKind == TypeKind::ARRAY) {
        auto arr_temp = new tree::TempExp(tree::Type::PTR, temp);
        auto sl = array_decl_helper(node, arr_temp);
        newNodes.insert(newNodes.end(), sl->begin(), sl->end());
    }

    // 类
    else if (node->type->typeKind == TypeKind::CLASS) {
        string class_name = node->type->cid->id;
        auto class_var_list = ast_info->name_maps->get_class_var_list(class_name);
        auto class_method_list = ast_info->name_maps->get_method_list(class_name);

        // 为类实例分配空间
        auto class_temp = new tree::TempExp(tree::Type::PTR, temp);
        auto class_size = class_table->offset;
        auto class_malloc_args = new vector<tree::Exp*> { new tree::Const(class_size) };
        auto class_malloc = new tree::ExtCall(tree::Type::PTR, "malloc", class_malloc_args);
        newNodes.push_back(new tree::Move(class_temp, class_malloc));

        // 初始化类成员变量
        auto name_maps = ast_info->name_maps;
        for (auto var_name : *class_var_list) {
            auto offset = class_table->get_var_pos(var_name);
            auto var_decl = name_maps->get_class_var(class_name, var_name);

            // 成员变量是整型
            if (var_decl->type->typeKind == TypeKind::INT) {
                if (holds_alternative<fdmj::IntExp*>(var_decl->init)) {
                    int val = get<fdmj::IntExp*>(var_decl->init)->val;

                    auto var_mem = new tree::Mem(
                        tree::Type::PTR, new tree::Binop(tree::Type::PTR, "+", class_temp, new tree::Const(offset)));

                    auto val_temp = new tree::TempExp(tree::Type::INT, temp_map.newtemp());
                    auto val_move = new tree::Move(val_temp, new tree::Const(val));
                    auto val_eseq
                        = new tree::Eseq(tree::Type::INT, new tree::Seq(new vector<tree::Stm*> { val_move }), val_temp);
                    newNodes.push_back(new tree::Move(var_mem, val_eseq));
                }
            }

            // 成员变量是整型数组
            else if (var_decl->type->typeKind == TypeKind::ARRAY) {
                // 首先构造暂存数组
                auto arr_temp = new tree::TempExp(tree::Type::PTR, temp_map.newtemp());
                auto array_sl = array_decl_helper(var_decl, arr_temp);
                auto array_init = new tree::Eseq(tree::Type::PTR, new tree::Seq(array_sl), arr_temp);
                // 然后把数组地址存入成员变量
                auto var_mem = new tree::Mem(
                    tree::Type::PTR, new tree::Binop(tree::Type::PTR, "+", class_temp, new tree::Const(offset)));
                newNodes.push_back(new tree::Move(var_mem, array_init));
            }
        }

        // 初始化类成员方法
        for (auto method_name : *class_method_list) {
            auto offset = class_table->get_method_pos(method_name);
            string method_real_class = class_name;

            // 找到第一个return_formal不同的
            auto cur_class_name = class_name;
            auto par_class_name = name_maps->get_parent(class_name);
            while (par_class_name != "") {
                auto cur_f_return = name_maps->get_method_return_formal(cur_class_name, method_name);
                auto par_f_return = name_maps->get_method_return_formal(par_class_name, method_name);

                if (par_f_return != cur_f_return)
                    break;

                method_real_class = par_class_name;
                cur_class_name = par_class_name;
                par_class_name = name_maps->get_parent(cur_class_name);
            }

            auto method_mem = new tree::Mem(
                tree::Type::PTR, new tree::Binop(tree::Type::PTR, "+", class_temp, new tree::Const(offset)));
            auto method_nameExp = new Name(temp_map.newstringlabel(method_real_class + "^" + method_name));
            newNodes.push_back(new tree::Move(method_mem, method_nameExp));
        }
    }
}

// 形参: 类型 变量名
// FORMAL: TYPE ID
void ASTToTreeVisitor::visit(fdmj::Formal* node) { }

// 语句
// STM: '{' STMLIST '}'
//      | IF '(' EXP ')' STM ELSE STM
//      | IF '(' EXP ')' STM
//      | WHILE '(' EXP ')' STM
//      | WHILE '(' EXP ')' ';'
//      | EXP '=' EXP ';'
//      | EXP '.' ID '(' EXPLIST ')' ';'
//      | CONTINUE ';'
//      | BREAK ';'
//      | RETURN EXP ';'
//      | PUTINT '(' EXP ')' ';'
//      | PUTCH '(' EXP ')' ';'
//      | PUTARRAY '(' EXP ',' EXP ')' ';'
//      | STARTTIME '(' ')' ';'
//      | STOPTIME '(' ')' ';'

// 语句->语句块
// NESTED: '{' STMLIST '}'
void ASTToTreeVisitor::visit(fdmj::Nested* node)
{
    vector<tree::Stm*>* sl = new vector<tree::Stm*>();
    for (fdmj::Stm* stm : *node->sl) {
        stm->accept(*this);
        for (auto newNode : newNodes)
            if (newNode)
                sl->push_back(static_cast<tree::Stm*>(newNode));
        newNodes.clear();
    }
    newNodes.push_back(new tree::Seq(sl));
}

vector<tree::Stm*>* if_helper(tree::Temp_map& temp_map, Tr_Exp* exp, tree::Stm* stm1, tree::Stm* stm2)
{
    // exp_cx:
    //   True_patch_list: *L1
    //   False_patch_list: *L2
    // If:
    //   exp_cx.stm
    //   L_true [L1]
    //   stm1
    //   Jump L_end
    //   L_false [L2]
    //   stm2
    //   L_end

    auto exp_cx = exp->unCx(&temp_map);
    auto L1 = exp_cx->true_list;
    auto L2 = exp_cx->false_list;
    auto L_true = temp_map.newlabel();
    auto L_false = temp_map.newlabel();
    auto L_end = temp_map.newlabel();

    // 用L_true填补L1, 用L_false填补L2
    L1->patch(L_true);
    L2->patch(L_false);

    vector<tree::Stm*>* sl = new vector<tree::Stm*>();
    sl->push_back(exp_cx->stm);
    sl->push_back(new tree::LabelStm(L_true));

    sl->push_back(stm1);
    sl->push_back(new tree::Jump(L_end));

    sl->push_back(new tree::LabelStm(L_false));
    if (stm2)
        sl->push_back(stm2);

    sl->push_back(new tree::LabelStm(L_end));
    return sl;
}

// 语句->if语句: if (条件表达式) 语句1 [else 语句2]
// STM: IF '(' EXP ')' STM ELSE STM
//    | IF '(' EXP ')' STM
void ASTToTreeVisitor::visit(fdmj::If* node)
{
    node->exp->accept(*this);
    Tr_Exp* exp = newExp;

    node->stm1->accept(*this);
    tree::Stm* stm1 = static_cast<tree::Stm*>(newNodes[0]);
    newNodes.clear();

    tree::Stm* stm2 = nullptr;
    if (node->stm2) {
        node->stm2->accept(*this);
        stm2 = static_cast<tree::Stm*>(newNodes[0]);
        newNodes.clear();
    }

    auto sl = if_helper(temp_map, exp, stm1, stm2);
    newNodes.push_back(new tree::Seq(sl));
}

// 语句->while语句: 辅助函数
static vector<tree::Stm*>* while_helper(
    tree::Temp_map& temp_map, Tr_Exp* exp, vector<tree::Stm*>& body, Label* L_while, Label* L_true, Label* L_end)
{
    // exp_cx:
    //   True_patch_list: *L1
    //   False_patch_list: *L2
    // While:
    //   L_while
    //   exp_cx.stm
    //   L_true [L1]
    //   stm
    //   Jump L_while
    //   L_end [L2]

    auto exp_cx = exp->unCx(&temp_map);
    auto L1 = exp_cx->true_list;
    auto L2 = exp_cx->false_list;

    L1->patch(L_true);
    L2->patch(L_end);

    vector<tree::Stm*>* sl = new vector<tree::Stm*>();
    sl->push_back(new tree::LabelStm(L_while));
    sl->push_back(exp_cx->stm);

    sl->push_back(new tree::LabelStm(L_true));
    for (auto& stm : body)
        sl->push_back(stm);
    sl->push_back(new tree::Jump(L_while));
    sl->push_back(new tree::LabelStm(L_end));
    return sl;
}

// 语句->while语句: while (条件表达式) 语句
// STM: WHILE '(' EXP ')' STM
//    | WHILE '(' EXP ')' ';'
void ASTToTreeVisitor::visit(fdmj::While* node)
{
    node->exp->accept(*this);
    auto exp = newExp;

    auto L_while = temp_map.newlabel();
    auto L_end = temp_map.newlabel();

    // 更新当前所在的while循环
    cur_L_while = L_while;
    cur_L_end = L_end;

    vector<tree::Stm*> body;
    if (node->stm) {
        node->stm->accept(*this);
        for (auto& newNode : newNodes)
            if (newNode)
                body.push_back(static_cast<tree::Stm*>(newNode));
        newNodes.clear();
    }

    auto sl = while_helper(temp_map, exp, body, L_while, temp_map.newlabel(), L_end);
    newNodes.push_back(new tree::Seq(sl));
}

// 语句->赋值语句: 左值表达式 = 右值表达式;
// STM: EXP '=' EXP ';'
void ASTToTreeVisitor::visit(fdmj::Assign* node)
{
    node->left->accept(*this);
    auto left = newExp->unEx(&temp_map)->exp;

    node->exp->accept(*this);
    auto exp = newExp->unEx(&temp_map)->exp;

    newNodes.push_back(new tree::Move(left, exp));
}

tree::Call* ASTToTreeVisitor::call_helper(fdmj::Exp* obj, fdmj::IdExp* name, vector<fdmj::Exp*>* par)
{
    auto class_name = get<string>(ast_info->getSemant(obj)->get_type_par());
    auto method_name = name->id;

    obj->accept(*this);
    tree::Exp* objExp = newExp->unEx(&temp_map)->exp;

    vector<tree::Exp*>* args = new vector<tree::Exp*> { objExp };
    for (auto& arg : *(par)) {
        arg->accept(*this);
        auto arg_exp = newExp->unEx(&temp_map)->exp;
        args->push_back(arg_exp);
    }

    int offset = class_table->get_method_pos(method_name);
    auto method_mem
        = new tree::Mem(tree::Type::PTR, new tree::Binop(tree::Type::PTR, "+", objExp, new tree::Const(offset)));

    auto return_f = ast_info->getNameMaps()->get_method_return_formal(class_name, method_name);
    auto return_type = return_f->type->typeKind == TypeKind::INT ? tree::Type::INT : tree::Type::PTR;
    auto method_call = new tree::Call(return_type, method_name, method_mem, args);
    return method_call;
}

// 语句->类方法调用: 类对象.方法名(形参列表);
// STM: EXP '.' ID '(' EXPLIST ')' ';'
void ASTToTreeVisitor::visit(fdmj::CallStm* node)
{
    auto method_call = call_helper(node->obj, node->name, node->par);
    newNodes.push_back(new tree::ExpStm(method_call));
}

// 语句->continue语句: continue;
// STM: CONTINUE ';'
void ASTToTreeVisitor::visit(fdmj::Continue* node)
{
    assert(cur_L_while);
    newNodes.push_back(new tree::Jump(cur_L_while));
}

// 语句->break语句: break;
// STM: BREAK ';'
void ASTToTreeVisitor::visit(fdmj::Break* node)
{
    assert(cur_L_end);
    newNodes.push_back(new tree::Jump(cur_L_end));
}

// 语句->return语句: return 表达式;
// STM: RETURN EXP ';'
void ASTToTreeVisitor::visit(fdmj::Return* node)
{
    node->exp->accept(*this);
    auto exp = newExp->unEx(&temp_map)->exp;
    newNodes.push_back(new tree::Return(exp));
}

// 语句->打印整数语句: putint(表达式);
// STM: PUTINT '(' EXP ')' ';'
void ASTToTreeVisitor::visit(fdmj::PutInt* node)
{
    node->exp->accept(*this);
    auto exp = newExp->unEx(&temp_map)->exp;

    auto args = new vector<tree::Exp*>();
    args->push_back(exp);

    auto ext_call = new tree::ExtCall(tree::Type::INT, "putint", args);
    newNodes.push_back(new tree::ExpStm(ext_call));
}

// 语句->打印字符语句: putch(表达式);
// STM: PUTCH '(' EXP ')' ';'
void ASTToTreeVisitor::visit(fdmj::PutCh* node)
{
    node->exp->accept(*this);
    auto exp = newExp->unEx(&temp_map)->exp;

    auto args = new vector<tree::Exp*>();
    args->push_back(exp);

    auto ext_call = new tree::ExtCall(tree::Type::INT, "putch", args);
    newNodes.push_back(new tree::ExpStm(ext_call));
}

// 语句->打印数组语句: putarray(长度表达式, 数组表达式);
// STM: PUTARRAY '(' EXP ',' EXP ')' ';'
void ASTToTreeVisitor::visit(fdmj::PutArray* node)
{
    node->n->accept(*this);
    auto n_exp = newExp->unEx(&temp_map)->exp;

    node->arr->accept(*this);
    auto arr_exp = newExp->unEx(&temp_map)->exp;

    auto args = new vector<tree::Exp*> { n_exp, arr_exp };
    auto ext_call = new tree::ExtCall(tree::Type::INT, "putarray", args);
    newNodes.push_back(new tree::ExpStm(ext_call));
}

// 语句->开始计时语句: starttime();
// STM: STARTTIME '(' ')' ';'
void ASTToTreeVisitor::visit(fdmj::Starttime* node)
{
    auto args = new vector<tree::Exp*>();
    auto ext_call = new tree::ExtCall(tree::Type::INT, "starttime", args);
    newNodes.push_back(new tree::ExpStm(ext_call));
}

// 语句->停止计时语句: stoptime();
// STM: STOPTIME '(' ')' ';'
void ASTToTreeVisitor::visit(fdmj::Stoptime* node)
{
    auto args = new vector<tree::Exp*>();
    auto ext_call = new tree::ExtCall(tree::Type::INT, "stoptime", args);
    newNodes.push_back(new tree::ExpStm(ext_call));
}

// 表达式
// EXP -> '(' EXP ')'
//      | '(' '{' STMLIST '}' EXP ')'
//      | ID
//      | NUM
//      | TRUE | FALSE
//      | EXP '[' EXP ']'
//      | OP
//      | EXP [+-*/ COMP && ||] EXP
//      | [-!] EXP
//      | THIS
//      | EXP '.' ID '(' EXPLIST ')'
//      | EXP '.' ID
//      | GETINT '(' ')'
//      | GETCH '(' ')'
//      | GETARRAY '(' EXP ')'
//      | LENGTH '(' EXP ')'

// 表达式->逃逸表达式: ({ 语句列表 } 表达式)
// EXP: '(' '{' STMLIST '}' EXP ')'
void ASTToTreeVisitor::visit(fdmj::Esc* node)
{
    vector<tree::Stm*>* sl = new vector<tree::Stm*>();
    for (fdmj::Stm* stm : *node->sl) {
        stm->accept(*this);
        for (auto newNode : newNodes)
            if (newNode)
                sl->push_back(static_cast<tree::Stm*>(newNode));
        newNodes.clear();
    }

    node->exp->accept(*this);
    auto exp = newExp->unEx(&temp_map)->exp;
    newExp = new Tr_ex(new tree::Eseq(tree::Type::INT, new tree::Seq(sl), exp));
}

// 表达式->标识符: id
// EXP: ID
void ASTToTreeVisitor::visit(fdmj::IdExp* node)
{
    tree::Type type = method_var_table->get_var_type(node->id);
    tree::Temp* temp = method_var_table->get_var_temp(node->id);
    newExp = new Tr_ex(new tree::TempExp(type, temp));
}

// 表达式->整数: num
// EXP: NUM
void ASTToTreeVisitor::visit(fdmj::IntExp* node)
{
    auto val = node->val;
    newExp = new Tr_ex(new tree::Const(val));
}

// 表达式->布尔常量: true | false
// EXP: TRUE | FALSE
void ASTToTreeVisitor::visit(fdmj::BoolExp* node)
{
    auto val = node->val;
    newExp = new Tr_ex(new tree::Const(val));
}

// 提前一下, 编译器眼瞎
Tr_cx* comp_helper(Temp_map& temp_map, tree::Exp* left, tree::Exp* right, string op);

// 表达式->数组访问: 数组表达式[下标表达式]
// EXP: EXP '[' EXP ']'
void ASTToTreeVisitor::visit(fdmj::ArrayExp* node)
{
    bool arr_pre = false, index_pre = false;

    node->arr->accept(*this);
    tree::Exp* arr_exp = newExp->unEx(&temp_map)->exp;

    tree::Exp* arr_temp = nullptr;
    if (arr_exp->getTreeKind() == Kind::TEMPEXP)
        arr_temp = arr_exp;
    else {
        arr_temp = new tree::TempExp(tree::Type::PTR, temp_map.newtemp());
        arr_pre = true; // 如果数组不是值, 则需要先对数组求值
    }

    node->index->accept(*this);
    tree::Exp* index_exp = newExp->unEx(&temp_map)->exp;

    tree::Exp* index_temp = nullptr;
    if (index_exp->getTreeKind() == Kind::TEMPEXP || index_exp->getTreeKind() == Kind::CONST)
        index_temp = index_exp;
    else {
        index_temp = new tree::TempExp(tree::Type::INT, temp_map.newtemp());
        index_pre = true; // 如果索引不是值, 则需要先对索引求值
    }

    // 取出下标 (越界检查)
    auto i_sl = new vector<tree::Stm*>();

    // 取出size
    tree::TempExp* size_temp = new tree::TempExp(tree::Type::INT, temp_map.newtemp());
    tree::Move* size_move = new tree::Move(size_temp, new tree::Mem(tree::Type::INT, arr_temp));
    i_sl->push_back(size_move);

    // index >= size
    Tr_cx* zero_geq_size = comp_helper(temp_map, index_temp, size_temp, ">=");

    // exit(-1)
    auto args = new vector<tree::Exp*> { new tree::Const(-1) };
    auto exit_stm = new tree::ExpStm(new tree::ExtCall(tree::Type::INT, "exit", args));

    // if index>=size ? exit(-1) (判断长度为正数)
    auto L1 = zero_geq_size->true_list;
    auto L2 = zero_geq_size->false_list;
    auto L_true = temp_map.newlabel();
    auto L_false = temp_map.newlabel();
    L1->patch(L_true);
    L2->patch(L_false);
    i_sl->push_back(zero_geq_size->stm);
    i_sl->push_back(new tree::LabelStm(L_true));
    i_sl->push_back(exit_stm);
    i_sl->push_back(new tree::LabelStm(L_false));
    auto i_exp = new tree::Eseq(tree::Type::INT, new tree::Seq(i_sl), index_temp);

    // (i+1)
    auto i_plus_one = new tree::Binop(tree::Type::INT, "+", i_exp, new tree::Const(1));

    // (i+1)*int_length
    auto i_offset = new tree::Binop(tree::Type::INT, "*", i_plus_one, new tree::Const(int_length));

    // *(arr+(i+1)*4)
    auto arr_offset = new tree::Binop(tree::Type::PTR, "+", arr_temp, i_offset);
    auto arr_mem = new tree::Mem(tree::Type::INT, arr_offset);

    // 如果数组和索引都是值, 则直接返回
    if (!arr_pre && !index_pre) {
        newExp = new Tr_ex(arr_mem);
    }
    // 否则需要先求值
    else {
        auto pre_sl = new vector<tree::Stm*>();
        if (arr_pre)
            pre_sl->push_back(new tree::Move(arr_temp, arr_exp));
        if (index_pre)
            pre_sl->push_back(new tree::Move(index_temp, index_exp));
        newExp = new Tr_ex(new tree::Eseq(tree::Type::INT, new tree::Seq(pre_sl), arr_mem));
    }
}

// 表达式->操作符: op
// EXP: [+-*/ COMP && ||] [-!]
void ASTToTreeVisitor::visit(fdmj::OpExp* node) { }

Tr_cx* comp_helper(Temp_map& temp_map, tree::Exp* left, tree::Exp* right, string op)
{
    // 构造CJump
    Label* t = temp_map.newlabel();
    Label* f = temp_map.newlabel();
    auto cjump = new tree::Cjump(op, left, right, t, f);

    // 添加修补列表
    auto true_list = new Patch_list();
    auto false_list = new Patch_list();
    true_list->add_patch(t);
    false_list->add_patch(f);
    return new Tr_cx(true_list, false_list, cjump);
}

// 表达式->二元操作: 左表达式 OP 右表达式
// EXP: EXP OP EXP
void ASTToTreeVisitor::visit(fdmj::BinaryOp* node)
{
    string op = node->op->op;

    node->left->accept(*this);
    auto left_tr = newExp;

    node->right->accept(*this);
    auto right_tr = newExp;

    vector<string> algo_op = { "+", "-", "*", "/" };
    vector<string> comp_op = { "==", "!=", "<", ">", "<=", ">=" };

    // 算数运算
    if (find(algo_op.begin(), algo_op.end(), op) != algo_op.end()) {
        auto left = left_tr->unEx(&temp_map)->exp;
        auto right = right_tr->unEx(&temp_map)->exp;

        // 整型
        if (left->type == tree::Type::INT && right->type == tree::Type::INT)
            newExp = new Tr_ex(new tree::Binop(tree::Type::INT, op, left, right));

        // 整型数组
        else if (left->type == tree::Type::PTR && right->type == tree::Type::PTR) {
            vector<tree::Stm*>* sl = new vector<tree::Stm*>();

            // 获取a.size
            auto a_size_temp = new tree::TempExp(tree::Type::INT, temp_map.newtemp());
            auto a_size_mem = new tree::Mem(tree::Type::INT, left);
            sl->push_back(new tree::Move(a_size_temp, a_size_mem));

            // 获取b.size
            auto b_size_temp = new tree::TempExp(tree::Type::INT, temp_map.newtemp());
            auto b_size_mem = new tree::Mem(tree::Type::INT, right);
            sl->push_back(new tree::Move(b_size_temp, b_size_mem));

            // a.size!=b.size
            Tr_cx* size_neq = comp_helper(temp_map, a_size_temp, b_size_temp, "!=");

            // exit(-1)
            auto args = new vector<tree::Exp*> { new tree::Const(-1) };
            auto exit_stm = new tree::ExpStm(new tree::ExtCall(tree::Type::INT, "exit", args));

            // if a.size!=b.size ? exit(-1)
            auto L1 = size_neq->true_list;
            auto L2 = size_neq->false_list;
            auto L_true = temp_map.newlabel();
            auto L_false = temp_map.newlabel();
            L1->patch(L_true);
            L2->patch(L_false);
            sl->push_back(size_neq->stm);
            sl->push_back(new tree::LabelStm(L_true));
            sl->push_back(exit_stm);
            sl->push_back(new tree::LabelStm(L_false));

            // 创建暂存数组 int[a.size] t
            auto int_length = Compiler_Config::get("int_length");
            auto t_temp = new tree::TempExp(tree::Type::PTR, temp_map.newtemp());
            auto t_size_binop = new tree::Binop(tree::Type::INT, "*",
                new tree::Binop(tree::Type::INT, "+", a_size_temp, new tree::Const(1)), new tree::Const(int_length));
            auto t_malloc = new tree::ExtCall(tree::Type::PTR, "malloc", new vector<tree::Exp*> { t_size_binop });
            sl->push_back(new tree::Move(t_temp, t_malloc));

            // 设置暂存数组大小 t.size=a.size
            auto t_size_mem = new tree::Mem(tree::Type::INT, t_temp);
            sl->push_back(new tree::Move(t_size_mem, a_size_temp));

            // i=int_length
            auto i_temp = new tree::TempExp(tree::Type::INT, temp_map.newtemp());
            sl->push_back(new tree::Move(i_temp, new tree::Const(int_length)));

            // end=(a.size+1)*4
            auto end_temp = new tree::TempExp(tree::Type::INT, temp_map.newtemp());
            auto end_binop = new tree::Binop(tree::Type::INT, "*",
                new tree::Binop(tree::Type::INT, "+", a_size_temp, new tree::Const(1)), new tree::Const(int_length));
            sl->push_back(new tree::Move(end_temp, end_binop));

            // while(i<end)
            //   *(t+i)=*(a+i)+*(b+i)
            //  i=i+int_length
            auto i_lt_end = comp_helper(temp_map, i_temp, end_temp, "<");

            vector<tree::Stm*> body;

            // *(t+i)=*(a+i)+*(b+i)
            auto t_i_mem = new tree::Mem(tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", t_temp, i_temp));
            auto a_i_mem = new tree::Mem(tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", left, i_temp));
            auto b_i_mem = new tree::Mem(tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", right, i_temp));
            auto t_i_binop = new tree::Binop(tree::Type::INT, op, a_i_mem, b_i_mem);
            body.push_back(new tree::Move(t_i_mem, t_i_binop));

            // i=i+int_length
            auto i_plus = new tree::Binop(tree::Type::INT, "+", i_temp, new tree::Const(int_length));
            body.push_back(new tree::Move(i_temp, i_plus));

            // 生成while循环 合并到sl
            auto sl_while
                = while_helper(temp_map, i_lt_end, body, temp_map.newlabel(), temp_map.newlabel(), temp_map.newlabel());
            for (auto& stm : *sl_while)
                sl->push_back(stm);
            newExp = new Tr_ex(new tree::Eseq(tree::Type::INT, new tree::Seq(sl), t_temp));
        }

        else {
            cerr << "BinaryOp: " << op << "未知运算方式" << endl;
            exit(-1);
        }

    }

    // 比较运算
    else if (find(comp_op.begin(), comp_op.end(), op) != comp_op.end()) {
        auto left = left_tr->unEx(&temp_map)->exp;
        auto right = right_tr->unEx(&temp_map)->exp;
        newExp = comp_helper(temp_map, left, right, op);
    }

    // Tr_cx1:
    //   True_patch_list: *L1
    //   False_patch_list: *L2
    // Tr_cx2:
    //   True_patch_list: *L3
    //   False_patch_list: *L4
    // Tr_cx3:
    //   True_patch_list: *L1, *L3
    //   False_patch_list: *L4
    //   Tr_cx1.stm [L5 patches L2]
    //   L5:
    //   Tr_cx2.stm

    // 逻辑或运算
    else if (op == "||") {
        auto left_cx = left_tr->unCx(&temp_map);
        auto right_cx = right_tr->unCx(&temp_map);
        auto L1 = left_cx->true_list;
        auto L2 = left_cx->false_list;
        auto L3 = right_cx->true_list;
        auto L4 = right_cx->false_list;

        // 用L5填补L2
        auto L5 = temp_map.newlabel();
        L2->patch(L5);

        // 合并L1和L3作为新的正确分支
        L1->add(L3);

        vector<tree::Stm*>* sl = new vector<tree::Stm*>();
        sl->push_back(left_cx->stm);
        sl->push_back(new tree::LabelStm(L5));
        sl->push_back(right_cx->stm);
        newExp = new Tr_cx(L1, L4, new tree::Seq(sl));
    }

    // Tr_cx1:
    //   True_patch_list: *L1
    //   False_patch_list: *L2
    // Tr_cx2:
    //   True_patch_list: *L3
    //   False_patch_list: *L4
    // Tr_cx3:
    //   True_patch_list: *L3
    //   False_patch_list: *L2, L4
    //   Tr_cx1.stm [L5 patches L1]
    //   L5:
    //   Tr_cx2.stm

    // 逻辑与运算
    else if (op == "&&") {
        auto left_cx = left_tr->unCx(&temp_map);
        auto right_cx = right_tr->unCx(&temp_map);
        auto L1 = left_cx->true_list;
        auto L2 = left_cx->false_list;
        auto L3 = right_cx->true_list;
        auto L4 = right_cx->false_list;

        // 用L5填补L1
        auto L5 = temp_map.newlabel();
        L1->patch(L5);

        // 合并L2和L4作为新的错误分支
        L2->add(L4);

        vector<tree::Stm*>* sl = new vector<tree::Stm*>();
        sl->push_back(left_cx->stm);
        sl->push_back(new tree::LabelStm(L5));
        sl->push_back(right_cx->stm);
        newExp = new Tr_cx(L3, L2, new tree::Seq(sl));
    }
}

// 表达式->一元操作: OP 表达式
// EXP: '!' EXP | '-' EXP
void ASTToTreeVisitor::visit(fdmj::UnaryOp* node)
{
    string op = node->op->op;

    node->exp->accept(*this);
    auto exp_tr = newExp;

    if (op == "-") {
        tree::Exp* exp = exp_tr->unEx(&temp_map)->exp;

        // 整型
        if (exp->type == tree::Type::INT)
            newExp = new Tr_ex(new tree::Binop(tree::Type::INT, "-", new tree::Const(0), exp));

        // 整型数组
        else if (exp->type == tree::Type::PTR) {
            vector<tree::Stm*>* sl = new vector<tree::Stm*>();

            // 获取数组大小size
            auto a_size_temp = new tree::TempExp(tree::Type::INT, temp_map.newtemp());
            auto a_size_mem = new tree::Mem(tree::Type::INT, exp);
            sl->push_back(new tree::Move(a_size_temp, a_size_mem));

            // 初始化相同大小的暂存数组
            auto int_length = Compiler_Config::get("int_length");
            auto t_temp = new tree::TempExp(tree::Type::PTR, temp_map.newtemp());
            auto t_size_binop = new tree::Binop(tree::Type::INT, "*",
                new tree::Binop(tree::Type::INT, "+", a_size_temp, new tree::Const(1)), new tree::Const(int_length));
            auto t_malloc = new tree::ExtCall(tree::Type::PTR, "malloc", new vector<tree::Exp*> { t_size_binop });
            sl->push_back(new tree::Move(t_temp, t_malloc));

            // 设置暂存数组大小为size
            auto t_size_mem = new tree::Mem(tree::Type::INT, t_temp);
            sl->push_back(new tree::Move(t_size_mem, a_size_temp));

            // 初始化下标索引i=int_length
            auto i_temp = new tree::TempExp(tree::Type::INT, temp_map.newtemp());
            auto i_const = new tree::Const(int_length);
            sl->push_back(new tree::Move(i_temp, i_const));

            // 初始化边界end=(size+1)*4
            auto end_temp = new tree::TempExp(tree::Type::INT, temp_map.newtemp());
            auto end_binop = new tree::Binop(tree::Type::INT, "*",
                new tree::Binop(tree::Type::INT, "+", a_size_temp, new tree::Const(1)), new tree::Const(int_length));
            sl->push_back(new tree::Move(end_temp, end_binop));

            // while(i<end)
            //   *(t+i)=0-*(a+i)
            //   i=i+int_length
            // auto sl = while_helper(tree::Temp_map& temp_map, Tr_Exp* exp, vector<tree::Stm*> body)
            auto i_lt_end = comp_helper(temp_map, i_temp, end_temp, "<");

            vector<tree::Stm*> body;

            // *(t+i)=0-*(a+i)
            auto t_i_mem = new tree::Mem(tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", t_temp, i_temp));
            auto a_i_mem = new tree::Mem(tree::Type::INT, new tree::Binop(tree::Type::PTR, "+", exp, i_temp));
            auto minus_binop = new tree::Binop(tree::Type::INT, "-", new tree::Const(0), a_i_mem);
            body.push_back(new tree::Move(t_i_mem, minus_binop));

            // i=i+int_length
            auto i_plus = new tree::Binop(tree::Type::INT, "+", i_temp, new tree::Const(int_length));
            body.push_back(new tree::Move(i_temp, i_plus));

            // 生成while循环 合并到sl
            auto sl_while
                = while_helper(temp_map, i_lt_end, body, temp_map.newlabel(), temp_map.newlabel(), temp_map.newlabel());
            for (auto& stm : *sl_while)
                sl->push_back(stm);
            newExp = new Tr_ex(new tree::Eseq(tree::Type::PTR, new tree::Seq(sl), t_temp));
        }

    } else if (op == "!") {
        auto exp = exp_tr->unCx(&temp_map);
        auto L1 = exp->true_list;
        auto L2 = exp->false_list;

        // 交换L1和L2
        newExp = new Tr_cx(L2, L1, exp->stm);
    }
}

// 表达式->this指针: this
// EXP: THIS
void ASTToTreeVisitor::visit(fdmj::This* node) { newExp = new Tr_ex(this_temp); }

// 表达式->类变量访问: 类对象.变量名
// EXP: EXP '.' ID
void ASTToTreeVisitor::visit(fdmj::ClassVar* node)
{
    node->obj->accept(*this);
    auto obj = newExp->unEx(&temp_map)->exp;

    auto obj_class_name = get<string>(ast_info->getSemant(node->obj)->get_type_par());
    auto var_name = node->id->id;
    auto var_decl = ast_info->name_maps->get_class_var(obj_class_name, var_name);
    auto var_type = var_decl->type->typeKind == TypeKind::INT ? tree::Type::INT : tree::Type::PTR;
    auto offset = class_table->get_var_pos(var_name);
    auto var_mem = new tree::Mem(var_type, new tree::Binop(tree::Type::PTR, "+", obj, new tree::Const(offset)));
    newExp = new Tr_ex(var_mem);
}

// 表达式->类方法调用: 类对象.方法名(形参列表)
// EXP: EXP '.' ID '(' EXPLIST ')'
void ASTToTreeVisitor::visit(fdmj::CallExp* node)
{
    auto method_call = call_helper(node->obj, node->name, node->par);
    newExp = new Tr_ex(method_call);
}

// 表达式->读取整数: getint()
// EXP: GETINT '(' ')'
void ASTToTreeVisitor::visit(fdmj::GetInt* node)
{
    auto args = new vector<tree::Exp*>();
    auto ext_call = new tree::ExtCall(tree::Type::INT, "getint", args);
    newExp = new Tr_ex(ext_call);
}

// 表达式->读取字符: getch()
// EXP: GETCH '(' ')'
void ASTToTreeVisitor::visit(fdmj::GetCh* node)
{
    auto args = new vector<tree::Exp*>();
    auto ext_call = new tree::ExtCall(tree::Type::INT, "getch", args);
    newExp = new Tr_ex(ext_call);
}

// 表达式->读取数组: getarray(数组变量)
// EXP: GETARRAY '(' EXP ')'
void ASTToTreeVisitor::visit(fdmj::GetArray* node)
{
    node->exp->accept(*this);
    auto args = new vector<tree::Exp*> { newExp->unEx(&temp_map)->exp };
    auto ext_call = new tree::ExtCall(tree::Type::INT, "getarray", args);
    newExp = new Tr_ex(ext_call);
}

// 表达式->获取数组长度: length(数组表达式)
// EXP: LENGTH '(' EXP ')'
void ASTToTreeVisitor::visit(fdmj::Length* node)
{
    node->exp->accept(*this);
    auto arr_temp = newExp->unEx(&temp_map)->exp;
    tree::TempExp* size_temp = new tree::TempExp(tree::Type::INT, temp_map.newtemp());
    tree::Move* size_move = new tree::Move(size_temp, new tree::Mem(tree::Type::INT, arr_temp));
    newExp = new Tr_ex(new tree::Eseq(tree::Type::INT, size_move, size_temp));
}