#ifndef _TREEP
#define _TREEP

#include <vector>
#include <cstdio>
#include <string>
#include <cassert>
#include "temp.hh"

using namespace std;
using namespace tree;

namespace tree {
class Tree;     // abstract class
class Program;  // member: funcdecllist
class FuncDecl; // members: name, args, stm
class Block;    // members: name, args, stm
class Stm;      // abstract class
class Jump;     // members: label
class Cjump;    // members: relop, left, right, t, f
class Move;     // members: dst, src
class Seq;      // members: stmlist
class LabelStm; // members: label
class Return;   // members: exp
class Phi;      // members: temp, args
class ExpStm;   // members: exp
class Exp;      // abstract class
class Binop;    // members: op, left, right
class Mem;      // members: mem
class TempExp;  // members: temp
class Eseq;     // members: stm, exp
class Name;     // members: name
class Const;    // members: constVal
class Call;     // members: id, obj, args
class ExtCall;  // members: extfun, args

enum class Type { INT, PTR };

string typeToString(Type type);

// 二元操作符: +, -, *. /, &&, ||, xor
// 无一元操作符: ! => "1 xor", - => "0 -"
// 关系运算符:  ==, !=, <, >, <=, >=

class Visitor {
public:
    virtual void visit(Program* prog) = 0;
    virtual void visit(FuncDecl* func) = 0;
    virtual void visit(Block* block) = 0;
    virtual void visit(Jump* jump) = 0;
    virtual void visit(Cjump* cjump) = 0;
    virtual void visit(Move* move) = 0;
    virtual void visit(Seq* seq) = 0;
    virtual void visit(LabelStm* labelstm) = 0;
    virtual void visit(Return* ret) = 0;
    virtual void visit(Phi* phi) = 0;
    virtual void visit(ExpStm* exp) = 0;
    virtual void visit(Binop* binop) = 0;
    virtual void visit(Mem* mem) = 0;
    virtual void visit(TempExp* tempexp) = 0;
    virtual void visit(Eseq* eseq) = 0;
    virtual void visit(Name* name) = 0;
    virtual void visit(Const* const) = 0;
    virtual void visit(Call* call) = 0;
    virtual void visit(ExtCall* extcall) = 0;
};

enum class Kind {
    PROGRAM,
    FUNCDECL,
    BLOCK,
    JUMP,
    CJUMP,
    MOVE,
    SEQ,
    LABELSTM,
    RETURN,
    PHI,
    EXPSTM,
    BINOP,
    MEM,
    TEMPEXP,
    ESEQ,
    NAME,
    CONST,
    CALL,
    EXTCALL
};

// forward declaration (declaration in treep.cc)
string kindToString(Kind kind);

class Tree {
public:
    ~Tree() { }
    Tree() { }
    virtual Kind getTreeKind() = 0;
    virtual void accept(Visitor& v) = 0;
};

// 程序
// Program: FunctionDeclaration
class Program : public Tree {
public:
    vector<tree::FuncDecl*>* funcdecllist;
    Program(vector<tree::FuncDecl*>* funcdecllist)
        : funcdecllist(funcdecllist) { };
    Kind getTreeKind() { return Kind::PROGRAM; }
    void accept(Visitor& v) { v.visit(this); }
};

// 函数声明
// FunctionDeclaration: Blocks
class FuncDecl : public Tree {
public:
    string name;                  // 函数名 (unique name: classname + methodname)
    vector<tree::Temp*>* args;    // 参数列表: the first argument is the object pointer (this)
    vector<tree::Block*>* blocks; // 首个块的第一个标签是函数的入口标签

    Type return_type;   // 返回类型
    int last_temp_num;  // 最后一个临时变量的编号
    int last_label_num; // 最后一个标签的编号

    FuncDecl(string name, vector<tree::Temp*>* args, vector<tree::Block*>* blocks, Type return_type, int lt, int ll)
        : name(name)
        , args(args)
        , blocks(blocks)
        , return_type(return_type)
        , last_temp_num(lt)
        , last_label_num(ll) { };
    Kind getTreeKind() { return Kind::FUNCDECL; }
    void accept(Visitor& v) { v.visit(this); }
};

// 代码块: 语句列表
// Block: Statements
class Block : public Tree {
public:
    Label* entry_label;     // 入口标签
    vector<tree::Stm*>* sl; // 语句列表
    vector<tree::Label*>* exit_labels;

