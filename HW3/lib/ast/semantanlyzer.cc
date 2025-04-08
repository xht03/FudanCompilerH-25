#define DEBUG
#undef DEBUG

#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include "namemaps.hh"
#include "semant.hh"

using namespace std;
using namespace fdmj;

// 对外接口函数
AST_Semant_Map* semant_analyze(Program* node)
{
    if (node == nullptr) {
        cerr << "semant_analyze: Program is null" << endl;
        exit(1);
    }

    // 构造名称映射
    Name_Maps* name_maps = makeNameMaps(node);
    AST_Semant_Visitor semant_visitor(name_maps);

    // 执行语义分析
    semant_visitor.visit(node);
    return semant_visitor.getSemantMap();
}

// 程序: 主方法 类声明列表
// PROG: MAINMETHOD CLASSDECLLIST
void AST_Semant_Visitor::visit(Program* node)
{
    // 调用accept(visit)
    node->main->accept(*this);
    for (auto cl : *(node->cdl)) {
        cl->accept(*this);
    }
}

// 主方法: public int main() { 变量声明列表 语句列表 }
// MAINMETHOD: PUBLIC INT MAIN '(' ')' '{' VARDECLLIST STMLIST '}';
void AST_Semant_Visitor::visit(MainMethod* node)
{
    // 更新当前类名和方法名
    current_class_name = "";
    current_method_name = "main";

    // 调用accept(visit)
    for (auto vd : *(node->vdl)) {
        vd->accept(*this);
    }
    for (auto s : *(node->sl)) {
        s->accept(*this);
    }
}

// 类声明: 类名 [基类名] { 变量声明列表 方法声明列表 }
// CLASSDECL: PUBLIC CLASS ID '{' VARDECLLIST METHODDECLLIST '}'
//          | PUBLIC CLASS ID EXTENDS ID '{' VARDECLLIST METHODDECLLIST '}'
void AST_Semant_Visitor::visit(ClassDecl* node)
{
    // 更新当前类名
    current_class_name = node->id->id;

    // 调用accept(visit)
    for (auto vd : *(node->vdl)) {
        vd->accept(*this);
    }
    for (auto md : *(node->mdl)) {
        md->accept(*this);
    }
}

// 类型:  整型 | 整型数组 | 类
// TYPE: INT | INT '[' ']' | CLASS ID
void AST_Semant_Visitor::visit(Type* node)
{
    // CLASS ID 检查类型是否存在
    if (node->typeKind == TypeKind::CLASS) {
        if (!name_maps->is_class(node->cid->id)) {
            cerr << node->cid->getPos()->print() << endl;
            cerr << "- Type does not exist: " << node->cid->id << endl;
        }
    }
}

// 变量声明
// VARDECL: CLASS ID ID ';'
//        | INT ID ';'
//        | INT '[' ']' ID ';'
//        | INT '[' NUM ']' ID ';'
//        | INT ID '=' CONST ';'
//        | INT '[' ']' ID '=' '{' CONSTLIST '}' ';'
//        | INT '[' NUM ']' ID '=' '{' CONSTLIST '}' ';'
void AST_Semant_Visitor::visit(VarDecl* node) { node->type->accept(*this); }

// 方法声明: 返回类型 方法名(形参列表) { 变量声明列表 语句列表 }
// METHODDECL: PUBLIC TYPE ID '(' FORMALLIST ')' '{' VARDECLLIST STMLIST '}'
void AST_Semant_Visitor::visit(MethodDecl* node)
{
    // 更新当前方法名
    current_method_name = node->id->id;

    // 调用accept(visit)
    node->type->accept(*this);
    for (auto vd : *(node->vdl)) {
        vd->accept(*this);
    }
    for (auto s : *(node->sl)) {
        s->accept(*this);
    }
}

// 形参: 类型 变量名
// FORMAL: TYPE ID
void AST_Semant_Visitor::visit(Formal* node) { node->type->accept(*this); }

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
void AST_Semant_Visitor::visit(Nested* node)
{
    for (auto s : *(node->sl)) {
        s->accept(*this);
    }
}

// 语句->if语句: if (条件表达式) 语句1 [else 语句2]
// STM: IF '(' EXP ')' STM ELSE STM
//    | IF '(' EXP ')' STM
void AST_Semant_Visitor::visit(If* node)
{
    node->exp->accept(*this);
    node->stm1->accept(*this);
    if (node->stm2 != nullptr) {
        node->stm2->accept(*this);
    }

    // 检查Exp为int
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr || exp_semant->get_kind() != AST_Semant::Kind::Value || exp_semant->get_type() != TypeKind::INT) {
        cerr << node->exp->getPos()->print() << endl;
        cerr << "- If condition expression is not of type int" << endl;
        exit(1);
    }
}

