#ifndef __OPT_HH__
#define __OPT_HH__

#include <iostream>
#include <string>
#include <variant>
#include "quad.hh"
#include "temp.hh"

using namespace std;
using namespace tree;
using namespace quad;

// 变量取值状态
enum  class ValueType {
    NO_VALUE = 0,       // 没有值
    ONE_VALUE = 1,      // 只有一个值
    MANY_VALUES = 2     // 有多个可能的值（如变量被多次赋不同值）
};

class RtValue {
    ValueType vt; 
    int value; // 仅当 vt=ONE_VALUE 时有效
public:
    RtValue() : vt(ValueType::NO_VALUE), value(0) {}                // default value type: no value
    RtValue(ValueType vt) : vt(vt), value(0) {}                     // only for ONE value and MANY values
    RtValue(int value) : vt(ValueType::ONE_VALUE), value(value) {}  //if int, then one value
    ValueType getType() {return vt;}
    int getIntValue() {return value;}
};

class Opt {
public:
    QuadFuncDecl* func;
    map<int, bool> block_executable;    // entry label num -> executable (true: executable, false: not executable)
    map<int, RtValue> temp_value;       // temp num -> RtValue
    map<int, QuadBlock*> label2block;   // entry label num -> QuadBlock*

    Opt(quad::QuadFuncDecl* func) : func(func) {}

    // 查询某个临时变量的取值状态
    RtValue getRtValue(int temp_num) {
        if (temp_value.find(temp_num) != temp_value.end()) {
            return temp_value[temp_num];
        }
        else {
            return temp_value[temp_num] = RtValue(); 
            //not found, return no value, effectively make all temps to have no value to start with
        }
    }

    // 计算哪些基本块可执行、每个临时变量的取值状态
    void calculateBT();
    // 将临时变量替换为常量（若为常量），并尽可能删除指令和基本块。
    void modifyFunc();

    QuadFuncDecl* optFunc();

    // 打印所有临时变量的取值状态（便于调试）
    void printRtValue() {
        for (auto& it : temp_value) {
            cout << "temp: " << it.first ;
            cout << ", type: ";
            if (it.second.getType() == ValueType::NO_VALUE) cout << "NO_VALUE";
            else if (it.second.getType() == ValueType::ONE_VALUE) cout << "ONE_VALUE";
            else if (it.second.getType() == ValueType::MANY_VALUES) cout << "MANY_VALUES";
            if (it.second.getType() == ValueType::ONE_VALUE)
                cout << ", value: " << it.second.getIntValue() << endl;
            else cout << endl;
        }
    }

    // 打印所有基本块的可执行性（便于调试）
    void printBlockExecutable() {
        for (auto& it : block_executable) {
            cout << "block: " << it.first << ", executable: " << it.second << endl;
        }
    }

    void Opt::updateRtValue(int temp_num, int val);

    RtValue Opt::getQuadTermRtValue(QuadTerm* term);
};

QuadProgram* optProg(QuadProgram* prog);

#endif