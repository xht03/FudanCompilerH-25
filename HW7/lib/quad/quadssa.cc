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
#include <functional>
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
    // 收集函数中所有被定义和使用的变量
    DataFlowInfo dataFlowInfo(domInfo->func);
    dataFlowInfo.findAllVars();

    // 为每个变量维护一个栈，用于跟踪当前版本
    map<int, stack<int>> stacks;    // 变量编号 -> 其所有版本号
    map<int, int> counter;          // 变量编号 -> 最新版本（版本计数器）

    // 初始化栈和计数器
    for (auto var : dataFlowInfo.allVars) {
        counter[var] = 0;
        stacks[var] = stack<int>();
        stacks[var].push(0);  // 初始版本为 0
    }

    // 递归函数：重命名变量
    std::function<void(int)> rename = [&](int blockLabel) {
        // 获取当前块
        QuadBlock* block = domInfo->labelToBlock[blockLabel];
        if (!block || !block->quadlist) return;

        vector<int> defined;  // 此块中定义的变量

        // 处理当前块中的每个语句
        for (auto& stm : *block->quadlist) {
            // 如果 stm 不是 Phi 函数
            if (stm->kind != QuadKind::PHI) {
                std::set<Temp*>* use_clone = stm->cloneTemps(stm->use);
                for (auto& temp : *use_clone) {
                    int ver = stacks[temp->num].top();  // 获取当前版本
                    int new_num = VersionedTemp::versionedTempNum(temp->num, ver);
                    Temp* new_temp = new Temp(new_num);
                    stm->use->erase(temp);          // 删除旧的 temp
                    stm->use->insert(new_temp);     // 插入新的 temp
                }
            }

            // 对于 stm 中的每一个变量的 def
            std::set<Temp*>* def_clone = stm->cloneTemps(stm->def);
            for (auto& temp : *def_clone) {
                // 更新版本栈和计数器
                counter[temp->num]++;  
                stacks[temp->num].push(counter[temp->num]);
                // 生成新的 temp
                int ver = stacks[temp->num].top();
                int new_num = VersionedTemp::versionedTempNum(temp->num, ver);
                Temp* new_temp = new Temp(new_num);
                stm->def->erase(temp);          // 删除旧的 temp
                stm->def->insert(new_temp);     // 插入新的 temp

                // 更新定义列表
                defined.push_back(temp->num);
            }
        }

        // 更新后继块中的 Phi 函数
        for(auto succ_num : domInfo->successors[blockLabel]) {
            QuadBlock* succ_block = domInfo->labelToBlock[succ_num];
            // 对于每一个 Phi 函数
            for (auto& stm : *succ_block->quadlist) {
                if (auto phi = dynamic_cast<QuadPhi*>(stm)) {
                    // 对于 Phi 每一个参数
                    for (auto& arg : *phi->args) {
                        // 如果参数的标签是当前块的标签
                        if (arg.second == block->entry_label) {
                            // 更新参数
                            int ver = stacks[arg.first->num].top();
                            int new_num = VersionedTemp::versionedTempNum(arg.first->num, ver);
                            Temp* new_temp = new Temp(new_num);
                            arg.first = new_temp;
                        }
                    }
                }
            }
        }

        // 递归处理子块
        for (auto child : domInfo->domTree[blockLabel]) {
            rename(child);
        }

        // 递归返回后，在当前块定义过的变量，弹出版本栈顶元素
        for (auto& temp : defined) {
            stacks[temp].pop();
        }
    };

    // 从入口块开始重命名
    rename(domInfo->entryBlock);
}

// 清理未使用的 phi 函数
static void cleanupUnusedPhi(QuadFuncDecl* func) {
    
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