// 语句->while语句: while (条件表达式) 语句
// STM: WHILE '(' EXP ')' STM
//    | WHILE '(' EXP ')' ';'
void AST_Semant_Visitor::visit(While* node)
{
    node->exp->accept(*this);
    if (node->stm != nullptr) {
        is_in_while = true;
        node->stm->accept(*this);
        is_in_while = false;
    }

    // 检查Exp为int
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr) {
        cerr << node->exp->getPos()->print() << endl;
        cerr << "- While condition expression has no sub-semantics" << endl;
        exit(1);
    }
    if (exp_semant->get_kind() != AST_Semant::Kind::Value || exp_semant->get_type() != TypeKind::INT) {
        cerr << node->exp->getPos()->print() << endl;
        cerr << "- While condition expression is not of type int" << endl;
        exit(1);
    }
}

// 语句->赋值语句: 左值表达式 = 右值表达式;
// STM: EXP '=' EXP ';'
void AST_Semant_Visitor::visit(Assign* node)
{
    node->left->accept(*this);
    node->exp->accept(*this);

    AST_Semant* left_semant = semant_map->getSemant(node->left);
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (!is_assignable(left_semant, exp_semant)) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Assignment statement left and right types do not match" << endl;
        exit(1);
    }
}

// 语句->类方法调用: 类对象.方法名(形参列表);
// STM: EXP '.' ID '(' EXPLIST ')' ';'
void AST_Semant_Visitor::visit(CallStm* node)
{
    node->obj->accept(*this);

    // 检查Exp为class
    AST_Semant* obj_semant = semant_map->getSemant(node->obj);
    if (!is_class(obj_semant)) {
        cerr << node->obj->getPos()->print() << endl;
        cerr << "- Class method call object is not of class type" << endl;
        exit(1);
    }

    // 处理类方法ID
    is_fetch_class_method = true;
    fetch_class_name = get<string>(obj_semant->get_type_par());
    node->name->accept(*this);
    is_fetch_class_method = false;

    for (auto e : *(node->par)) {
        e->accept(*this);
    }

    // 检查Exp有id方法
    string method_name = node->name->id;
    string class_name = get<string>(obj_semant->get_type_par());
    if (!name_maps->is_method(class_name, method_name)) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Class method does not exist" << endl;
        exit(1);
    }

    // 检查ExpList匹配方法参数
    vector<Formal*>* formal_list = name_maps->get_method_formal_list(class_name, method_name);
    vector<Exp*>* par_list = node->par;
    if (formal_list->size() != par_list->size()) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Class method call parameter count does not match" << endl;
        exit(1);
    }

    for (int i = 0; i < formal_list->size(); i++) {
        Formal* f = (*formal_list)[i];
        Exp* e = (*par_list)[i];

        Type* formal_type = f->type;
        AST_Semant* exp_semant = semant_map->getSemant(e);
        if (!is_assignable(formal_type, exp_semant)) {
            cerr << node->getPos()->print() << endl;
            cerr << "- Class method call parameter types do not match" << endl;
            exit(1);
        }
    }
}

// 语句->continue语句: continue;
// STM: CONTINUE ';'
void AST_Semant_Visitor::visit(Continue* node)
{
    // 检查在while里
    if (!is_in_while) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Continue statement is not inside a while loop" << endl;
        exit(1);
    }
}

// 语句->break语句: break;
// STM: BREAK ';'
void AST_Semant_Visitor::visit(Break* node)
{
    // 检查在while里
    if (!is_in_while) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Break statement is not inside a while loop" << endl;
        exit(1);
    }
}

// 语句->return语句: return 表达式;
// STM: RETURN EXP ';'
void AST_Semant_Visitor::visit(Return* node)
{
    node->exp->accept(*this);

    // 检查Exp为函数返回值类型
    Type* method_return_type = name_maps->get_method_type(current_class_name, current_method_name);
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (!is_assignable(method_return_type, exp_semant)) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Return statement return type does not match" << endl;
        exit(1);
    }

    // 返回对应的返回值类型
    semant_map->setSemant(node, build_semant(method_return_type, false));
}

