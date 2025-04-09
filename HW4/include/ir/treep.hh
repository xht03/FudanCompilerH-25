#ifndef _TREEP
#define _TREEP

#include <vector>
#include <cstdio>
#include <string>
#include "temp.hh"

using namespace std;
using namespace tree;

namespace tree {

//forward declaration
class Tree;         // abstract class
class Program;      // member: funcdecllist
class FuncDecl;     // members: name, args, stm
class Block;        // members: name, args, stm
class Stm;          // abstract class
class Jump;         // members: label
class Cjump;        // members: relop, left, right, t, f
class Move;         // members: dst, src
class Seq;          // members: stmlist
class LabelStm;     // members: label
class Return;       // members: exp
class Phi;          // members: temp, args
class ExpStm;       // members: exp
class Exp;          // abstract class
class Binop;        // members: op, left, right
class Mem;          // members: mem
class TempExp;      // members: temp
class Eseq;         // members: stm, exp
class Name;         // members: name
class Const;        // members: constVal
class Call;         // members: id, obj, args
class ExtCall;      // members: extfun, args

enum class Type {INT, PTR};

string typeToString(Type type); 

// 支持的二元操作符：+, -, *, /, &&, ||, xor
// 不需要一元操作符："!" -> "1 xor", "-" -> "0 -"
// 支持的关系操作符：==, !=, <, >, <=, >=

class Visitor {
  public:
    virtual void visit(Program *prog) = 0;
    virtual void visit(FuncDecl *func) = 0;
    virtual void visit(Block *block) = 0;
    virtual void visit(Jump *jump) = 0;
    virtual void visit(Cjump *cjump) = 0;
    virtual void visit(Move *move) = 0;
    virtual void visit(Seq *seq) = 0;
    virtual void visit(LabelStm *labelstm) = 0;
    virtual void visit(Return *ret) = 0;
    virtual void visit(Phi *phi) = 0;
    virtual void visit(ExpStm *exp) = 0;
    virtual void visit(Binop *binop) = 0;
    virtual void visit(Mem *mem) = 0;
    virtual void visit(TempExp *tempexp) = 0;
    virtual void visit(Eseq *eseq) = 0;
    virtual void visit(Name *name) = 0;
    virtual void visit(Const *const) = 0;
    virtual void visit(Call *call) = 0;
    virtual void visit(ExtCall *extcall) = 0;
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

//forward declaration (declaration in treep.cc)
string kindToString(Kind kind);

class Tree {
  public:
    ~Tree() {}
    Tree() {}
    virtual Kind getTreeKind() = 0;
    virtual void accept(Visitor &v) = 0;
}; 

// 整个程序的根节点
class Program : public Tree {
public:
  std::vector<tree::FuncDecl*> *funcdecllist;   // 函数声明列表

  Program(std::vector<tree::FuncDecl*> *funcdecllist) : funcdecllist(funcdecllist) {}
  Kind getTreeKind() { return Kind::PROGRAM; }
  void accept(Visitor &v) { v.visit(this); }
};

// 基本块
class Block : public Tree {
public:
  Label *entry_label;                         // 入口标签
  std::vector<tree::Label*> *exit_labels;     // 出口标签列表
  std::vector<tree::Stm*> *sl;                // 语句列表

  Block(Label *entry_label, std::vector<tree::Label*> *exit_labels, std::vector<tree::Stm*> *sl) : 
      entry_label(entry_label), exit_labels(exit_labels), sl(sl) {}
  Kind getTreeKind() { return Kind::BLOCK; }
  void accept(Visitor &v) { v.visit(this); }
};

// 函数声明
class FuncDecl : public Tree {
public:
  string name;                        // 函数名 (唯一名称：类名 + 方法名)
  std::vector<tree::Temp*> *args;     // 参数：第一个参数是对象指针 (this)
  std::vector<tree::Block*> *blocks;  // 第一个块是入口块 (block[0]的第一个标签是入口标签)
  Type return_type;
  int last_temp_num;                  // 函数中使用的最后一个临时变量编号
  int last_label_num;                 // 函数中使用的最后一个标签编号

  FuncDecl(string name, std::vector<tree::Temp*> *args, std::vector<tree::Block*> *blocks, Type return_type, int lt, int ll) : 
      name(name), args(args), blocks(blocks), return_type(return_type), last_temp_num(lt), last_label_num(ll) {}
  Kind getTreeKind() { return Kind::FUNCDECL; }
  void accept(Visitor &v)  { v.visit(this); }
};

// 语句的基类
class Stm : public Tree {
public:
  ~Stm() {}
  Stm() {}
  virtual void accept(Visitor &v) = 0;
};

// 语句序列
class Seq : public Stm {
  public:
    std::vector<tree::Stm*> *sl = nullptr;

    Seq(std::vector<tree::Stm*> *sl): sl(sl) {}
    Seq() {sl=nullptr;} //if nothing, make sl nullptr
    Kind getTreeKind() { return Kind::SEQ; }
    void accept(Visitor &v) { v.visit(this); }
};

// 标签定义语句
class LabelStm : public Stm {
  public:
    Label *label;

    LabelStm(Label *label) : label(label) {}
    Kind getTreeKind() { return Kind::LABELSTM; }
    void accept(Visitor &v) { v.visit(this); }
};

// 跳转语句
class Jump : public Stm {
  public:
    Label* label;

