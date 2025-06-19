#ifndef _QUAD
#define _QUAD

#include <vector>
#include <string>
#include <variant>
#include "tinyxml2.hh"
#include "temp.hh"
#include "treep.hh"

using namespace std;
using namespace tree;

namespace quad {

class Quad;
class QuadProgram;
class QuadFuncDecl;
class QuadBlock;
class QuadStm;
class QuadMove;
class QuadLoad;
class QuadStore;
class QuadMoveBinop;
class QuadCall;
class QuadExtCall;
class QuadMoveCall;
class QuadMoveExtCall;
class QuadLabel;
class QuadJump;
class QuadCJump;
class QuadPhi;
class QuadReturn;

class QuadVisitor {
public:
    virtual void visit(QuadProgram* quad) = 0;
    virtual void visit(QuadFuncDecl* quad) = 0;
    virtual void visit(QuadBlock* quad) = 0;
    virtual void visit(QuadMove* quad) = 0;
    virtual void visit(QuadLoad* quad) = 0;
    virtual void visit(QuadStore* quad) = 0;
    virtual void visit(QuadMoveBinop* quad) = 0;
    virtual void visit(QuadCall* quad) = 0;
    virtual void visit(QuadMoveCall* quad) = 0;
    virtual void visit(QuadExtCall* quad) = 0;
    virtual void visit(QuadMoveExtCall* quad) = 0;
    virtual void visit(QuadLabel* quad) = 0;
    virtual void visit(QuadJump* quad) = 0;
    virtual void visit(QuadCJump* quad) = 0;
    virtual void visit(QuadPhi* quad) = 0;
    virtual void visit(QuadReturn* quad) = 0;
};

// - kind: 四元式的类型 (MOVE、LOAD、STORE)
// - node: 与该四元式关联的语法树节点(Tree Node)
// - def: 此四元式定义的临时变量集合(可能为空)
// - use: 此四元式使用的临时变量集合(可能为空)

// term 是语法树节点(Tree Nodes)的简化表示
//   - TempExp(临时变量)
//   - Const(常量)
//   - Name(字符串，来自语法树中的 NAME 节点)

enum class QuadKind {
    PROGRAM,
    FUNCDECL,
    BLOCK,
    MOVE,
    LOAD,
    STORE,
    MOVE_BINOP,
    CALL,
    MOVE_CALL,
    EXTCALL,
    MOVE_EXTCALL,
    LABEL,
    JUMP,
    CJUMP,
    PHI,
    RETURN
};

string quadKindToString(QuadKind kind);

// 四元式的类型
enum class QuadTermKind { TEMP, CONST, MAME };

// 四元式
class QuadTerm {
public:
    QuadTermKind kind;
    variant<monostate, TempExp*, int, string> term;
    QuadTerm(TempExp* temp)
        : term(temp)
        , kind(QuadTermKind::TEMP) { };
    QuadTerm(int constant)
        : term(constant)
        , kind(QuadTermKind::CONST) { };
    QuadTerm(string name)
        : term(name)
        , kind(QuadTermKind::MAME) { };

    QuadTerm(Tree* node)
    {
        if (node->getTreeKind() == Kind::NAME) {
            kind = QuadTermKind::MAME;
            term = static_cast<Name*>(node)->sname->name;
        } else if (node->getTreeKind() == Kind::CONST) {
            kind = QuadTermKind::CONST;
            term = static_cast<Const*>(node)->constVal;
        } else if (node->getTreeKind() == Kind::TEMPEXP) {
            kind = QuadTermKind::TEMP;
            term = static_cast<TempExp*>(node);
        }
    }

    string print();
    TempExp* get_temp();
    int get_const();
    string get_name();

    QuadTerm* clone() const;
};

class Quad {
public:
    QuadKind kind;
    tree::Tree* node; // 记录对应的IR结点
    virtual void print(string& output_str, int indent, bool print_def_use) = 0;
    virtual void accept(QuadVisitor& v) = 0;
    virtual Quad* clone() const = 0;
    Quad(QuadKind k, tree::Tree* node)
        : kind(k)
        , node(node) { };
};

class QuadProgram : public Quad {
public:
    vector<QuadFuncDecl*>* quadFuncDeclList;
    QuadProgram(tree::Program* node, vector<QuadFuncDecl*>* quadFuncDeclList)
        : Quad(QuadKind::PROGRAM, node)
        , quadFuncDeclList(quadFuncDeclList) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadProgram* clone() const override;
};

// 函数声明
// QuadFuncDecl: vector<QuadBlock>
class QuadFuncDecl : public Quad {
public:
    vector<QuadBlock*>* quadblocklist;
    string funcname;
    vector<Temp*>* params;
    int last_temp_num;
    int last_label_num;

