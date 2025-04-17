#ifndef _TEMP
#define _TEMP

#include <map>
#include <set>
#include <string>
#include <cstdio>

using namespace std;

namespace tree {

// 临时变量 (虚拟寄存器)
class Temp {
  public:
    int num;  //唯一的整数标识符

    Temp(int num) : num(num) {}

    int name() {
      return num;
    }

    string str() {
      return "t" + to_string(num);
    }

    // temp equality is just based on the number, not the object!
    // 判断两个 Temp 是否相同 -> 检查它们的编号是否相同即可
    bool operator==(const Temp& t) const {
      return num == t.num;
    }
};

// 标签 (控制流的跳转目标)
class Label {
  public:
    int num;
    Label(int num) : num(num) {}
    int name() {
      return num;
    }
    string str() {
      return "L" + to_string(num);
    }
    // label equality is just based on the number, not the object!
    // 判断两个 Label 是否相同 -> 检查它们的编号是否相同即可
    bool operator==(const Label& l) const {
      return num == l.num;
    }
};

class String_Label {
  public:
    string name;
    String_Label(string name) : name(name) {}
    string str() {
      return name;
    }
};


// 工厂类
// 用于生成和管理 Temp 和 Label
class Temp_map {
  public:
    map<int, bool> t_map; // Temp map (true 表示该编号已被使用)
    map<int, bool> l_map; // Label map

    int next_temp; // 下一个可用的 Temp 编号
    int next_label; // 下一个可用的 Label 编号

    set<string> s_labels; // 用于存储字符串标签的集合

    Temp_map() {
      next_temp = 100; //start from 100
      next_label = 100;
      t_map.clear();
      l_map.clear();
      s_labels.clear();
    }

    // 创建新的临时变量
    Temp* newtemp() {
      while (t_map[next_temp]) { //just to make sure the temp is unique (in terms of nnumber/name)
        next_temp++;
      }
      t_map[next_temp] = true;
      return new Temp(next_temp++);
    }

    // 创建新的标签
    Label* newlabel() {
      while (l_map[next_label]) { //just to make sure the label is unique (in terms of nnumber/name)
        next_label++;
      }
      l_map[next_label] = true;
      return new Label(next_label++);
    }

    // 创建新的字符串标签
    String_Label* newstringlabel(string name) {
      s_labels.insert(name);
      return new String_Label(name);
    }

    // 检查字符串标签是否已存在
    bool in_stringlabel(string name) {
      return s_labels.find(name) != s_labels.end();
    }

    // 获取所有字符串标签
    set<string> get_all_stringlabels() {
      return s_labels;
    }

    // 获取指定编号的 Temp (如果不存在则返回 nullptr ?)
    Temp* named_temp(int num) {
      if (!t_map[num]) t_map[num] = true;

      // 即使编号已存在，也会返回一个新对象
      // 但是请注意，Temp 的相等性是基于编号，而不是对象本身！
      return new Temp(num);
    }

    // 获取指定编号的 Label
    Label* named_label(int num) {
      if (!l_map[num]) l_map[num] = true;

      // 即使编号已存在，也会返回一个新对象
      // 但是请注意，Label 的相等性是基于编号，而不是对象本身！
      return new Label(num);
    }

    // 某编号是否已分配给临时变量
    bool in_tempmap(int num) {
      return t_map[num];
    }

    // 某编号是否已分配给标签
    bool in_labelmap(int num) {
      return l_map[num];
    }
};

} // namespace tree

#endif
