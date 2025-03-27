#define DEBUG
#undef DEBUG

#include <iostream>
#include <variant>
#include <map>
#include <vector>
#include <algorithm>
#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "namemaps.hh"

using namespace std;
using namespace fdmj;

// 1. 判断参数非空
// 2. 执行名称映射操作
// 3. 更新当前类名或方法名
// 4. 调用accept(visit)

// 程序: 主方法 类声明列表
// PROG: MAINMETHOD CLASSDECLLIST
void AST_Name_Map_Visitor::visit(Program* node)
{
    node->main->accept(*this);
    for (auto cl : *(node->cdl)) {
        cl->accept(*this);
    }
}

// 主方法: public int main() { 变量声明列表 语句列表 }
// MAINMETHOD: PUBLIC INT MAIN '(' ')' '{' VARDECLLIST STMLIST '}';
void AST_Name_Map_Visitor::visit(MainMethod* node)
{
    // 执行名称映射操作 (类, 类->方法)
    if (!name_maps->add_class("")) {
        cerr << node->getPos()->print() << endl;
        cerr << "- 主方法类名已存在" << endl;
        exit(1);
    }
    if (!name_maps->add_method("", "main", new Type(node->getPos()))) {
        cerr << node->getPos()->print() << endl;
        cerr << "- 主方法名已存在" << endl;
        exit(1);
    }

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

    // 恢复当前类名和方法名
    current_class_name = "";
    current_method_name = "";
}


bool is_same_type(Type* t1, Type* t2)
{
    if (t1->typeKind != t2->typeKind)
        return false;
    if (t1->typeKind == TypeKind::CLASS && t1->cid->id != t2->cid->id)
        return false;
    if (t1->typeKind == TypeKind::ARRAY && t1->arity->val != t2->arity->val)
        return false;
    return true;
}

