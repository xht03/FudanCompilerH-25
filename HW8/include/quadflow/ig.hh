#ifndef __INTERFERENCEGRAPH_HH
#define __INTERFERENCEGRAPH_HH

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include "quad.hh"
#include "temp.hh"

using namespace std;
using namespace quad;

class InterferenceGraph {
public:
    map<int, set<int>> graph;           // 邻接表表示的干扰图
    set<pair<int, int>> movePairs;      // Set of move pairs in the original graph

    // 根据原始图和 move pairs 来构建图
    InterferenceGraph(map<int, set<int>> graph, set<pair<int, int>> movePairs) : graph(graph), movePairs(movePairs) { 
    }; // Constructor

    string printGraph();                // 将当前图及其他信息打印为字符串
    void printGraph(string filename);   // 打印到文件
    void printGraph(ofstream &io);      // 打印到文件流

    // 辅助函数
    static bool isMachineReg(int n) {return n < 100;}; //检查节点是否为机器寄存器
};

InterferenceGraph *buildIg(QuadFuncDecl* funcdecl); // 构建干扰图 (after the function is prepared)

#endif