#define DEBUG
#undef DEBUG

#include "executor.hh"
#include "ASTheader.hh"
#include "FDMJAST.hh"
#include <iostream>
#include <string>
#include <variant>
#include <vector>
#include <map>

using namespace std;
using namespace fdmj;

#define StmList vector<Stm*>
#define ExpList vector<Exp*>

int execute(Program* root)
{
    if (root == nullptr)
        return 0;
    Executor v(nullptr);
    root->accept(v);

    if (v.newNode->getASTKind() != ASTKind::Program) {
        cerr << "> execute: No Program" << endl;
        return 0;
    }

    v.newNode = static_cast<Program*>(v.newNode)->main;
    if (v.newNode->getASTKind() != ASTKind::MainMethod) {
        cerr << "> execute: No MainMethod" << endl;
        return 0;
    }

    StmList* sl = static_cast<MainMethod*>(v.newNode)->sl;
    if (sl == nullptr) {
        cerr << "> execute: No StmList" << endl;
        return 0;
    }

    AST *node = sl->back();
    if (node->getASTKind() != ASTKind::Return) {
        cerr << "> execute: No Return" << endl;
        return 0;
    }

    Return* Rnode = static_cast<Return*>(node);
    int ret = 0;
    if (Rnode->exp == nullptr) {
        cerr << "> execute: No Return expression" << endl;
        return 0;
    } else if (Rnode->exp->getASTKind() == ASTKind::IntExp) {
        ret = static_cast<IntExp*>(Rnode->exp)->val;
    } else if (Rnode->exp->getASTKind() == ASTKind::IdExp) {
        string varname = static_cast<IdExp*>(Rnode->exp)->id;
        if (v.symtab.find(varname) != v.symtab.end()) {
            ret = v.symtab[varname];
        } else {
            cerr << "> execute: Variable " << varname << " not defined in Return" << endl;
            return 0;
        }
    } else {
        cerr << "> execute: Invalid expression type in Return" << endl;
        return 0;
    }
    return ret;
}

template <typename T> static vector<T*>* visitList(Executor& v, vector<T*>* tl)
{
    if (tl == nullptr || tl->size() == 0)
        return nullptr;
    vector<T*>* vt = new vector<T*>();
    for (T* x : *tl) {
        if (x == nullptr)
            continue;
        x->accept(v);
        if (v.newNode == nullptr)
            continue;
        vt->push_back(static_cast<T*>(v.newNode));
    }
    if (vt->size() == 0) {
        delete vt;
        vt = nullptr;
    }
    return vt;
}

void Executor::visit(Program* node)
{
    if (node == nullptr) {
        newNode = nullptr;
        return;
    }

    // 访问 main 函数节点（如果存在）
    MainMethod* m;
    if (node->main == nullptr) {
        m = nullptr;
    } else {
        node->main->accept(*this);
        if (newNode == nullptr)
            m = nullptr;
        else
            m = static_cast<MainMethod*>(newNode);
    }
    newNode = new Program(node->getPos()->clone(), m);
}

void Executor::visit(MainMethod* node)
{
    if (node == nullptr) {
        newNode = nullptr;
        return;
    }

    StmList* sl = nullptr;
    if (node->sl != nullptr)
        sl = visitList<Stm>(*this, node->sl);
    newNode = new MainMethod(node->getPos()->clone(), sl);
}

void Executor::visit(Assign* node)
{
    if (node == nullptr) {
        newNode = nullptr;
        return;
    }

    if (node->left == nullptr) {
        cerr << "Error: No left expression found in the Assign statement" << endl;
        newNode = nullptr;
        return;
    }

    if (node->exp == nullptr) {
        cerr << "Error: No right expression found in the Assign statement" << endl;
        newNode = nullptr;
        return;
    }

    Exp* l = nullptr;
    node->left->accept(*this);
    l = static_cast<Exp*>(newNode);

    Exp* r = nullptr;
    node->exp->accept(*this);
    r = static_cast<Exp*>(newNode); 

    if (l->getASTKind() != ASTKind::IdExp) {
        cerr << "Error: Left expression is not an IdExp in Assign" << endl;
        newNode = nullptr;
        return;
    }
    string varname = static_cast<IdExp*>(l)->id;

    int val = 0;
    if (r->getASTKind() == ASTKind::IntExp) {
        val = static_cast<IntExp*>(r)->val;
    } else if (r->getASTKind() == ASTKind::IdExp) {
        string rvarname = static_cast<IdExp*>(r)->id;
        if (symtab.find(rvarname) != symtab.end()) {
            val = symtab[rvarname];
        } else {
            cerr << "Error: Variable " << rvarname << " not defined in Assign" << endl;
            newNode = nullptr;
            return;
        }
    } else {
        cerr << "Error: Invalid right expression type in Assign" << endl;
        newNode = nullptr;
        return;
    }
    // 更新变量表
    symtab[varname] = val;

    newNode = new Assign(node->getPos()->clone(), l, r);
}

void Executor::visit(Return* node)
{
    if (node == nullptr) {
        newNode = nullptr;
        return;
    }

    if ( node->exp == nullptr) {
        cerr << "Error: No expression found in the Return statement" << endl;
        newNode = nullptr;
        return;
    }

    Exp* e = nullptr;
    node->exp->accept(*this);
    e = static_cast<Exp*>(newNode);

    // int val = 0;
    // if (e->getASTKind() == ASTKind::IntExp) {
    //     val = static_cast<IntExp*>(e)->val;
    // } else if (e->getASTKind() == ASTKind::IdExp) {
    //     string varname = static_cast<IdExp*>(e)->id;
    //     if (symtab.find(varname) != symtab.end()) {
    //         val = symtab[varname];
    //     } else {
    //         cerr << "Error: Variable " << varname << " not defined in Return" << endl;
    //         newNode = nullptr;
    //         return;
    //     }
    // } else {
    //     cerr << "Error: Invalid expression type in Return" << endl;
    //     newNode = nullptr;
    //     return;
    // }

    newNode = new Return(node->getPos()->clone(), e);
}