// 语句->打印整数语句: putint(表达式);
// STM: PUTINT '(' EXP ')' ';'
void AST_Semant_Visitor::visit(PutInt* node)
{
    node->exp->accept(*this);

    // 检查Exp为int
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr) {
        cerr << node->getPos()->print() << endl;
        cerr << "- PutInt statement has no sub-semantics" << endl;
        exit(1);
    }
    if (exp_semant->get_kind() != AST_Semant::Kind::Value || exp_semant->get_type() != TypeKind::INT) {
        cerr << node->getPos()->print() << endl;
        cerr << "- PutInt statement expression is not of type int" << endl;
        exit(1);
    }
}

// 语句->打印字符语句: putch(表达式);
// STM: PUTCH '(' EXP ')' ';'
void AST_Semant_Visitor::visit(PutCh* node)
{
    node->exp->accept(*this);

    // 检查Exp为int
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr) {
        cerr << node->getPos()->print() << endl;
        cerr << "- PutCh statement has no sub-semantics" << endl;
        exit(1);
    }
    if (exp_semant->get_kind() != AST_Semant::Kind::Value || exp_semant->get_type() != TypeKind::INT) {
        cerr << node->getPos()->print() << endl;
        cerr << "- PutCh statement expression is not of type int" << endl;
        exit(1);
    }
}

// 语句->打印数组语句: putarray(长度表达式, 数组表达式);
// STM: PUTARRAY '(' EXP ',' EXP ')' ';'
void AST_Semant_Visitor::visit(PutArray* node)
{
    node->n->accept(*this);
    node->arr->accept(*this);

    // 检查第一个Exp为int，第二个Exp为array
    AST_Semant* n_semant = semant_map->getSemant(node->n);
    AST_Semant* arr_semant = semant_map->getSemant(node->arr);
    if (n_semant == nullptr || arr_semant == nullptr) {
        cerr << node->getPos()->print() << endl;
        cerr << "- PutArray statement has no sub-semantics" << endl;
        exit(1);
    }
    if (n_semant->get_kind() != AST_Semant::Kind::Value || n_semant->get_type() != TypeKind::INT) {
        cerr << node->getPos()->print() << endl;
        cerr << "- PutArray length expression is not of type int" << endl;
        exit(1);
    }
    if (arr_semant->get_kind() != AST_Semant::Kind::Value || arr_semant->get_type() != TypeKind::ARRAY) {
        cerr << node->getPos()->print() << endl;
        cerr << "- PutArray array expression is not of array type" << endl;
        exit(1);
    }
}

// 语句->开始计时语句: starttime();
// STM: STARTTIME '(' ')' ';'
void AST_Semant_Visitor::visit(Starttime* node) { }

// 语句->停止计时语句: stoptime();
// STM: STOPTIME '(' ')' ';'
void AST_Semant_Visitor::visit(Stoptime* node) { }

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
void AST_Semant_Visitor::visit(Esc* node)
{
    for (auto s : *(node->sl)) {
        s->accept(*this);
    }
    node->exp->accept(*this);

    // 返回Exp类型
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Escape expression has no sub-semantics" << endl;
        exit(1);
    }
    semant_map->setSemant(node, exp_semant);
}

// 表达式->标识符: id
// EXP: ID
void AST_Semant_Visitor::visit(IdExp* node)
{
    // 返回变量类型
    string id = node->id;

    // 获取类成员变量
    if (is_fetch_class_var)
        if (name_maps->is_class_var(fetch_class_name, id)) {
            VarDecl* vd = name_maps->get_class_var(fetch_class_name, id);
            semant_map->setSemant(node, build_semant(vd->type));
            return;
        } else {
            cerr << node->getPos()->print() << endl;
            cerr << "- Class member variable does not exist: " << fetch_class_name << "->" << id << endl;
            exit(1);
        }

    // 获取类方法
    if (is_fetch_class_method)
        if (name_maps->is_method(fetch_class_name, id)) {
            Type* t = name_maps->get_method_type(fetch_class_name, id);
            auto semant = new AST_Semant(AST_Semant::Kind::MethodName, t->typeKind, monostate(), false);
            semant_map->setSemant(node, semant);
            return;
        } else {
            cerr << node->getPos()->print() << endl;
            cerr << "- Class method does not exist: " << fetch_class_name << "->" << id << endl;
            exit(1);
        }

    // 首先判断是否为类方法局部变量
    if (name_maps->is_method_var(current_class_name, current_method_name, id)) {
        VarDecl* vd = name_maps->get_method_var(current_class_name, current_method_name, id);
        semant_map->setSemant(node, build_semant(vd->type));
        return;
    }

    // 然后判断是否为方法参数
    if (name_maps->is_method_formal(current_class_name, current_method_name, id)) {
        Formal* f = name_maps->get_method_formal(current_class_name, current_method_name, id);
        semant_map->setSemant(node, build_semant(f->type));
        return;
    }

    // 然后判断是否为类成员变量
    if (name_maps->is_class_var(current_class_name, id)) {
        VarDecl* vd = name_maps->get_class_var(current_class_name, id);
        semant_map->setSemant(node, build_semant(vd->type));
        return;
    }

    // 最后判断是否为类方法
    if (name_maps->is_method(current_class_name, id)) {
        Type* t = name_maps->get_method_type(current_class_name, id);
        auto semant = new AST_Semant(AST_Semant::Kind::MethodName, t->typeKind, monostate(), false);
        semant_map->setSemant(node, semant);
        return;
    }

    // 最终判断是否为类
    if (name_maps->is_class(id)) {
        auto semant = new AST_Semant(AST_Semant::Kind::Value, TypeKind::CLASS, id, false);
        semant_map->setSemant(node, semant);
        return;
    }
}

