#ifndef _TrExp_HH
#define _TrExp_HH

#include <iostream>
#include <variant>
#include "temp.hh"
#include "treep.hh"

using namespace std;

// 控制流修补列表
class Patch_list {
public:
    vector<tree::Label*>* patch_list;
    Patch_list() { patch_list = new vector<tree::Label*>(); }
    ~Patch_list() { delete patch_list; }

    // 添加待修补标签 (-1)
    void add_patch(tree::Label* label) { patch_list->push_back(label); }

    // 修补该种类的标签
    void patch(tree::Label* label)
    {
        for (auto l : *patch_list)
            l->num = label->num;
    }

    // 合并两个修补列表
    void add(Patch_list* p2) { patch_list->insert(patch_list->end(), p2->patch_list->begin(), p2->patch_list->end()); }
};

class Tr_ex;
class Tr_nx;
class Tr_cx;

class Tr_Exp {
public:
    virtual Tr_ex* unEx(Temp_map* tm) = 0;
    virtual Tr_cx* unCx(Temp_map* tm) = 0;
    virtual Tr_nx* unNx(Temp_map* tm) = 0;
    enum Tr_Exp_Kind { TR_EX, TR_NX, TR_CX };
    virtual Tr_Exp_Kind getKind() = 0;
};

// 值
class Tr_ex : public Tr_Exp {
public:
    tree::Exp* exp;
    Tr_ex(tree::Exp* e) { exp = e; }
    Tr_cx* unCx(Temp_map* tm) override;
    Tr_nx* unNx(Temp_map* tm) override;
    Tr_ex* unEx(Temp_map* tm) override;
    Tr_Exp_Kind getKind() override { return TR_EX; }
};

// 语句
class Tr_nx : public Tr_Exp {
public:
    tree::Stm* stm;
    Tr_nx(tree::Stm* s) { stm = s; }
    Tr_cx* unCx(Temp_map* tm) override;
    Tr_nx* unNx(Temp_map* tm) override;
    Tr_ex* unEx(Temp_map* tm) override;
    Tr_Exp_Kind getKind() override { return TR_NX; }
};

// 条件表达式
class Tr_cx : public Tr_Exp {
public:
    Patch_list* true_list;
    Patch_list* false_list;
    tree::Stm* stm;
    Tr_cx(Patch_list* t, Patch_list* f, tree::Stm* s)
    {
        true_list = t;
        false_list = f;
        stm = s;
    }
    Tr_ex* unEx(Temp_map* tm) override;
    Tr_nx* unNx(Temp_map* tm) override;
    Tr_cx* unCx(Temp_map* tm) override;
    Tr_Exp_Kind getKind() override { return TR_CX; }
};

#endif