void Executor::visit(BinaryOp* node)
{
    if (node == nullptr) {
        newNode = nullptr;
        return;
    }

    if (node->left == nullptr) {
        cerr << "Error: No left expression found in the BinaryOp" << endl;
        newNode = nullptr;
        return;
    }

    if (node->right == nullptr) {
        cerr << "Error: No right expression found in the BinaryOp" << endl;
        newNode = nullptr;
        return;
    }

    if (node->op == nullptr) {
        cerr << "Error: No operator found in the BinaryOp" << endl;
        newNode = nullptr;
        return;
    }

    Exp* l = nullptr;
    node->left->accept(*this);
    l = static_cast<Exp*>(newNode);

    Exp* r = nullptr;
    node->right->accept(*this);
    r = static_cast<Exp*>(newNode);

    int lval = 0;
    int rval = 0;

    // 判断左右 Exp 是否合法
    if (l->getASTKind() == ASTKind::IntExp) {
        lval = static_cast<IntExp*>(l)->val;
    } else if (l->getASTKind() == ASTKind::IdExp) {
        string varname = static_cast<IdExp*>(l)->id;
        if (symtab.find(varname) != symtab.end()) {
            lval = symtab[varname];
        } else {
            cerr << "Error: Variable " << varname << " not defined in BinaryOp" << endl;
            newNode = nullptr;
            return;
        }
    } else {
        cerr << "Error: Invalid left expression in BinaryOp" << endl;
        newNode = nullptr;
        return;
    }

    if (r->getASTKind() == ASTKind::IntExp) {
        rval = static_cast<IntExp*>(r)->val;
    } else if (r->getASTKind() == ASTKind::IdExp) {
        string varname = static_cast<IdExp*>(r)->id;
        if (symtab.find(varname) != symtab.end()) {
            rval = symtab[varname];
        } else {
            cerr << "Error: Variable " << varname << " not defined in BinaryOp" << endl;
            newNode = nullptr;
            return;
        }
    } else {
        cerr << "Error: Invalid right expression in BinaryOp" << endl;
        newNode = nullptr;
        return;
    }

    // 执行计算
    int val = 0;

    if (node->op->op == "+") {
        val = lval + rval;
    } else if (node->op->op == "-") {
        val = lval - rval;
    } else if (node->op->op == "*") {
        val = lval * rval;
    } else if (node->op->op == "/") {
        val = lval / rval;
    } else if (node->op->op == "%") {
        val = lval % rval;
    } else {
        cerr << "Error: Unsupported operator in BinaryOp" << endl;
        newNode = nullptr;
        return;
    }

    newNode = new IntExp(node->getPos()->clone(), val);
}

void Executor::visit(UnaryOp* node)
{
    if (node == nullptr) {
        newNode = nullptr;
        return;
    }

    if (node->exp == nullptr) {
        cerr << "Error: No expression found in UnaryOp" << endl;
        newNode = nullptr;
        return;
    }

    if (node->op == nullptr) {
        cerr << "Error: No operator found in the UnaryOp statement" << endl;
        newNode = nullptr;
        return;
    }

    Exp* e = nullptr;
    node->exp->accept(*this);
    e = static_cast<Exp*>(newNode);

    int eval = 0;
    if (e->getASTKind() == ASTKind::IntExp) {
        eval = static_cast<IntExp*>(e)->val;
    } else if (e->getASTKind() == ASTKind::IdExp) {
        string varname = static_cast<IdExp*>(e)->id;
        if (symtab.find(varname) != symtab.end()) {
            eval = symtab[varname];
        } else {
            cerr << "Error: Variable " << varname << " not defined in UnaryOp" << endl;
            newNode = nullptr;
            return;
        }
    } else {
        cerr << "Error: Invalid expression type in UnaryOp" << endl;
        newNode = nullptr;
        return;
    }

    // 执行计算
    int val = 0;
    if (node->op->op == "-") {
        val = -eval;
    } else {
        cerr << "Error: Unsupported operator in UnaryOp" << endl;
        newNode = nullptr;
        return;
    }

    newNode = new IntExp(node->getPos()->clone(), val);
}

void Executor::visit(Esc* node)
{
    StmList* sl = nullptr;
    Exp* e = nullptr;

    if (node == nullptr) {
        newNode = nullptr;
        return;
    }
    if (node->sl != nullptr)
        sl = visitList<Stm>(*this, node->sl);
    if (node->exp != nullptr) {
        node->exp->accept(*this);
        e = static_cast<Exp*>(newNode);
    }

    // 直接返回下层 Exp 节点 (因而不修改 newNode)
}

void Executor::visit(IdExp* node)
{
    newNode = (node == nullptr) ? nullptr : static_cast<IdExp*>(node->clone());
}

void Executor::visit(OpExp* node)
{
    newNode = (node == nullptr) ? nullptr : static_cast<OpExp*>(node->clone());
}

void Executor::visit(IntExp* node)
{
    newNode = (node == nullptr) ? nullptr : static_cast<IntExp*>(node->clone());
}