// 表达式->整数: num
// EXP: NUM
void AST_Semant_Visitor::visit(IntExp* node)
{
    // 返回int
    semant_map->setSemant(node, new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, monostate(), false));
}

// 表达式->布尔常量: true | false
// EXP: TRUE | FALSE
void AST_Semant_Visitor::visit(BoolExp* node)
{
    // 返回int
    semant_map->setSemant(node, new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, monostate(), false));
}

// 表达式->数组访问: 数组表达式[下标表达式]
// EXP: EXP '[' EXP ']'
void AST_Semant_Visitor::visit(ArrayExp* node)
{
    node->arr->accept(*this);
    node->index->accept(*this);

    // 检查第一个Exp为array，第二个Exp为int
    AST_Semant* arr_semant = semant_map->getSemant(node->arr);
    AST_Semant* index_semant = semant_map->getSemant(node->index);
    if (arr_semant == nullptr || index_semant == nullptr) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Array access has no sub-semantics" << endl;
        exit(1);
    }
    if (arr_semant->get_kind() != AST_Semant::Kind::Value || arr_semant->get_type() != TypeKind::ARRAY) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Array access array expression is not of array type" << endl;
        exit(1);
    }
    if (index_semant->get_kind() != AST_Semant::Kind::Value || index_semant->get_type() != TypeKind::INT) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Array access index expression is not of type int" << endl;
        exit(1);
    }

    // 返回int类型
    semant_map->setSemant(node, new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, monostate(), true));
}

// 表达式->操作符: op
// EXP: [+-*/ COMP && ||] [-!]
void AST_Semant_Visitor::visit(OpExp* node) { }

// 表达式->二元操作: 左表达式 OP 右表达式
// EXP: EXP OP EXP
void AST_Semant_Visitor::visit(BinaryOp* node)
{
    node->left->accept(*this);
    node->right->accept(*this);

    // 检查Exp都是int或array
    AST_Semant* left_semant = semant_map->getSemant(node->left);
    AST_Semant* right_semant = semant_map->getSemant(node->right);
    if (left_semant == nullptr || right_semant == nullptr) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Binary operation has no sub-semantics" << endl;
        exit(1);
    }
    if (left_semant->get_kind() != AST_Semant::Kind::Value || right_semant->get_kind() != AST_Semant::Kind::Value) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Binary operation left or right is not of Value type" << endl;
        exit(1);
    }
    if (left_semant->get_type() == TypeKind::CLASS || right_semant->get_type() == TypeKind::CLASS) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Binary operation does not support class types" << endl;
        exit(1);
    }
    if (left_semant->get_type() != right_semant->get_type()) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Binary operation left and right types do not match" << endl;
        exit(1);
    }

    // 返回Exp类型
    semant_map->setSemant(node, new AST_Semant(AST_Semant::Kind::Value, left_semant->get_type(), left_semant->get_type_par(), false));
}

