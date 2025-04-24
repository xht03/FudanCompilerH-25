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
    virtual void visit(QuadProgram *quad) = 0;
    virtual void visit(QuadFuncDecl *quad) = 0;
    virtual void visit(QuadBlock *quad) = 0;
    virtual void visit(QuadMove *quad) = 0;
    virtual void visit(QuadLoad *quad) = 0;
    virtual void visit(QuadStore *quad) = 0;
    virtual void visit(QuadMoveBinop *quad) = 0;
    virtual void visit(QuadCall *quad) = 0;
    virtual void visit(QuadMoveCall *quad) = 0;
    virtual void visit(QuadExtCall *quad) = 0;
    virtual void visit(QuadMoveExtCall *quad) = 0;
    virtual void visit(QuadLabel *quad) = 0;
    virtual void visit(QuadJump *quad) = 0;
    virtual void visit(QuadCJump *quad) = 0;
    virtual void visit(QuadPhi *quad) = 0;
    virtual void visit(QuadReturn *quad) = 0;
};

/*
Quad is a class that represents a quadruple in the intermediate representation.

包含以下成员：
- kind: Quad 的类型 (如 MOVE、LOAD、STORE 等)
- node: 与该 Quad 关联的树节点
- def: 由该 Quad 定义的临时变量集合
- use: 该 Quad 使用的临时变量集合

// 这些 Quad 是简化的树节点/子结构
// term 可以是临时变量 (TempExp)、常量 (Const)或名称 (字符串，来自 NAME)

Move:  temp <- term
Load:  temp <- mem(term)
Store: mem(term) <- term
MoveBinop: temp <- term op term
Call:  ExpStm(call) //ignore the result
ExtCall: ExpStm(extcall) //ignore the result
MoveCall: temp <- call
MoveExtCall: temp <- extcall
Label: label
Jump: jump label
CJump: cjump relop term, term, label, label
Phi:  temp <- list of {temp, label} //same as the Phi in the tree
*/

// Quad 的类型
enum class QuadKind { 
    PROGRAM, FUNCDECL, BLOCK,
    MOVE, LOAD, STORE, MOVE_BINOP, 
    CALL, MOVE_CALL, EXTCALL, MOVE_EXTCALL, 
    LABEL, JUMP, CJUMP,
    PHI, RETURN 
};

enum class QuadTermKind { 
    TEMP, CONST, MAME
};

// 表示终结符的类
// QuadTerm 可以表示三种类型的终结：temp, const, name (whichi is from a string label)
class QuadTerm {
  public:
    QuadTermKind kind;
    variant<monostate, TempExp *, int, string> term;
    QuadTerm(TempExp *temp) : term(temp), kind(QuadTermKind::TEMP) {}
    QuadTerm(int constant) : term(constant), kind(QuadTermKind::CONST) {}
    QuadTerm(string name) : term(name), kind(QuadTermKind::MAME) {}

    QuadTerm(Tree *node) {
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
    TempExp *get_temp();
    int get_const();
    string get_name();
};

class Quad {
  public:
    QuadKind kind;
    tree::Tree *node; // 以防需要访问 IR 源节点
    virtual void print(string &output_str, int indent, bool print_def_use) = 0;
    virtual void accept(QuadVisitor &v) = 0;
    Quad(QuadKind k, tree::Tree *node) : kind(k), node(node) {}
};

class QuadProgram : public Quad {
  public:
    vector<QuadFuncDecl*> *quadFuncDeclList;  // 所有函数的声明

