#define DEBUG
//#undef DEBUG

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <queue>
#include <algorithm>
#include <utility>
#include "treep.hh"
#include "quad.hh"
#include "flowinfo.hh"
#include "quadssa.hh"
#include "temp.hh"

using namespace std;
using namespace quad;

static void deleteUnreachableBlocks(ControlFlowInfo* domInfo);
static void placePhi(ControlFlowInfo* domInfo);
static void renameVariables(ControlFlowInfo* domInfo);
static void cleanupUnusedPhi(QuadFuncDecl* func);

// 删除不可达的块
static void deleteUnreachableBlocks(ControlFlowInfo* domInfo) {
    domInfo->eliminateUnreachableBlocks();
}

// 放置 phi 函数
static void placePhi(ControlFlowInfo* domInfo) {
    // 计算每个块的支配边界
    domInfo->computeDominanceFrontiers();

    // 收集所有变量及其定义位置，分析活跃性
    DataFlowInfo dataFlowInfo(domInfo->func);
    dataFlowInfo.findAllVars();        
    dataFlowInfo.computeLiveness();

    // 记录一个块都对哪些变量有 phi 函数
    // 块编号 -> 变量编号集合
    map<int, set<int>> already_phi;
    for (auto block : domInfo->allBlocks)
        already_phi[block] = set<int>();

    // 对于每一个变量
    for (auto var: dataFlowInfo.allVars) {
        auto W = dataFlowInfo.defs->at(var);    // W 初始化为：变量的定义位置
        while(!W.empty()) {
            quad::QuadBlock* block = W.begin()->first;
            quad::QuadStm* stm = W.begin()->second;
            W.erase(W.begin());

            // 对于每一个支配边界
            for (int F : domInfo->dominanceFrontiers[block->entry_label->num]) {
                /**
                 * 如果在该支配边界上：
                 * 1. 该变量的 phi 函数未定义
                 * 2. 该变量在 F 入口标签的 live_out 里 (即：控制流离开 F 之后还会使用该变量)
                 */
                auto F_block = domInfo->labelToBlock[F];
                const auto& F_liveout = dataFlowInfo.liveout->at(*F_block->quadlist->begin());
                if (already_phi[F].find(var) == already_phi[F].end() && F_liveout.find(var) != F_liveout.end()) {
                    auto tempExp = new TempExp(dataFlowInfo.varType[var], new Temp(var));
                    auto phi = new QuadPhi(nullptr, tempExp, new vector<pair<Temp*, Label*>>(), new set<Temp*>(), new set<Temp*>());
                    domInfo->labelToBlock[F]->quadlist->insert(domInfo->labelToBlock[F]->quadlist->begin() + 1, phi);
                }
            }
        }
    }
}

// 重命名变量
static void renameVariables(ControlFlowInfo* domInfo) {
    
}

// 清理未使用的 phi 函数
static void cleanupUnusedPhi(QuadFuncDecl* func) {
    return; // Placeholder for the actual implementation
}

QuadProgram *quad2ssa(QuadProgram* program) {
    // SSA 版本的程序
    QuadProgram* ssaProgram = new QuadProgram(static_cast<tree::Program*>(program->node), new vector<QuadFuncDecl*>());
    
    // 遍历每个函数声明
    for (auto func : *program->quadFuncDeclList) {
        // 计算控制流信息
        ControlFlowInfo* domInfo = new ControlFlowInfo(func);
        domInfo->computeEverything();
        
        // 删除不可达块
        deleteUnreachableBlocks(domInfo);
        
        // 放置 phi 函数
        placePhi(domInfo);
        
        // 重命名变量
        renameVariables(domInfo);
        
        // 清理未使用的 phi 函数
        cleanupUnusedPhi(func);
        
        // 添加到 SSA 程序
        ssaProgram->quadFuncDeclList->push_back(func);
    }

    return ssaProgram;
}