    QuadFuncDecl(
        Tree* node, string funcname, vector<Temp*>* params, vector<QuadBlock*>* quadblocklist, int lln, int ltn)
        : Quad(QuadKind::FUNCDECL, node)
        , params(params)
        , quadblocklist(quadblocklist)
        , funcname(funcname)
        , last_label_num(lln)
        , last_temp_num(ltn) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadFuncDecl* clone() const override;
};

// 代码块: 语句列表
// QuadBlock: vector<QuadStm>
class QuadBlock : public Quad {
public:
    Label* entry_label;
    vector<tree::Label*>* exit_labels;
    vector<QuadStm*>* quadlist;
    QuadBlock(Tree* node, vector<QuadStm*>* quadlist, Label* entry_label, vector<tree::Label*>* exit_labels)
        : Quad(QuadKind::BLOCK, node)
        , entry_label(entry_label)
        , exit_labels(exit_labels)
        , quadlist(quadlist) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadBlock* clone() const override;
};

// ------------------------------------------------

static void renameDefUseSet(set<Temp*>* defUse, Temp* oldTemp, Temp* newTemp)
{
    assert(defUse->find(oldTemp) != defUse->end());
    defUse->erase(oldTemp);
    defUse->insert(newTemp);
}

static void removeUseSet(set<Temp*>* use, Temp* oldTemp)
{
    assert(use->find(oldTemp) != use->end());
    use->erase(oldTemp);
}

class QuadStm : public Quad {
public:
    QuadKind kind;
    set<Temp*>* def;
    set<Temp*>* use;
    QuadStm(QuadKind k, Tree* node, set<Temp*>* def, set<Temp*>* use)
        : Quad(k, node)
        , kind(k)
        , def(def)
        , use(use) { };
    virtual void accept(QuadVisitor& v) = 0;
    virtual void print(string& output_str, int indent, bool print_def_use) = 0;
    set<Temp*>* cloneTemps(const set<Temp*>* temps) const;

    virtual void renameDef(Temp* oldTemp, Temp* newTemp) = 0;
    virtual void renameUse(Temp* oldTemp, Temp* newTemp) = 0;
    virtual void constantUse(Temp* oldTemp, int value) = 0;
};

// 语句->读内存
// Load: temp <- mem(term)
class QuadLoad : public QuadStm {
public:
    TempExp* dst;
    QuadTerm* src;
    QuadLoad(Tree* node, TempExp* dst, QuadTerm* src, set<Temp*>* def, set<Temp*>* use)
        : QuadStm(QuadKind::LOAD, node, def, use)
        , dst(dst)
        , src(src) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadLoad* clone() const override;

    void renameDef(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(def, oldTemp, newTemp);
        assert(dst->temp == oldTemp);
        dst->temp = newTemp;
    }
    void renameUse(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(use, oldTemp, newTemp);
        assert(src->kind == QuadTermKind::TEMP);
        TempExp* temp = get<TempExp*>(src->term);
        assert(temp->temp == oldTemp);
        temp->temp = newTemp;
    }

    void constantUse(Temp* oldTemp, int value) override
    {
        removeUseSet(use, oldTemp);
        assert(src->kind == QuadTermKind::TEMP);
        src = new QuadTerm(value);
    }
};

// 语句->写内存
// Store: mem(term) <- term
class QuadStore : public QuadStm {
public:
    QuadTerm* src;
    QuadTerm* dst; // in the form: mem(dst)
    QuadStore(Tree* node, QuadTerm* src, QuadTerm* dst, set<Temp*>* def, set<Temp*>* use)
        : QuadStm(QuadKind::STORE, node, def, use)
        , src(src)
        , dst(dst) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadStore* clone() const override;

    void renameDef(Temp* oldTemp, Temp* newTemp) override { assert(false); }
    void renameUse(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(use, oldTemp, newTemp);
        if (src->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(src->term);
            if (temp->temp == oldTemp) {
                temp->temp = newTemp;
            }
        }
        if (dst->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(dst->term);
            if (temp->temp == oldTemp) {
                temp->temp = newTemp;
            }
        }
    }