    Jump(Label *label) : label(label) {}
    Kind getTreeKind() { return Kind::JUMP; }
    void accept(Visitor &v) { v.visit(this); }
};

// 条件跳转语句
class Cjump : public Stm {
  public:
    string relop;     // 关系运算符
    Exp *left;        // 左表达式
    Exp *right;       // 右表达式
    Label *t, *f;     // 真分支和假分支的标签

    Cjump(string relop, Exp *left, Exp *right, Label *t, Label *f) :
      relop(relop), left(left), right(right), t(t), f(f) {}
    Kind getTreeKind() { return Kind::CJUMP; }
    void accept(Visitor &v) { v.visit(this); }
};

// 复制语句
class Move : public Stm {
  public:
    Exp *dst;
    Exp *src;

    Move(Exp *dst, Exp *src) : dst(dst), src(src) {}
    Kind getTreeKind() { return Kind::MOVE; }
    void accept(Visitor &v) { v.visit(this); }
};

// Phi 语句
// 实现 SSA(静态单赋值) 的控制流合并
class Phi : public Stm {
  public:
    Temp *temp;   // 临时变量
    vector<pair<tree::Temp*, tree::Label*>> *args;  // Label 是该值定义所在基本块的标签

    Phi(Temp *temp, vector<pair<tree::Temp*, tree::Label*>> *args) :
      temp(temp), args(args) {}
    Kind getTreeKind() { return Kind::PHI; }
    void accept(Visitor &v) { v.visit(this); }
};

// 表达式语句 (表达式的结果被忽略，仅保留副作用)
class ExpStm : public Stm {
  public:
    Exp *exp;

    ExpStm(Exp *exp) : exp(exp) {}
    Kind getTreeKind() { return Kind::EXPSTM; }
    void  accept(Visitor &v) { v.visit(this); }
};

// 返回语句
class Return : public Stm {
  public:
    Exp *exp;

    Return(Exp *exp) : exp(exp) {}
    Kind getTreeKind() { return Kind::RETURN; }
    void accept(Visitor &v) { v.visit(this); }
};

// 表达式的基类
class Exp : public Tree {
  public:
    ~Exp() {}
    Exp() {}
    Type type; //each expression has a type (int or pointer)
    Exp(Type type) : type(type) {}
    virtual void accept(Visitor &v) = 0;
};

// 二元表达式
class Binop : public Exp {
  public:
    string op;
    Exp *left;
    Exp *right;

    Binop(Type t, string op, Exp *left, Exp *right) : Exp(t), op(op), left(left), right(right) {}
    Kind getTreeKind() { return Kind::BINOP; }
    void accept(Visitor &v) { v.visit(this); }
}; 

// 访存表达式
class Mem : public Exp {
  public:
    Exp *mem;

    Mem(Type t, Exp *mem) : Exp(t), mem(mem) {} 
    Kind getTreeKind() { return Kind::MEM; }
    void accept(Visitor &v) { v.visit(this); }
}; 

// 临时变量引用表达式
class TempExp : public Exp {
  public:
    Temp *temp;

    TempExp(Type t, Temp *temp) : Exp(t), temp(temp) {}
    Kind getTreeKind() { return Kind::TEMPEXP; }
    void accept(Visitor &v) { v.visit(this); }
};

// Esc 表达式
class Eseq : public Exp {
  public:
    Stm *stm;
    Exp *exp;

    Eseq(Type t, Stm *stm, Exp *exp) : Exp(t), stm(stm), exp(exp) {}
    Kind getTreeKind() { return Kind::ESEQ; }
    void accept(Visitor &v) { v.visit(this); }
};

// 将标签转换为指针 (地址)
class Name : public Exp {
  public:
    Label *name;

    Name(Label *name) : name(name) {type = Type::PTR;}
    Kind getTreeKind() { return Kind::NAME; }
    void accept(Visitor &v) { v.visit(this); }
};

// 整数常量
class Const : public Exp {
  public:
    int constVal; 
    Const(int constVal) : constVal(constVal) {type = Type::INT;}
    Kind getTreeKind() { return Kind::CONST; }
    void accept(Visitor &v) { v.visit(this); }
};

// 调用表达式
class Call : public Exp {
  public:
    string id;                        // 方法名
    Exp *obj;                         // 调用对象
    std::vector<tree::Exp*> *args;    // 参数列表
  
    Call(tree::Type t, string id, tree::Exp *obj, std::vector<tree::Exp*> *args) : 
      Exp(t), id(id), obj(obj), args(args) {}
    Kind getTreeKind() { return Kind::CALL; }
    void accept(Visitor &v) { v.visit(this); }
};

// 外部函数调用 (内置函数或库函数，如 putInt(x), getInt() 等)
class ExtCall : public Exp {
  public:
    string extfun;                      // 外部函数名
    std::vector<tree::Exp*> *args;      // 参数列表

    ExtCall(tree::Type t, string extfun, std::vector<tree::Exp*> *args) : 
      Exp(t), extfun(extfun), args(args) {}
    Kind getTreeKind() { return Kind::EXTCALL; }
    void accept(Visitor &v) { v.visit(this); }
};

} //namespace tree

#endif
