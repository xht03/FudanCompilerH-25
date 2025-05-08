#ifndef _TEMP
#define _TEMP

#include <iostream>
#include <set>
#include <map>
#include <string>
#include <cstdio>

using namespace std;

namespace tree {

class Temp {
  public:
    int num;
    Temp(int num) : num(num) {}
    int name() {
      return num;
    }
    string str() {
      return "t" + to_string(num);
    }

    // Temp 的相等性仅基于数值，而非对象本身！
    // 也就是说：要判断两个 Temp 是否相同，只需检查它们的数值是否相同！
    bool operator==(const Temp& t) const {
      return num == t.num;
    }
};

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
    // Label 的相等性仅基于数值，而非对象本身！
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

// Temp_map 类用于管理临时变量和标签的映射关系
class Temp_map {
  public:
    map<int, bool> t_map;   // 临时变量映射表
    map<int, bool> l_map;   // 标签映射表
    int next_temp;          // 下一个可用的临时变量
    int next_label;         // 下一个可用的标签编号
    set<string> s_labels;   // 字符串标签集合

    Temp_map() {
      next_temp = 100;      // start from 100
      next_label = 100;
      t_map.clear();
      l_map.clear();
      s_labels.clear();
    }

    Temp* newtemp() {
      while (t_map[next_temp]) { // 为了确保 Temp 的唯一性（在编号/名称上）
        next_temp++;
      }
      t_map[next_temp] = true;
      return new Temp(next_temp++);
    }

    Label* newlabel() {
      while (l_map[next_label]) { // 为了确保 Label 的唯一性（在编号/名称上）
        next_label++;
      }
      l_map[next_label] = true;
      return new Label(next_label++);
    }

    String_Label* newstringlabel(string name) {
      s_labels.insert(name);
      return new String_Label(name);
    }

    bool in_stringlabel(string name) {
      return s_labels.find(name) != s_labels.end();
    }

    set<string> get_all_stringlabels() {
      return s_labels;
    }

    // 根据 编号 获取对应的 Temp，若不存在则返回 nullptr  
    Temp* named_temp(int num) {  
      if (!t_map[num]) t_map[num] = true;  
      return new Temp(num); //即使已存在相同编号的临时变量，仍返回一个新对象
      //注意：临时变量的相等性是基于编号num而非对象本身！ 
    }  

    Label* named_label(int num) {
      if (!l_map[num]) l_map[num] = true;
      return new Label(num);  //即使已存在相同编号的标签，仍返回一个新对象
    }

    bool in_tempmap(int num) {
      return t_map[num];
    }

    bool in_labelmap(int num) {
      return l_map[num];
    }
};

} // namespace tree

#endif