    QuadProgram(tree::Program *node, vector<QuadFuncDecl*> *quadFuncDeclList) 
        : Quad(QuadKind::PROGRAM, node), quadFuncDeclList(quadFuncDeclList) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadFuncDecl : public Quad {
  public:
    vector<QuadBlock*> *quadblocklist;    // 所有块
    string funcname;                      // 函数名称
    vector<Temp*> *params;                // 参数列表
    int last_label_num;                   // 函数中使用的最后一个标签号
    int last_temp_num;                    // 函数中使用的最后一个临时变量号

    QuadFuncDecl(Tree *node, string funcname, vector<Temp*> *params, vector<QuadBlock*> *quadblocklist, int lln, int ltn)
        : Quad(QuadKind::FUNCDECL, node), params(params), quadblocklist(quadblocklist), funcname(funcname), last_label_num(lln), last_temp_num(ltn) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadBlock : public Quad {
  public:
    Label *entry_label;                   // 入口标签
    vector<tree::Label*> *exit_labels;    // 退出标签
    vector<QuadStm*> *quadlist;           // Quad 语句列表

    QuadBlock(Tree *node, vector<QuadStm*> *quadlist, Label *entry_label, vector<tree::Label*> *exit_labels)
        : Quad(QuadKind::BLOCK, node), entry_label(entry_label), exit_labels(exit_labels), quadlist(quadlist) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadStm : public Quad {
  public:
    QuadKind kind;      // 语句类型
    set<Temp*> *def;    // 该指令定义的临时变量集合
    set<Temp*> *use;    // 该指令使用的临时变量集合
    
    QuadStm(QuadKind k, Tree *node, set<Temp*> *def, set<Temp*> *use) 
        : Quad(k, node), kind(k), def(def), use(use) {}
    virtual void accept(QuadVisitor &v) = 0;
    virtual void print(string &output_str, int indent, bool print_def_use) = 0;
};

class QuadMove : public QuadStm{
  public:
    TempExp *dst;     // 目标临时变量
    QuadTerm *src;    // 源终结符

    QuadMove(Tree *node, TempExp *dst, QuadTerm *src, set<Temp*> *def, set<Temp*> *use) 
        : QuadStm(QuadKind::MOVE, node, def, use), dst(dst), src(src) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadLoad : public QuadStm{
  public:
    TempExp *dst;
    QuadTerm *src;
    QuadLoad(Tree *node, TempExp *dst, QuadTerm *src, set<Temp*> *def, set<Temp*> *use) 
        : QuadStm(QuadKind::LOAD, node, def, use), dst(dst), src(src) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadStore : public QuadStm{
  public:
    QuadTerm *src;    // 要存储的值
    QuadTerm *dst;    // 目标内存地址：mem(dst) 形式

    QuadStore(Tree *node, QuadTerm *src, QuadTerm *dst, set<Temp*> *def, set<Temp*> *use) 
        : QuadStm(QuadKind::STORE, node, def, use), src(src), dst(dst) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadMoveBinop : public QuadStm{
  public:
    TempExp *dst;
    QuadTerm *left;
    QuadTerm *right;
    string binop;

    QuadMoveBinop(Tree *node, TempExp *dst, QuadTerm *left, string binop, QuadTerm *right, set<Temp*> *def, set<Temp*> *use) 
        : QuadStm(QuadKind::MOVE_BINOP, node, def, use), dst(dst), left(left), binop(binop), right(right) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

//Call is always a load a call result to a temp
class QuadCall : public QuadStm{
  public:
    string name;                // 被调用的函数名
    QuadTerm *obj_term;         // 对象实例
    vector<QuadTerm*> *args;    // 参数列表
    TempExp *result_temp;       // 返回值的临时变量

    QuadCall(Tree *node, TempExp *result_temp, string name, QuadTerm *obj_term, vector<QuadTerm*> *args, set<Temp*> *def, set<Temp*> *use) 
        : QuadStm(QuadKind::CALL, node, def, use), result_temp(result_temp), name(name), obj_term(obj_term), args(args) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadMoveCall : public QuadStm{
  public:
    TempExp *dst;
    QuadCall *call;
    QuadMoveCall(Tree *node, TempExp *dst, QuadCall *call, set<Temp*> *def, set<Temp*> *use) 
        : QuadStm(QuadKind::MOVE_CALL, node, def, use), dst(dst), call(call) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadExtCall : public QuadStm{
  public:
    string extfun;              // 外部函数名
    vector<QuadTerm*> *args;    // 参数列表
    TempExp *result_temp;       // 返回值的临时变量

    QuadExtCall(Tree *node, TempExp *result_temp, string extfun, vector<QuadTerm*> *args, set<Temp*> *def, set<Temp*> *use) 
        : QuadStm(QuadKind::EXTCALL, node, def, use), extfun(extfun), result_temp(result_temp), args(args) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadMoveExtCall : public QuadStm{
  public:
    TempExp *dst;
    QuadExtCall *extcall;
    QuadMoveExtCall(Tree *node, TempExp *dst, QuadExtCall *extcall, set<Temp*> *def, set<Temp*> *use) 
        : QuadStm(QuadKind::MOVE_EXTCALL, node, def, use), dst(dst), extcall(extcall) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadLabel : public QuadStm{
  public:
    Label *label;
    QuadLabel(Tree *node, Label *label, set<Temp*> *def, set<Temp*> *use) 
        : QuadStm(QuadKind::LABEL, node, def, use), label(label) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadJump : public QuadStm{
  public:
    Label *label;
    QuadJump(Tree *node, Label *label, set<Temp*> *def, set<Temp*> *use) 
        : QuadStm(QuadKind::JUMP, node, def, use), label(label) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadCJump : public QuadStm{
  public:
    string relop;         // 关系运算符
    QuadTerm *left;       // 左操作数
    QuadTerm *right;      // 右操作数
    Label *t, *f;         // 真分支和假分支标签

    QuadCJump(Tree *node, string relop, QuadTerm *left, QuadTerm *right, Label *t, Label *f, set<Temp*> *def, set<Temp*> *use) 
        : QuadStm(QuadKind::CJUMP, node, def, use), relop(relop), left(left), right(right), t(t), f(f) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadPhi : public QuadStm {
  public:
    TempExp *temp;                            // 合并后的结果
    vector<pair<Temp*, Label*>> *args;        // (值, 标签) 列表
    
    QuadPhi(Tree *node, TempExp *temp, vector<pair<Temp*, Label*>> *args, set<Temp*> *def, set<Temp*> *use) 
        : QuadStm(QuadKind::PHI, node, def, use), temp(temp), args(args) {}
    void accept(QuadVisitor &v) {v.visit(this);};
    void print(string &output_str, int indent, bool print_def_use) override;
};

class QuadReturn : public QuadStm{
  public:
    QuadTerm *value;
    QuadReturn(Tree *node, QuadTerm *value, set<Temp*> *def, set<Temp*> *use) 
        : QuadStm(QuadKind::RETURN, node, def, use), value(value) {}
    void accept(QuadVisitor &v) {v.visit(this);}
    void print(string &output_str, int indent, bool print_def_use) override;
};

} //namespace quad

#endif
