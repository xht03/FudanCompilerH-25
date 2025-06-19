#ifndef QUADFLOW_FLOWINFO_HH
#define QUADFLOW_FLOWINFO_HH

#include <map>
#include <set>
#include <string>
#include <vector>
#include "quad.hh"
#include "treep.hh"

// 函数控制流信息
class ControlFlowInfo {
public:
    quad::QuadFuncDecl* func; // 基本块化的四元式程序

    map<int, quad::QuadBlock*> labelToBlock; // 标签->基本块
    int entryBlock;                          // 首块的入口标签

    set<int> allBlocks;         // 基本块集
    set<int> unreachableBlocks; // 不可达基本块集

    // int是基本块标签编号
    map<int, set<int>> predecessors; // 基本块->前驱基本块
    map<int, set<int>> successors;   // 基本块->后继基本块

    map<int, set<int>> dominators;         // 基本块->支配者
    map<int, int> immediateDominator;      // 基本块->直接支配者
    map<int, set<int>> dominanceFrontiers; // 基本块->支配边界
    map<int, set<int>> domTree;            // 支配树 (节点->子基本块)

    ControlFlowInfo(quad::QuadFuncDecl* func)
        : func(func)
    {
        // 记录首块的入口标签
        entryBlock = -1;
        if (func->quadblocklist != nullptr && !func->quadblocklist->empty())
            entryBlock = func->quadblocklist->at(0)->entry_label->num;

        if (entryBlock == -1)
            cerr << "错误: 函数无入口基本块" << endl;

        // 映射 (标签号->基本块)
        for (auto block : *func->quadblocklist)
            if (block->entry_label)
                labelToBlock[block->entry_label->num] = block;
    }

    void computeAllBlocks();           // 计算所有基本块
    void computeUnreachableBlocks();   // 计算不可达基本块
    void eliminateUnreachableBlocks(); // 从函数中移除不可达基本块 并清空所有集合和映射

    void printPredecessors();        // 打印前驱信息
    void printSuccessors();          // 打印后继信息
    void printDominators();          // 打印支配者信息
    void printImmediateDominators(); // 打印直接支配者信息
    void printDominanceFrontier();   // 打印支配边界信息
    void printDomTree();             // 打印支配树信息

    void computePredecessors();       // 计算前驱
    void computeSuccessors();         // 计算后继
    void computeDominators();         // 计算支配者
    void computeImmediateDominator(); // 计算直接支配者
    void computeDominanceFrontiers(); // 计算支配边界
    void computeDomTree();            // 计算支配树

    void computeEverything(); // 计算所有信息
};

class DataFlowInfo {
public:
    quad::QuadFuncDecl* func; // 基本块化的四元式程序

    set<int> allVars;          // 变量集
    map<int, tree::Type> varTypeMap; // 变量类型

    map<int, set<pair<quad::QuadBlock*, quad::QuadStm*>>>* defs; // 变量定义: 变量->{<基本块, 语句>}
    map<int, set<pair<quad::QuadBlock*, quad::QuadStm*>>>* uses; // 变量使用: 变量->{<基本块, 语句>}
    map<quad::QuadStm*, set<int>>* liveout;                      // 语句出口变量集
    map<quad::QuadStm*, set<int>>* livein;                       // 语句入口变量集

    DataFlowInfo(quad::QuadFuncDecl* func)
        : func(func)
    {
        allVars.clear();
        defs = new map<int, set<pair<quad::QuadBlock*, quad::QuadStm*>>>();
        uses = new map<int, set<pair<quad::QuadBlock*, quad::QuadStm*>>>();
        liveout = new map<quad::QuadStm*, set<int>>();
        livein = new map<quad::QuadStm*, set<int>>();
    }

    void findAllVars();
    void computeLiveness();
    string printLiveness();
};

set<DataFlowInfo*>* dataFLowProg(quad::QuadProgram* prog);

#endif