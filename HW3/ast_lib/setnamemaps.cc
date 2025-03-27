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

void AST_Name_Map_Visitor::visit(Program* node) {
#ifdef DEBUG
    std::cout << "Visiting Program" << std::endl;
#endif

    if (node == nullptr) return;
    
    // 访问所有类声明
    if (node->cdl != nullptr) {
        for (auto cl : *(node->cdl)) {
            cl->accept(*this);
        }
    }

    // 访问主方法
    if (node->main != nullptr) {
        node->main->accept(*this);
    }
}

void AST_Name_Map_Visitor::visit(MainMethod* node) {
#ifdef DEBUG
    std::cout << "Visiting MainMethod" << std::endl;
#endif

    if (node == nullptr) return;

    // 设置当前上下文
    current_class = "";
    current_method = "main";

    // 添加主方法（和空类名）
    if (!name_maps->add_method("", "main", new Type(node->getPos()))) {
        cerr << "Error: Main method already exists" << endl;
        return;
    }

    // 访问方法内的变量声明列表
    if (node->vdl != nullptr) {
        for (auto vd : *(node->vdl)) {
            vd->accept(*this);
        }
    }

    // 访问方法内的 Stm 列表
    if (node->sl != nullptr) {
        for (auto s : *(node->sl)) {
            s->accept(*this);
        }
    }

    // 清空当前上下文
    current_method = "";
}

void AST_Name_Map_Visitor::visit(ClassDecl* node) {
#ifdef DEBUG
    std::cout << "Visiting ClassDecl" << std::endl;
#endif

    if (node == nullptr) return;

    // 添加类名
    if (!name_maps->add_class(node->id->id)) {
        cerr << "Error: Class " << node->id->id << " already exists" << endl;
        return;
    }

    // 添加类的继承关系
    if (node->eid != nullptr) {
        name_maps->add_class_hiearchy(node->id->id, node->eid->id);
    }

    // 设置当前上下文
    current_class = node->id->id;
    
    // 访问类内的变量声明列表
    if (node->vdl != nullptr) {
        for (auto vd : *(node->vdl)) {
            vd->accept(*this);
        }
    }

    // 访问类内的方法声明列表
    if (node->mdl != nullptr) {
        for (auto md : *(node->mdl)) {
            md->accept(*this);
        }
    }

    // 清空当前上下文
    current_class = "";
}

void AST_Name_Map_Visitor::visit(Type *node) {
#ifdef DEBUG
    std::cout << "Visiting Type" << std::endl;
#endif

    if (node == nullptr) return;

    // 如果是类类型，检查类名是否存在
    if (node->typeKind == TypeKind::CLASS && node->cid != nullptr) {
        string class_name = node->cid->id;

        if (!name_maps->is_class(class_name)) {
            cerr << "Error: Class " << class_name << " not found" << endl;
            return;
        }
    }

    // 如果是数组类型
    if (node->typeKind == TypeKind::ARRAY && node->arity != nullptr) {
        // 暂时不做处理
    }

    // INT 类型不需要处理
}

void AST_Name_Map_Visitor::visit(VarDecl* node) {
#ifdef DEBUG
    std::cout << "Visiting VarDecl" << std::endl;
#endif

    if (node == nullptr) return;

    // 访问类型
    if (node->type != nullptr) {
        node->type->accept(*this);
    }

    if (current_method.empty()) {
        // 如果是类变量
        if (!name_maps->add_class_var(current_class, node->id->id, node)) {
            cerr << "Error: Variable " << node->id->id << " in class " << current_class << " already exists" << endl;
            return;
        }
    } else {
        // 如果是方法内的局部变量
        if (!name_maps->add_method_var(current_class, current_method, node->id->id, node)) {
            cerr << "Error: Variable " << node->id->id << " in method " << current_method << " already exists" << endl;
            return;
        }
    }
}

void AST_Name_Map_Visitor::visit(MethodDecl* node) {
#ifdef DEBUG
    std::cout << "Visiting MethodDecl" << std::endl;
#endif

    if (node == nullptr) return;

    current_method = node->id->id;

    // 添加方法到当前类
    if (!name_maps->add_method(current_class, current_method, node->type)) {
        cerr << "Error: Method " << current_method << " in class " << current_class << " already exists" << endl;
        current_method = "";
        return;
    }

    // 访问方法的返回类型
    if (node->type != nullptr) {
        node->type->accept(*this);
    }

    // 访问方法的形参列表
    if (node->fl != nullptr) {
        vector<string> formal_names;
        for (auto formal : *(node->fl)) {
            formal->accept(*this);
            formal_names.push_back(formal->id->id);
        }
        name_maps->add_method_formal_list(current_class, current_method, formal_names);     // 添加形参列表
    }

    // 访问方法内的局部变量
    if (node->vdl != nullptr) {
        for (auto vd : *(node->vdl)) {
            vd->accept(*this);
        }
    }

    // 处理方法内的 Stm 列表
    if (node->sl != nullptr) {
        for (auto s : *(node->sl)) {
            s->accept(*this);
        }
    }

    current_method = "";
}

void AST_Name_Map_Visitor::visit(Formal* node) {
#ifdef DEBUG
    std::cout << "Visiting Formal" << std::endl;
#endif

    if (node == nullptr) return;

    // 访问形参类型
    if (node->type != nullptr) {
        node->type->accept(*this);
    }

    string formal_name = node->id->id;

    if (!name_maps->add_method_formal(current_class, current_method, formal_name, node)) {
        cerr << "Error: Formal " << formal_name << " in method " << current_method << "of class" << current_class << " already exists" << endl;
        return;
    }
}