    void constantUse(Temp* oldTemp, int value) override
    {
        removeUseSet(use, oldTemp);
        if (src->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(src->term);
            if (temp->temp == oldTemp) {
                src = new QuadTerm(value);
                return;
            }
        }
        if (dst->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(dst->term);
            if (temp->temp == oldTemp) {
                dst = new QuadTerm(value);
                return;
            }
        }
        assert(false);
    }
};

// 语句->赋值
// Move: temp <- term
class QuadMove : public QuadStm {
public:
    TempExp* dst;
    QuadTerm* src;
    QuadMove(Tree* node, TempExp* dst, QuadTerm* src, set<Temp*>* def, set<Temp*>* use)
        : QuadStm(QuadKind::MOVE, node, def, use)
        , dst(dst)
        , src(src) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadMove* clone() const override;

    void renameDef(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(def, oldTemp, newTemp);
        assert(dst->temp == oldTemp);
        dst->temp = newTemp;
    }
    void renameUse(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(use, oldTemp, newTemp);
        assert(src->kind == QuadTermKind::TEMP);
        TempExp* temp = get<TempExp*>(src->term);
        assert(temp->temp == oldTemp);
        temp->temp = newTemp;
    }

    void constantUse(Temp* oldTemp, int value) override
    {
        removeUseSet(use, oldTemp);
        assert(src->kind == QuadTermKind::TEMP);
        src = new QuadTerm(value);
    }
};

// 语句->二元操作赋值
// MoveBinop: temp <- term op term
class QuadMoveBinop : public QuadStm {
public:
    TempExp* dst;
    QuadTerm* left;
    QuadTerm* right;
    string binop;
    QuadMoveBinop(
        Tree* node, TempExp* dst, QuadTerm* left, string binop, QuadTerm* right, set<Temp*>* def, set<Temp*>* use)
        : QuadStm(QuadKind::MOVE_BINOP, node, def, use)
        , dst(dst)
        , left(left)
        , binop(binop)
        , right(right) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadMoveBinop* clone() const override;

    void renameDef(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(def, oldTemp, newTemp);
        assert(dst->temp == oldTemp);
        dst->temp = newTemp;
    }
    void renameUse(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(use, oldTemp, newTemp);
        if (left->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(left->term);
            if (temp->temp == oldTemp) {
                temp->temp = newTemp;
            }
        }
        if (right->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(right->term);
            if (temp->temp == oldTemp) {
                temp->temp = newTemp;
            }
        }
    }

    void constantUse(Temp* oldTemp, int value) override
    {
        removeUseSet(use, oldTemp);
        if (left->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(left->term);
            if (temp->temp == oldTemp) {
                left = new QuadTerm(value);
                return;
            }
        }
        if (right->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(right->term);
            if (temp->temp == oldTemp) {
                right = new QuadTerm(value);
                return;
            }
        }
        assert(false);
    }
};

// 语句->类方法函数调用 (忽略返回值)
// Call: ExpStm(call)
// Call is always a load a call result to a temp
class QuadCall : public QuadStm {
public:
    string name;
    QuadTerm* obj_term;
    vector<QuadTerm*>* args;
    QuadCall(Tree* node, string name, QuadTerm* obj_term, vector<QuadTerm*>* args, set<Temp*>* def, set<Temp*>* use)
        : QuadStm(QuadKind::CALL, node, def, use)
        , name(name)
        , obj_term(obj_term)
        , args(args) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadCall* clone() const override;

    void renameDef(Temp* oldTemp, Temp* newTemp) override { assert(false); }
    void renameUse(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(use, oldTemp, newTemp);
        if (obj_term->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(obj_term->term);
            if (temp->temp == oldTemp) {
                temp->temp = newTemp;
            }
        }
        for (auto& arg : *args) {
            if (arg->kind == QuadTermKind::TEMP) {
                TempExp* temp = get<TempExp*>(arg->term);
                if (temp->temp == oldTemp) {
                    temp->temp = newTemp;
                }
            }
        }
        assert(false);
    }

    void constantUse(Temp* oldTemp, int value) override
    {
        removeUseSet(use, oldTemp);
        if (obj_term->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(obj_term->term);
            if (temp->temp == oldTemp) {
                obj_term = new QuadTerm(value);
                return;
            }
        }
        for (auto& arg : *args) {
            if (arg->kind == QuadTermKind::TEMP) {
                TempExp* temp = get<TempExp*>(arg->term);
                if (temp->temp == oldTemp) {
                    arg = new QuadTerm(value);
                    return;
                }
            }
        }
        assert(false);
    }
};

// 语句->类方法函数调用 (需要赋值)
// MoveCall: temp <- call
class QuadMoveCall : public QuadStm {
public:
    TempExp* dst;
    QuadCall* call;
    QuadMoveCall(Tree* node, TempExp* dst, QuadCall* call, set<Temp*>* def, set<Temp*>* use)
        : QuadStm(QuadKind::MOVE_CALL, node, def, use)
        , dst(dst)
        , call(call) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadMoveCall* clone() const override;