// 表达式->一元操作: OP 表达式
// EXP: '!' EXP | '-' EXP
void AST_Semant_Visitor::visit(UnaryOp* node)
{
    node->exp->accept(*this);

    // 检查Exp是int或array
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Unary operation has no sub-semantics" << endl;
        exit(1);
    }
    if (exp_semant->get_kind() != AST_Semant::Kind::Value) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Unary operation is not of Value type" << endl;
        exit(1);
    }
    if (exp_semant->get_type() == TypeKind::CLASS) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Unary operation does not support class types" << endl;
        exit(1);
    }

    // 返回Exp类型
    semant_map->setSemant(node, new AST_Semant(AST_Semant::Kind::Value, exp_semant->get_type(), exp_semant->get_type_par(), false));
}

// 表达式->this指针: this
// EXP: THIS
void AST_Semant_Visitor::visit(This* node)
{
    // 返回class
    semant_map->setSemant(node, new AST_Semant(AST_Semant::Kind::Value, TypeKind::CLASS, current_class_name, false));
}

// 表达式->类变量访问: 类对象.变量名
// EXP: EXP '.' ID
void AST_Semant_Visitor::visit(ClassVar* node)
{
    node->obj->accept(*this);

    // 检查Exp为class
    AST_Semant* obj_semant = semant_map->getSemant(node->obj);
    if (!is_class(obj_semant)) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Class variable access object is not of class type" << endl;
        exit(1);
    }

    // 处理类变量ID
    is_fetch_class_var = true;
    fetch_class_name = get<string>(obj_semant->get_type_par());
    node->id->accept(*this);
    is_fetch_class_var = false;

    // 检查Exp是否有id成员变量
    string var_name = node->id->id;
    string class_name = get<string>(obj_semant->get_type_par());
    if (!name_maps->is_class_var(class_name, var_name)) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Class variable access member variable does not exist" << endl;
        exit(1);
    }

    // 返回id成员变量类型
    VarDecl* vd = name_maps->get_class_var(class_name, var_name);
    semant_map->setSemant(node, build_semant(vd->type));
}

// 表达式->类方法调用: 类对象.方法名(形参列表)
// EXP: EXP '.' ID '(' EXPLIST ')'
void AST_Semant_Visitor::visit(CallExp* node)
{
    node->obj->accept(*this);

    // 检查Exp为class
    AST_Semant* obj_semant = semant_map->getSemant(node->obj);
    if (!is_class(obj_semant)) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Class method call object is not of class type" << endl;
        exit(1);
    }

    is_fetch_class_method = true;
    fetch_class_name = get<string>(obj_semant->get_type_par());
    node->name->accept(*this);
    is_fetch_class_method = false;

    for (auto e : *(node->par)) {
        e->accept(*this);
    }

    // 检查Exp是否有id方法
    string method_name = node->name->id;
    string class_name = get<string>(obj_semant->get_type_par());
    if (!name_maps->is_method(class_name, method_name)) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Class method call method does not exist" << endl;
        exit(1);
    }

    // 检查ExpList是否匹配参数列表
    vector<Formal*>* formal_list = name_maps->get_method_formal_list(class_name, method_name);
    vector<Exp*>* par_list = node->par;
    if (formal_list->size() != par_list->size()) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Class method call parameter count does not match" << endl;
        exit(1);
    }

    for (int i = 0; i < formal_list->size(); i++) {
        Formal* f = (*formal_list)[i];
        Exp* e = (*par_list)[i];

        Type* formal_type = f->type;
        AST_Semant* exp_semant = semant_map->getSemant(e);
        if (!is_assignable(formal_type, exp_semant)) {
            cerr << node->getPos()->print() << endl;
            cerr << "- Class method call parameter types do not match" << endl;
            exit(1);
        }
    }

    // 返回方法类型
    Type* t = name_maps->get_method_type(class_name, method_name);
    semant_map->setSemant(node, build_semant(t, false));
}

// 表达式->读取整数: getint()
// EXP: GETINT '(' ')'
void AST_Semant_Visitor::visit(GetInt* node)
{
    // 返回int
    semant_map->setSemant(node, new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, monostate(), false));
}

// 表达式->读取字符: getch()
// EXP: GETCH '(' ')'
void AST_Semant_Visitor::visit(GetCh* node)
{
    // 返回int
    semant_map->setSemant(node, new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, monostate(), false));
}

// 表达式->读取数组: getarray(数组变量)
// EXP: GETARRAY '(' EXP ')'
void AST_Semant_Visitor::visit(GetArray* node)
{
    node->exp->accept(*this);

    // 检查Exp是array
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr) {
        cerr << node->getPos()->print() << endl;
        cerr << "- GetArray statement has no sub-semantics" << endl;
        exit(1);
    }
    if (exp_semant->get_kind() != AST_Semant::Kind::Value || exp_semant->get_type() != TypeKind::ARRAY) {
        cerr << node->getPos()->print() << endl;
        cerr << "- GetArray statement expression is not of array type" << endl;
        exit(1);
    }

    // 返回int
    semant_map->setSemant(node, new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, monostate(), false));
}

