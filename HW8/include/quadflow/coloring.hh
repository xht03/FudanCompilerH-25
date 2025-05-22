#ifndef __COLORING_HH
#define __COLORING_HH

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include "ig.hh"
#include "tinyxml2.hh"

using namespace std;
using namespace quad;
using namespace tinyxml2;

class Coloring {
public:
    InterferenceGraph *ig;              // 干扰图指针
    int k;                              // 可用颜色数 (机器寄存器数量)
    map<int, set<int>> graph;           // 着色各阶段使用的干扰图邻接表表示
    set<pair<int, int>> movePairs;      // 仍活跃的 movePair 集合 (未合并或冻结)
    stack<int> simplifiedNodes;         // 已简化节点栈 (按简化顺序存储)
    map<int, set<int>> coalescedMoves;  // 已合并的移动指令集合 (键为图中现存节点)
    set<int> spilled;                   // 溢出临时变量集合 (着色结果)
    map<int, int> colors;               // 最终着色结果 (节点->颜色)

    // 通过原始干扰图和移动指令对构造图
    Coloring(InterferenceGraph *ig, int k) : ig(ig), k(k) {
        graph = ig->graph;
        movePairs = ig->movePairs;
        simplifiedNodes = stack<int>();
        coalescedMoves = map<int, set<int>>();
        spilled = set<int>();
    }; // 构造函数

    // 打印功能
    string printColoring();             // 将当前图及移动指令对等信息输出为字符串
    void printColoring(iostream &io);   // 将当前图信息输出到流
    tinyxml2::XMLElement* coloring2xml(XMLDocument *doc, string funcname); // 转换为XML格式

    // 辅助函数
    void addEdge(int u, int v);         // 向图中添加边
    void eraseNode(int node);           // 从图中删除节点 (及其关联边)
    set<int> getNeighbors(int node);    // 获取节点的邻居集合
    bool isMove(int n);                 // 检查节点是否仍存在于移动指令中
    static bool isMachineReg(int n) {return n < 100;}; // 判断节点是否为机器寄存器

    // 着色功能
    bool simplify();    // 简化：移除度数 <k 的节点，返回是否发生简化
    bool coalesce();    // 合并：合并相同颜色节点，返回是否发生合并
    bool freeze();      // 冻结：处理度数 <k 的不可合并节点，返回是否发生冻结
    bool spill();       // 溢出选择：选择度数 >=k 的节点，返回是否发生潜在溢出
    bool select();      // 最终选择：实际着色节点，返回是否完成选择 (实际溢出发生在此处)

    bool checkColoring(); // 验证着色结果有效性
};

Coloring *coloring(InterferenceGraph *ig, int k); // Color the graph with k colors (machine registers)

#endif