    void renameDef(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(def, oldTemp, newTemp);
        assert(dst->temp == oldTemp);
        dst->temp = newTemp;
    }
    void renameUse(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(use, oldTemp, newTemp);
        if (call->obj_term->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(call->obj_term->term);
            if (temp->temp == oldTemp) {
                temp->temp = newTemp;
            }
        }
        for (auto& arg : *call->args) {
            if (arg->kind == QuadTermKind::TEMP) {
                TempExp* temp = get<TempExp*>(arg->term);
                if (temp->temp == oldTemp) {
                    temp->temp = newTemp;
                }
            }
        }
    }

    void constantUse(Temp* oldTemp, int value) override
    {
        removeUseSet(use, oldTemp);
        if (call->obj_term->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(call->obj_term->term);
            if (temp->temp == oldTemp) {
                call->obj_term = new QuadTerm(value);
                return;
            }
        }
        for (auto& arg : *call->args) {
            if (arg->kind == QuadTermKind::TEMP) {
                TempExp* temp = get<TempExp*>(arg->term);
                if (temp->temp == oldTemp) {
                    arg = new QuadTerm(value);
                    return;
                }
            }
        }
        assert(false);
    }
};

// 语句->外部函数调用 (忽略返回值)
// ExtCall: ExpStm(extcall)
class QuadExtCall : public QuadStm {
public:
    string extfun;
    vector<QuadTerm*>* args;
    QuadExtCall(Tree* node, string extfun, vector<QuadTerm*>* args, set<Temp*>* def, set<Temp*>* use)
        : QuadStm(QuadKind::EXTCALL, node, def, use)
        , extfun(extfun)
        , args(args) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadExtCall* clone() const override;

    void renameDef(Temp* oldTemp, Temp* newTemp) override { assert(false); }
    void renameUse(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(use, oldTemp, newTemp);
        for (auto& arg : *args) {
            if (arg->kind == QuadTermKind::TEMP) {
                TempExp* temp = get<TempExp*>(arg->term);
                if (temp->temp == oldTemp) {
                    temp->temp = newTemp;
                }
            }
        }
    }

    void constantUse(Temp* oldTemp, int value) override
    {
        removeUseSet(use, oldTemp);
        for (auto& arg : *args) {
            if (arg->kind == QuadTermKind::TEMP) {
                TempExp* temp = get<TempExp*>(arg->term);
                if (temp->temp == oldTemp) {
                    arg = new QuadTerm(value);
                    return;
                }
            }
        }
        assert(false);
    }
};

// 语句->外部函数调用 (需要赋值)
// MoveExtCall: temp <- extcall
class QuadMoveExtCall : public QuadStm {
public:
    TempExp* dst;
    QuadExtCall* extcall;
    QuadMoveExtCall(Tree* node, TempExp* dst, QuadExtCall* extcall, set<Temp*>* def, set<Temp*>* use)
        : QuadStm(QuadKind::MOVE_EXTCALL, node, def, use)
        , dst(dst)
        , extcall(extcall) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadMoveExtCall* clone() const override;

    void renameDef(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(def, oldTemp, newTemp);
        assert(dst->temp == oldTemp);
        dst->temp = newTemp;
    }
    void renameUse(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(use, oldTemp, newTemp);
        for (auto& arg : *extcall->args) {
            if (arg->kind == QuadTermKind::TEMP) {
                TempExp* temp = get<TempExp*>(arg->term);
                if (temp->temp == oldTemp) {
                    temp->temp = newTemp;
                }
            }
        }
    }

    void constantUse(Temp* oldTemp, int value) override
    {
        removeUseSet(use, oldTemp);
        for (auto& arg : *extcall->args) {
            if (arg->kind == QuadTermKind::TEMP) {
                TempExp* temp = get<TempExp*>(arg->term);
                if (temp->temp == oldTemp) {
                    arg = new QuadTerm(value);
                    return;
                }
            }
        }
        assert(false);
    }
};

// 语句->标签
// Label: label
class QuadLabel : public QuadStm {
public:
    Label* label;
    QuadLabel(Tree* node, Label* label, set<Temp*>* def, set<Temp*>* use)
        : QuadStm(QuadKind::LABEL, node, def, use)
        , label(label) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadLabel* clone() const override;

