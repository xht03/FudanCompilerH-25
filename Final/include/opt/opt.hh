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

enum class ValueType {
    NO_VALUE = 0,    // 还未确定赋值
    ONE_VALUE = 1,   // 有单值
    MANY_VALUES = 2, // 有多值
};

class RtValue {
    ValueType vt;
    int value; // 变量的单值

public:
    RtValue() // 默认为无值
        : vt(ValueType::NO_VALUE)
        , value(0) { };
    RtValue(ValueType vt)
        : vt(vt)
        , value(0) { };
    RtValue(int value)
        : vt(ValueType::ONE_VALUE)
        , value(value) { };
    ValueType getType() { return vt; }
    int getIntValue() { return value; }

    bool operator==(const RtValue& other) const { return vt == other.vt && value == other.value; }
    bool operator!=(const RtValue& other) const { return !(*this == other); }
};

class Opt {
public:
    QuadFuncDecl* func;
    map<int, bool> block_executable;  // 块可达映射
    map<int, RtValue> temp_value;     // 变量值映射
    map<int, QuadBlock*> label2block; // 块标签映射

    Opt(quad::QuadFuncDecl* func)
        : func(func) { };

    // true: 值发送变化
    bool setExecutable(int block_num, bool executable)
    {
        if (block_executable[block_num] != executable) {
            block_executable[block_num] = executable;
            return true;
        } else
            return false;
    }

    // true: 值发送变化
    bool setRtValue(int temp_num, RtValue rtvalue)
    {
        if (temp_value.find(temp_num) == temp_value.end()) temp_value[temp_num] = RtValue();
        if (temp_value[temp_num] != rtvalue) {
            temp_value[temp_num] = rtvalue;
            return true;
        } else
            return false;
    }

    // 获取变量值
    RtValue getRtValue(int temp_num)
    {
        if (temp_value.find(temp_num) != temp_value.end())
            return temp_value[temp_num];
        else
            return temp_value[temp_num] = RtValue();
    }

    void calculateBT();
    void modifyFunc();
    QuadFuncDecl* optFunc();

    void printRtValue()
    {
        for (auto& it : temp_value) {
            cout << "temp: " << it.first;
            cout << ", type: ";
            if (it.second.getType() == ValueType::NO_VALUE)
                cout << "NO_VALUE";
            else if (it.second.getType() == ValueType::ONE_VALUE)
                cout << "ONE_VALUE";
            else if (it.second.getType() == ValueType::MANY_VALUES)
                cout << "MANY_VALUES";
            if (it.second.getType() == ValueType::ONE_VALUE)
                cout << ", value: " << it.second.getIntValue() << endl;
            else
                cout << endl;
        }
    }

    void printBlockExecutable()
    {
        for (auto& it : block_executable)
            cout << "block: " << it.first << ", executable: " << it.second << endl;
    }
};

QuadProgram* optProg(QuadProgram* prog);

#endif