// 表达式->获取数组长度: length(数组表达式)
// EXP: LENGTH '(' EXP ')'
void AST_Semant_Visitor::visit(Length* node)
{
    node->exp->accept(*this);

    // 检查Exp是array
    AST_Semant* exp_semant = semant_map->getSemant(node->exp);
    if (exp_semant == nullptr) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Length statement has no sub-semantics" << endl;
        exit(1);
    }
    if (exp_semant->get_kind() != AST_Semant::Kind::Value || exp_semant->get_type() != TypeKind::ARRAY) {
        cerr << node->getPos()->print() << endl;
        cerr << "- Length statement expression is not of array type" << endl;
        exit(1);
    }

    // 返回int
    semant_map->setSemant(node, new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, monostate(), false));
}


// 判断类型是否相同
bool AST_Semant_Visitor::is_assignable(AST_Semant* left, AST_Semant* right)
{
    if (left == nullptr || right == nullptr)
        return false;
    if (left->get_kind() != AST_Semant::Kind::Value || right->get_kind() != AST_Semant::Kind::Value)
        return false;
    if (left->get_type() != right->get_type())
        return false;
    // 判断是否为左值
    if (!left->is_lvalue())
        return false;
    // 判断类是否相同 或可向上转型
    if (left->get_type() == TypeKind::CLASS) {
        string left_class = get<string>(left->get_type_par());
        string right_class = get<string>(right->get_type_par());
        if (!name_maps->is_class(left_class) || !name_maps->is_class(right_class))
            return false;
        if (left_class != right_class) {
            set<string> right_ancestors = name_maps->get_ancestors(right_class);
            if (right_ancestors.find(left_class) == right_ancestors.end())
                return false;
        }
    }
    // 判断数组维数是否相同
    if (left->get_type() == TypeKind::ARRAY) {
        int left_arity = get<int>(left->get_type_par());
        int right_arity = get<int>(right->get_type_par());
        if (left_arity != right_arity)
            return false;
    }
    return true;
}

// 判断类型是否相同
bool AST_Semant_Visitor::is_assignable(Type* left, AST_Semant* right)
{
    if (left == nullptr || right == nullptr)
        return false;
    if (right->get_kind() != AST_Semant::Kind::Value)
        return false;
    if (left->typeKind != right->get_type())
        return false;
    // 判断类是否相同 或可向上转型
    if (left->typeKind == TypeKind::CLASS) {
        string left_class = left->cid->id;
        string right_class = get<string>(right->get_type_par());
        if (!name_maps->is_class(left_class) || !name_maps->is_class(right_class))
            return false;
        if (left_class != right_class) {
            set<string> right_ancestors = name_maps->get_ancestors(right_class);
            if (right_ancestors.find(left_class) == right_ancestors.end())
                return false;
        }
    }
    // 判断数组维数是否相同
    if (left->typeKind == TypeKind::ARRAY) {
        int left_arity = left->arity->val;
        int right_arity = get<int>(right->get_type_par());
        if (left_arity != right_arity)
            return false;
    }
    return true;
}

// 判断是否为类
bool AST_Semant_Visitor::is_class(AST_Semant* obj_semant)
{
    if (obj_semant == nullptr)
        return false;
    if (obj_semant->get_kind() != AST_Semant::Kind::Value || obj_semant->get_type() != TypeKind::CLASS)
        return false;
    string class_name = get<string>(obj_semant->get_type_par());
    return name_maps->is_class(class_name);
}

// 从Type构造AST_Semant
AST_Semant* AST_Semant_Visitor::build_semant(Type* type, bool is_lvalue)
{
    if (type->typeKind == TypeKind::CLASS)
        return new AST_Semant(AST_Semant::Kind::Value, TypeKind::CLASS, type->cid->id, is_lvalue);
    else if (type->typeKind == TypeKind::ARRAY)
        return new AST_Semant(AST_Semant::Kind::Value, TypeKind::ARRAY, type->arity->val, is_lvalue);
    else if (type->typeKind == TypeKind::INT)
        return new AST_Semant(AST_Semant::Kind::Value, TypeKind::INT, monostate(), is_lvalue);
    cerr << "build_semant: Unknown type" << endl;
    exit(1);
}