    void renameDef(Temp* oldTemp, Temp* newTemp) override { assert(false); }
    void renameUse(Temp* oldTemp, Temp* newTemp) override { assert(false); }
    void constantUse(Temp* oldTemp, int value) override { assert(false); }
};

// 语句->跳转
// Jump: jump label
class QuadJump : public QuadStm {
public:
    Label* label;
    QuadJump(Tree* node, Label* label, set<Temp*>* def, set<Temp*>* use)
        : QuadStm(QuadKind::JUMP, node, def, use)
        , label(label) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadJump* clone() const override;

    void renameDef(Temp* oldTemp, Temp* newTemp) override { assert(false); }
    void renameUse(Temp* oldTemp, Temp* newTemp) override { assert(false); }
    void constantUse(Temp* oldTemp, int value) override { assert(false); }
};

// 语句->条件跳转
// CJump: cjump relop term, term, label, label
class QuadCJump : public QuadStm {
public:
    string relop;
    QuadTerm* left;
    QuadTerm* right;
    Label *t, *f;
    QuadCJump(
        Tree* node, string relop, QuadTerm* left, QuadTerm* right, Label* t, Label* f, set<Temp*>* def, set<Temp*>* use)
        : QuadStm(QuadKind::CJUMP, node, def, use)
        , relop(relop)
        , left(left)
        , right(right)
        , t(t)
        , f(f) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadCJump* clone() const override;

    void renameDef(Temp* oldTemp, Temp* newTemp) override { assert(false); }
    void renameUse(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(use, oldTemp, newTemp);
        if (left->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(left->term);
            if (temp->temp == oldTemp) {
                temp->temp = newTemp;
            }
        }
        if (right->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(right->term);
            if (temp->temp == oldTemp) {
                temp->temp = newTemp;
            }
        }
    }

    void constantUse(Temp* oldTemp, int value) override
    {
        removeUseSet(use, oldTemp);
        if (left->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(left->term);
            if (temp->temp == oldTemp) {
                left = new QuadTerm(value);
                return;
            }
        }
        if (right->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(right->term);
            if (temp->temp == oldTemp) {
                right = new QuadTerm(value);
                return;
            }
        }
        assert(false);
    }
};

// 语句->Phi函数
// Phi: temp <- {<temp, label>}
class QuadPhi : public QuadStm {
public:
    TempExp* temp;
    vector<pair<Temp*, Label*>>* args;
    QuadPhi(Tree* node, TempExp* temp, vector<pair<Temp*, Label*>>* args, set<Temp*>* def, set<Temp*>* use)
        : QuadStm(QuadKind::PHI, node, def, use)
        , temp(temp)
        , args(args) { };
    void accept(QuadVisitor& v) { v.visit(this); };
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadPhi* clone() const override;

    void renameDef(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(def, oldTemp, newTemp);
        assert(temp->temp == oldTemp);
        temp->temp = newTemp;
    }
    void renameUse(Temp* oldTemp, Temp* newTemp) override { 
        renameDefUseSet(use, oldTemp, newTemp);
        for (auto& arg : *args) {
            if (arg.first == oldTemp) {
                arg.first = newTemp;
            }
        }
     }
    void constantUse(Temp* oldTemp, int value) override { }
};

// 语句->返回语句
// Return: return term
class QuadReturn : public QuadStm {
public:
    QuadTerm* value;
    QuadReturn(Tree* node, QuadTerm* value, set<Temp*>* def, set<Temp*>* use)
        : QuadStm(QuadKind::RETURN, node, def, use)
        , value(value) { };
    void accept(QuadVisitor& v) { v.visit(this); }
    void print(string& output_str, int indent, bool print_def_use) override;
    QuadReturn* clone() const override;

    void renameDef(Temp* oldTemp, Temp* newTemp) override { assert(false); }
    void renameUse(Temp* oldTemp, Temp* newTemp) override
    {
        renameDefUseSet(use, oldTemp, newTemp);
        if (value->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(value->term);
            if (temp->temp == oldTemp) {
                temp->temp = newTemp;
            }
        }
    }

    void constantUse(Temp* oldTemp, int value) override
    {
        removeUseSet(use, oldTemp);
        if (this->value->kind == QuadTermKind::TEMP) {
            TempExp* temp = get<TempExp*>(this->value->term);
            if (temp->temp == oldTemp) {
                this->value = new QuadTerm(value);
                return;
            }
        }
        assert(false);
    }
};

} // namespace quad

void quad2file(quad::Quad* quad, string filename, bool print_def_use);

#endif
