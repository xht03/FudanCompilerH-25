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
        if (dataFlowInfo.defs->find(var) == dataFlowInfo.defs->end()) 
            continue; // 如果该变量没有定义，跳过

        auto W = dataFlowInfo.defs->at(var);    // W 初始化为：变量的定义位置 (可能不止一个)

        // 对于每一个定义位置
        while(!W.empty()) {
            quad::QuadBlock* block = W.begin()->first;
            quad::QuadStm* stm = W.begin()->second;
            W.erase(W.begin());

            // 对于每一个支配边界 F
            for (int F : domInfo->dominanceFrontiers[block->entry_label->num]) {
                /**
                 * 如果在该支配边界 F 上：
                 * 
                 * 1. 该变量的 phi 函数未定义
                 * 2. 该变量在 F 入口标签的 live_out 里 (即：控制流离开 F 之后还会使用该变量)
                 * 
                 * 那么在 F 上放置一个 phi 函数
                 */
                auto F_block = domInfo->labelToBlock[F];
                const auto& F_liveout = dataFlowInfo.liveout->at(*F_block->quadlist->begin());
                if (already_phi[F].find(var) == already_phi[F].end() && F_liveout.find(var) != F_liveout.end()) {
                    auto newTemp = new Temp(var);
                    auto tempExp = new TempExp(dataFlowInfo.varType[var], newTemp);
                    auto phi = new QuadPhi(nullptr, tempExp, new vector<pair<Temp*, Label*>>(), new set<Temp*> {newTemp}, new set<Temp*>());
                    domInfo->labelToBlock[F]->quadlist->insert(domInfo->labelToBlock[F]->quadlist->begin() + 1, phi); 
                    already_phi[F].insert(var); // 记录该变量在 F 上已经有 phi 函数
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
    map<int, stack<Temp*>> stacks;  // 变量编号 -> 所有历史变量名
    map<int, int> counter;          // 变量编号 -> 最新版本（版本计数器）

    // 初始化栈和计数器
    for (auto var : dataFlowInfo.allVars) {
        counter[var] = 0;
        stacks[var] = stack<Temp*>();
    }

    // 递归函数：重命名变量
    std::function<void(int)> rename = [&](int blockLabel) {
        // 获取当前块
        QuadBlock* block = domInfo->labelToBlock[blockLabel];
        if (!block || !block->quadlist) return;

        map<int, int> records;  // 记录一个变量在当前块中被定义的次数

        // 处理当前块中的每个语句
        for (auto& stm : *block->quadlist) {
            // 如果 stm 不是 Phi 函数
            if (stm->kind != QuadKind::PHI) {
                // 对 stm 中的每一个 use 的变量，进行重命名
                for (auto& temp : *stm->cloneTemps(stm->use)) {
                    if (stacks.find(temp->num) != stacks.end() && stacks[temp->num].size() > 0) {
                        stm->renameUse(temp, stacks[temp->num].top());
                    }
                }
            }

            // 对于 stm 中的每一个变量的 def
            for (auto& temp : *stm->cloneTemps(stm->def)) {
                // 更新版本栈和计数器
                stacks[temp->num].push(new Temp(VersionedTemp::versionedTempNum(temp->num, counter[temp->num])));
                counter[temp->num]++;
                stm->renameDef(temp, stacks[temp->num].top());

                // 更新定义次数
                if (records.find(temp->num) == records.end()) {
                    records[temp->num] = 1;
                } else {
                    records[temp->num]++;
                }
            }
        }

        // 更新后继块中的 Phi 函数
        for(auto succ_num : domInfo->successors[blockLabel]) {
            QuadBlock* succ_block = domInfo->labelToBlock[succ_num];
            // 对于后继块中的每一个语句
            for (auto& stm : *succ_block->quadlist) {
                // 如果是 Phi 函数
                if (stm->kind == QuadKind::PHI) {
                    auto phi = static_cast<QuadPhi*>(stm);
                    auto origin_num = VersionedTemp::origTempNum(phi->temp->temp->num);
                    // 更新参数（在 PlacePhi 时 Phi 函数的参数留空了）
                    if (stacks[origin_num].size() > 0) {
                        phi->args->push_back(make_pair(stacks[origin_num].top(), domInfo->labelToBlock[blockLabel]->entry_label));
                        phi->use->insert(stacks[origin_num].top());
                    } else {
                        phi->args->push_back(make_pair(new Temp(origin_num), domInfo->labelToBlock[blockLabel]->entry_label));
                        phi->use->insert(new Temp(origin_num));
                    }
                    
                }
            }
        }

        // 递归处理子块
        for (auto child : domInfo->domTree[blockLabel]) {
            rename(child);
        }

        // 递归返回后，在当前块定义过的变量，弹出版本栈顶元素
        for (auto temp: records) {
            int num = temp.first;
            int count = temp.second;
            for (int i = 0; i < count; i++) {
                stacks[num].pop();
            }
        }
    };

    // 从入口块开始重命名
    rename(domInfo->entryBlock);
}

// 清理未使用的 phi 函数
static void cleanupUnusedPhi(QuadFuncDecl* func) { }

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