// 类声明: 类名 [基类名] { 变量声明列表 方法声明列表 }
// CLASSDECL: PUBLIC CLASS ID '{' VARDECLLIST METHODDECLLIST '}'
//          | PUBLIC CLASS ID EXTENDS ID '{' VARDECLLIST METHODDECLLIST '}'
void AST_Name_Map_Visitor::visit(ClassDecl* node)
{
    // 执行名称映射操作 (类)
    if (!name_maps->add_class(node->id->id)) {
        cerr << node->id->getPos()->print() << endl;
        cerr << "- 类名已存在: " << node->id->id << endl;
        exit(1);
    }

    if (node->eid != nullptr) {
        if (!name_maps->add_class_hiearchy(node->id->id, node->eid->id)) {
            cerr << node->eid->getPos()->print() << endl;
            cerr << "- 类继承关系错误: " << node->id->id << " extends " << node->eid->id << endl;
            exit(1);
        }
        if (!name_maps->is_class(node->eid->id)) {
            cerr << node->eid->getPos()->print() << endl;
            cerr << "- 父类不存在: " << node->id->id << " extends " << node->eid->id << endl;
            exit(1);
        }

        // 子类从其所有祖先类继承类变量
        // 子类不允许覆盖父类变量
        vector<VarDecl*>* vl = name_maps->get_class_var(node->eid->id);
        for (auto v : *vl)
            name_maps->add_class_var(node->id->id, v->id->id, v);
    }

    // 更新当前类名
    current_class_name = node->id->id;

    // 调用accept(visit)
    for (auto vd : *(node->vdl)) {
        vd->accept(*this);
    }
    for (auto md : *(node->mdl)) {
        md->accept(*this);
    }

    // 恢复当前类名
    current_class_name = "";

    if (node->eid != nullptr) {
        // 子类从其所有祖先类继承类方法
        // 子类可以覆盖其祖先类中声明的方法, 但必须具有相同(返回类型,参数类型)
        // 所以在子类的方法初始完之后, 再检查是否覆盖了父类的方法
        vector<string>* ml = name_maps->get_method(node->eid->id);
        for (auto m : *ml) {
            // 如果存在同名方法, 则检查是否可覆盖
            if (name_maps->is_method(node->id->id, m)) {
                // 检查返回类型是否相同
                Type* t1 = name_maps->get_method_type(node->eid->id, m);
                Type* t2 = name_maps->get_method_type(node->id->id, m);
                if (!is_same_type(t1, t2)) {
                    cerr << node->id->getPos()->print() << endl;
                    cerr << "- 覆写类方法返回类型不同: " << node->id->id << "->" << m << endl;
                    exit(1);
                }

                // 检查每个参数类型是否相同
                vector<Formal*>* fsl1 = name_maps->get_method_formal_list(node->eid->id, m);
                vector<Formal*>* fsl2 = name_maps->get_method_formal_list(node->id->id, m);
                if (fsl1->size() != fsl2->size()) {
                    cerr << node->id->getPos()->print() << endl;
                    cerr << "- 覆写类方法参数个数不同: " << node->id->id << "->" << m << endl;
                    exit(1);
                }
                for (int i = 0; i < fsl1->size(); i++) {
                    Formal* f1 = fsl1->at(i);
                    Formal* f2 = fsl2->at(i);
                    if (!is_same_type(f1->type, f2->type)) {
                        cerr << node->id->getPos()->print() << endl;
                        cerr << "- 覆写类方法参数类型不同: " << node->id->id << "->" << m << "->" << f1->id->id << endl;
                        exit(1);
                    }
                }
            }

            // 如果不存在同名方法, 直接添加
            else {
                Type* t = name_maps->get_method_type(node->eid->id, m);
                name_maps->add_method(node->id->id, m, t);

                vector<string> fsl;
                vector<Formal*>* fl = name_maps->get_method_formal_list(node->eid->id, m);
                for (auto f : *fl) {
                    fsl.push_back(f->id->id);
                    if (!name_maps->add_method_formal(node->id->id, m, f->id->id, f)) {
                        cerr << f->id->getPos()->print() << endl;
                        cerr << "- 类方法参数名已存在: " << node->id->id << "->" << m << "->" << f->id->id << endl;
                        exit(1);
                    }
                }
                if (!name_maps->add_method_formal_list(node->id->id, m, fsl)) {
                    cerr << node->id->getPos()->print() << endl;
                    cerr << "- 类方法参数列表已存在: " << node->id->id << "->" << m << endl;
                    exit(1);
                }
            }
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
void AST_Name_Map_Visitor::visit(VarDecl* node)
{
    // 执行名称映射操作 (类->方法->变量)
    if (current_method_name != "") {
        if (!name_maps->add_method_var(current_class_name, current_method_name, node->id->id, node)) {
            cerr << node->id->getPos()->print() << endl;
            cerr << "- 类方法局部变量名已存在: " << current_class_name << "->" << current_method_name << "->" << node->id->id << endl;
            exit(1);
        }
    }

    // 执行名称映射操作 (类->成员变量)
    else {
        if (!name_maps->add_class_var(current_class_name, node->id->id, node)) {
            cerr << node->id->getPos()->print() << endl;
            cerr << "- 类成员变量名已存在: " << current_class_name << "->" << node->id->id << endl;
            exit(1);
        }
    }
}

// 方法声明: 返回类型 方法名(形参列表) { 变量声明列表 语句列表 }
// METHODDECL: PUBLIC TYPE ID '(' FORMALLIST ')' '{' VARDECLLIST STMLIST '}'
void AST_Name_Map_Visitor::visit(MethodDecl* node)
{
    // 执行名称映射操作 (类->方法, 类->方法->参数列表)
    if (!name_maps->add_method(current_class_name, node->id->id, node->type)) {
        // TODO: 子类可以覆盖其祖先类中声明的方法, 但必须具有相同(返回类型,参数类型)
        cerr << node->id->getPos()->print() << endl;
        cerr << "- 类方法名已存在: " << current_class_name << "->" << current_method_name << endl;
        exit(1);
    }
    {
        vector<string> vl;
        for (auto f : *(node->fl))
            vl.push_back(f->id->id);
        if (!name_maps->add_method_formal_list(current_class_name, node->id->id, vl)) {
            cerr << node->id->getPos()->print() << endl;
            cerr << "- 类方法参数列表已存在: " << current_class_name << "->" << node->id->id << endl;
            exit(1);
        }
    }

    // 更新当前方法名
    current_method_name = node->id->id;

    for (auto f : *(node->fl)) {
        f->accept(*this);
    }
    for (auto vd : *(node->vdl)) {
        vd->accept(*this);
    }
    for (auto s : *(node->sl)) {
        s->accept(*this);
    }

    // 恢复当前方法名
    current_method_name = "";
}

// 形参: 类型 变量名
// FORMAL: TYPE ID
void AST_Name_Map_Visitor::visit(Formal* node)
{
    // 执行名称映射操作 (类->方法->参数)
    if (!name_maps->add_method_formal(current_class_name, current_method_name, node->id->id, node)) {
        cerr << node->id->getPos()->print() << endl;
        cerr << "- 类方法参数名已存在: " << current_class_name << "->" << current_method_name << "->" << node->id->id << endl;
        exit(1);
    }
}
