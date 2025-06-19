#ifndef __COLOR_HH
#define __COLOR_HH
#include <iostream>
#include <string>
#include <set>
#include <map>
#include "config.hh"
#include "tinyxml2.hh"

using namespace std;
using namespace tinyxml2;

// 以下做法不好：将需要压入栈的被调用者保存寄存器(callee-saved registers)数量固定写死
// 这样写代码会更简单，但不是个好主意。后续可以改进。

#define BYTES_FOR_CALLEE_REGS 36 // 为被调用者保存的寄存器（r4-r10、sp、lr，共9个寄存器 × 4字节 = 36字节）在栈帧中预留空间

class Color {
public:
    string funcname;
    map<int, int> color_map;        // temp(临时变量编号) -> color(寄存器编号)
    set<int> spills;                // 被溢出的临时变量集合
    map<int, int> spill_offset;     // 溢出的临时变量 -> 在栈帧中的偏移量

    Color(string funcname): funcname(funcname) {
        color_map = map<int, int>();
        spills = set<int>();
    }

    Color(string funcname, map<int, int> color_map, set<int> spills): 
        funcname(funcname), color_map(color_map), spills(spills) {}
    
    void add_color(int temp, int color) {color_map[temp] = color;}  // 添加一个临时变量的颜色映射
    void add_spill(int temp) {spills.insert(temp);}                 // 添加一个溢出的临时变量

    // 获取临时变量的颜色
    int color_of(int temp) {
        if (color_map.find(temp) == color_map.end())
            return -1; // 溢出的临时变量没有颜色
        return color_map[temp];
    }

    // 是否溢出
    bool is_spill(int temp) {
        if (spill_offset.size()!=spills.size())  compute_spill_offsets();
        return spills.find(temp) != spills.end();
    }
    
    // 获取在栈帧中的偏移量
    int get_spill_offset(int temp) {
        if (spill_offset.size()!=spills.size())  compute_spill_offsets();
        if (spill_offset.find(temp) != spill_offset.end()) {
            return spill_offset[temp];
        }
        return -1; // not found
    }

    // 计算所有溢出变量的偏移量
    void compute_spill_offsets() {
        int offset = BYTES_FOR_CALLEE_REGS; //+Compiler_Config::get("int_length");
        for (auto it = spills.begin(); it != spills.end(); ++it) {
            spill_offset[*it] = offset;
            offset += Compiler_Config::get("int_length");
        }   
    }
    void print();
};


class ColorMap {
public:
    map<string, Color*> color_map;  // 函数名 -> Color 对象

    ColorMap() {color_map = map<string, Color*>();}

    // 添加一个函数的颜色映射表
    void addColor(string fn, Color* c) {
        if (color_map.find(fn) != color_map.end()) {
            cerr << "Error: Function " << fn << " already exists in the color map." << endl;
            return;
        }
        color_map[fn] = c;
    }

    // 获取一个函数的颜色映射表
    Color* getColor(string fn) {
        if (color_map.find(fn) == color_map.end()) {
            cerr << "Error: Function " << fn << " not found in the color map." << endl;
            return nullptr;
        }
        return color_map[fn];
    }

    void print();
};

ColorMap* xml2colormap(string fn);

#endif 

    