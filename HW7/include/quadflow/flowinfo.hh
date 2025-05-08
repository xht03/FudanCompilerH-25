#ifndef QUADFLOW_FLOWINFO_HH
#define QUADFLOW_FLOWINFO_HH

#include <map>
#include <set>
#include <string>
#include <vector>
#include "quad.hh"

/**
 * 支配关系 (Dominance Relation)
 * 
 * 节点 A 支配 B <-> 从起始节点到 B 的所有路径都必须经过 A
 * 
 * A 直接支配 B <-> A 支配 B，且不存在其他节点 C 使得 A 支配 C 且 C 支配 B (即A是“离B最近的”支配者)
 * 
 * 支配边界 (Dominance Frontier)
 * 
 * A 的支配边界 = { B | A 支配 B 的前驱节点，且 A 不支配 B } = 控制流首次“逃逸”A支配范围的节点集合
 */
class ControlFlowInfo {
    public:
        quad::QuadFuncDecl* func;                   // 指向正在分析的函数的指针
        map<int, quad::QuadBlock*> labelToBlock;    // 标签编号 -> block
        int entryBlock;                             // 函数入口块的标签编号

        set<int> allBlocks;         // 所有 block 的集合
        set<int> unreachableBlocks; // 不可达 block 的集合

        // int 是标签编号，表示 QuadFuncDecl 中的一个 block
        map<int, set<int>> predecessors;        // block -> { block 的前驱块 } 
        map<int, set<int>> successors;          // block -> { block 的后继块 } 
        map<int, set<int>> dominators;          // block -> { block 被哪些块支配 }
        map<int, int> immediateDominator;       // block -> { block 的直接支配块 }
        map<int, set<int>> dominanceFrontiers;  // block -> { block 的支配边界 }
        map<int, set<int>> domTree;             // 支配树 (a node to its children blocks)

        ControlFlowInfo(quad::QuadFuncDecl *func): func(func) {
            entryBlock = -1;
            if (func->quadblocklist != nullptr && !func->quadblocklist->empty()) {
                entryBlock = func->quadblocklist->at(0)->entry_label->num;
            }
            if (entryBlock == -1) {
               cerr << "Error: Entry block not found in function" << endl;
            }
            for (auto block : *func->quadblocklist) {
                if (block->entry_label) {
                    labelToBlock[block->entry_label->num] = block;
                }
            }
        }

        void computeAllBlocks();            // 计算 allBlocks 集合
        void computeUnreachableBlocks();    // 计算 unreachableBlocks 集合
        void eliminateUnreachableBlocks();  // 从函数中消除不可达代码块，并清空上述所有集合和映射，因为所有内容都可能需要重新计算！

        // 打印控制流信息
        void printPredecessors();
        void printSuccessors();
        void printDominators();
        void printImmediateDominators();
        void printDominanceFrontier();
        void printDomTree();

        // 计算控制流信息
        void computePredecessors();
        void computeSuccessors();
        void computeDominators();
        void computeImmediateDominator();
        void computeDominanceFrontiers();
        void computeDomTree();

        void computeEverything();
};


/**
 * ​变量在边上活跃 (Live on an edge)
 * 
 * 如果存在一条从该边出发的有向路径，满足：
 * 1. 路径最终会到达该变量的某个使用点(use)​。
 * 2. 且这条路径不会经过该变量的任何定义点(def)​​（即没有重新赋值）。
 * 
 * 变量在某个节点的入口处活跃 (Live in a node) <-> 它在任意一条进入该节点的边上是活跃的
 * 
 * 变量在某个节点的出口处活跃 (Live out a node) <-> 它在任意一条离开该节点的边上是活跃的
 * 
 * 用处：
 * 1. 若变量在某个范围内不再活跃，可以释放其寄存器供其他变量使用
 * 2. 如果变量被定义但后续不再活跃，其定义可能是无用的（可删除）
 */
class DataFlowInfo {
    public:
        quad::QuadFuncDecl* func;   // 指向正在分析的函数的指针
        set<int> allVars;           // { 函数中所有变量(编号) } 
        map<int, Type> varType;  // 变量 -> 变量类型
        map<int, set<pair<quad::QuadBlock*, quad::QuadStm*>>> *defs;    // 变量 -> 被定义的位置 (block, statement)
        map<int, set<pair<quad::QuadBlock*, quad::QuadStm*>>> *uses;    // 变量 -> 被使用的位置 (block, statement)
        map<quad::QuadStm*, set<int>> *liveout;                         // 每个语句的 live_out 变量集合
        map<quad::QuadStm*, set<int>> *livein;                          // 每个语句的 live_in 变量集合
        DataFlowInfo(quad::QuadFuncDecl *func) : func(func) {
            allVars.clear();
            defs = new map<int, set<pair<quad::QuadBlock*, quad::QuadStm*>>>();
            uses = new map<int, set<pair<quad::QuadBlock*, quad::QuadStm*>>>();
            liveout = new map<quad::QuadStm*, set<int>>();
            livein = new map<quad::QuadStm*, set<int>>();
        }

        void findAllVars();         // 收集函数中所有被定义和使用的变量
        void computeLiveness();     // 计算每个语句的 live_in 和 live_out 集合
        string printLiveness();
};

// 对整个程序进行数据流分析
// 为每个函数生成一个 DataFlowInfo 对象
set<DataFlowInfo*>* dataFLowProg(quad::QuadProgram* prog);

#endif