    Block(Label* entry_label, vector<tree::Label*>* exit_labels, vector<tree::Stm*>* sl)
        : entry_label(entry_label)
        , exit_labels(exit_labels)
        , sl(sl) { };
    Kind getTreeKind() { return Kind::BLOCK; }
    void accept(Visitor& v) { v.visit(this); }
};

// ------------------------------------------------

class Stm : public Tree {
public:
    ~Stm() { }
    Stm() { }
    virtual void accept(Visitor& v) = 0;
};

// 语句->语句列表
// Sequence
class Seq : public Stm {
public:
    vector<tree::Stm*>* sl = nullptr;
    Seq(vector<tree::Stm*>* sl)
        : sl(sl) { };
    Seq() { sl = nullptr; } // if nothing, make sl nullptr
    Kind getTreeKind() { return Kind::SEQ; }
    void accept(Visitor& v) { v.visit(this); }
};


// 语句->表达式 (忽略返回值)
class ExpStm : public Stm {
  public:
      Exp* exp;
      ExpStm(Exp* exp)
          : exp(exp) { };
      Kind getTreeKind() { return Kind::EXPSTM; }
      void accept(Visitor& v) { v.visit(this); }
  };

// 语句->标签
// Label
class LabelStm : public Stm {
public:
    Label* label;
    LabelStm(Label* label)
        : label(label) { };
    Kind getTreeKind() { return Kind::LABELSTM; }
    void accept(Visitor& v) { v.visit(this); }
};

// 语句->跳转
// Jump
class Jump : public Stm {
public:
    Label* label;
    Jump(Label* label)
        : label(label) { };
    Kind getTreeKind() { return Kind::JUMP; }
    void accept(Visitor& v) { v.visit(this); }
};

// 语句->条件跳转
// CJump
class Cjump : public Stm {
public:
    string relop;
    Exp* left;
    Exp* right;
    Label *t, *f;
    Cjump(string relop, Exp* left, Exp* right, Label* t, Label* f)
        : relop(relop)
        , left(left)
        , right(right)
        , t(t)
        , f(f) { };
    Kind getTreeKind() { return Kind::CJUMP; }
    void accept(Visitor& v) { v.visit(this); }
};

// 语句->赋值
// Move: Temp Val
class Move : public Stm {
public:
    Exp* dst;
    Exp* src;
    Move(Exp* dst, Exp* src)
        : dst(dst)
        , src(src) { };
    Kind getTreeKind() { return Kind::MOVE; }
    void accept(Visitor& v) { v.visit(this); }
};

class Phi : public Stm {
public:
    Temp* temp;
    vector<pair<tree::Temp*, tree::Label*>>* args;
    // Label is the label of the block where the value is defined
    Phi(Temp* temp, vector<pair<tree::Temp*, tree::Label*>>* args)
        : temp(temp)
        , args(args) { };
    Kind getTreeKind() { return Kind::PHI; }
    void accept(Visitor& v) { v.visit(this); }
};



// 语句->返回语句
// Return: Temp
class Return : public Stm {
public:
    Exp* exp;
    Return(Exp* exp)
        : exp(exp) { };
    Kind getTreeKind() { return Kind::RETURN; }
    void accept(Visitor& v) { v.visit(this); }
};

// ------------------------------------------------

class Exp : public Tree {
public:
    ~Exp() { }
    Exp() { }
    Type type;
    Exp(Type type)
        : type(type) { };
    virtual void accept(Visitor& v) = 0;
};

// 表达式->二元运算
// BinOp: Val Val
class Binop : public Exp {
public:
    string op;
    Exp* left;
    Exp* right;
    Binop(Type t, string op, Exp* left, Exp* right)
        : Exp(t)
        , op(op)
        , left(left)
        , right(right) { };
    Kind getTreeKind() { return Kind::BINOP; }
    void accept(Visitor& v) { v.visit(this); }
};

// 表达式->访存表达式
// 数组布局 <size,a[0],a[1]...>
// Memory: Temp
class Mem : public Exp {
public:
    Exp* mem;
    Mem(Type t, Exp* mem)
        : Exp(t)
        , mem(mem) { };
    Kind getTreeKind() { return Kind::MEM; }
    void accept(Visitor& v) { v.visit(this); }
};

// 表达式->临时变量表达式
class TempExp : public Exp {
public:
    Temp* temp;
    TempExp(Type t, Temp* temp)
        : Exp(t)
        , temp(temp) { };
    Kind getTreeKind() { return Kind::TEMPEXP; }
    void accept(Visitor& v) { v.visit(this); }
};

// 表达式->逃逸表达式
class Eseq : public Exp {
public:
    Stm* stm;
    Exp* exp;
    Eseq(Type t, Stm* stm, Exp* exp)
        : Exp(t)
        , stm(stm)
        , exp(exp) { };
    Kind getTreeKind() { return Kind::ESEQ; }
    void accept(Visitor& v) { v.visit(this); }
};

// 类方法标签 (示例化时使用)
class Name : public Exp {
public:
    Label* name;
    String_Label* sname;
    Name(Label* name)
        : Exp(Type::PTR)
        , name(name)
        , sname(nullptr) { };
    Name(String_Label* sname)
        : Exp(Type::PTR)
        , name(nullptr)
        , sname(sname) { };
    Kind getTreeKind() { return Kind::NAME; }
    void accept(Visitor& v) { v.visit(this); }
};

// 表达式->常量
class Const : public Exp {
public:
    int constVal;
    Const(int constVal)
        : constVal(constVal)
    {
        type = Type::INT;
    }
    Kind getTreeKind() { return Kind::CONST; }
    void accept(Visitor& v) { v.visit(this); }
};

// 表达式->类方法函数调用
class Call : public Exp {
public:
    string id;
    Exp* obj;
    vector<tree::Exp*>* args;
    Call(tree::Type t, string id, tree::Exp* obj, vector<tree::Exp*>* args)
        : Exp(t)
        , id(id)
        , obj(obj)
        , args(args) { };
    Kind getTreeKind() { return Kind::CALL; }
    void accept(Visitor& v) { v.visit(this); }
};

// 表达式->外部函数调用
class ExtCall : public Exp {
public:
    string extfun;
    vector<tree::Exp*>* args;
    ExtCall(tree::Type t, string extfun, vector<tree::Exp*>* args)
        : Exp(t)
        , extfun(extfun)
        , args(args) { };
    Kind getTreeKind() { return Kind::EXTCALL; }
    void accept(Visitor& v) { v.visit(this); }
};

} // namespace tree

#endif
