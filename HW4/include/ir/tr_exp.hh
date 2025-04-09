#ifndef _TrExp_HH
#define _TrExp_HH

#include <iostream>
#include <variant>
#include "temp.hh"
#include "treep.hh"

using namespace std;

// 管理跳转标签 (tree::Label*) 的类
// 用于处理条件分支
class Patch_list {
public:
     vector<tree::Label*> *patch_list;  // 需要回填的跳转标签

     Patch_list() {
          patch_list = new vector<tree::Label*>();
     }
     ~Patch_list() {
          delete patch_list;
     }

     // 添加一个标签到 patch_list
     void add_patch(tree::Label* label) { //this label should be named -1
          patch_list->push_back(label);
     }

     // 回填所有的 Label
     // 将 patch_list 中的所有标签的编号设置为 label->num
     void patch(tree::Label* label) {
          for (auto l : *patch_list) {
               l->num = label->num;
          }
     }

     // 合并两个标签列表
     // 将 p2 中的所有标签添加到当前列表
     void add(Patch_list* p2) {
          patch_list->insert(patch_list->end(), p2->patch_list->begin(), p2->patch_list->end());
     }

};

class Tr_ex;
class Tr_nx;
class Tr_cx;

class Tr_Exp {
public:
     virtual Tr_ex* unEx(Temp_map* tm) = 0;       // 表达式 (计算并产生一个值)
     virtual Tr_cx* unCx(Temp_map* tm) = 0;       // 条件 (影响控制流)
     virtual Tr_nx* unNx(Temp_map* tm) = 0;       // 语句 (执行动作但不产生值)
};

class Tr_ex : public Tr_Exp {
public:
     tree::Exp* exp;

     Tr_ex(tree::Exp* e) {
          exp = e;
     }

     Tr_cx* unCx(Temp_map* tm) override;     // 将 Tr_ex 转换为 Tr_cx
     Tr_nx* unNx(Temp_map* tm) override;     // 将 Tr_ex 转换为 Tr_nx
     Tr_ex* unEx(Temp_map* tm) override;     // 将 Tr_ex 转换为 Tr_ex
};
     
class Tr_nx : public Tr_Exp {
public:
     tree::Stm* stm;

     Tr_nx(tree::Stm* s) {
          stm = s;
     }

     Tr_cx* unCx(Temp_map* tm) override;
     Tr_nx* unNx(Temp_map* tm) override;
     Tr_ex* unEx(Temp_map* tm) override;
};
     
class Tr_cx : public Tr_Exp {
public:
     Patch_list* true_list;   // 条件为真时跳转的标签列表
     Patch_list* false_list;  // 条件为假时跳转的标签列表
     tree::Stm* stm;          // 条件语句

     Tr_cx(Patch_list* t, Patch_list* f, tree::Stm* s) {
          true_list = t;
          false_list = f;
          stm = s;
     }

     Tr_ex* unEx(Temp_map* tm) override;
     Tr_nx* unNx(Temp_map* tm) override;
     Tr_cx* unCx(Temp_map* tm) override;
};
     
#endif