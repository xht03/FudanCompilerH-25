#define DEBUG
// #undef DEBUG

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

// 函数声明前移
static void deleteUnreachableBlocks(ControlFlowInfo* domInfo);
static void placePhi(ControlFlowInfo* domInfo);
static void renameVariables(ControlFlowInfo* domInfo);
static void cleanupUnusedPhi(QuadFuncDecl* func);

// 删除不可达基本块
static void deleteUnreachableBlocks(ControlFlowInfo* domInfo) { domInfo->eliminateUnreachableBlocks(); }

// 在支配边界插入Phi函数
static void placePhi(ControlFlowInfo* domInfo)
{
    // 构造数据流信息
    DataFlowInfo dataFlowInfo(domInfo->func);
    dataFlowInfo.findAllVars();
    dataFlowInfo.computeLiveness();

    // 记录块中已经定义的Phi函数变量
    map<int, set<int>> A_phi;
    for (int block : domInfo->allBlocks)
        A_phi[block] = set<int>();

    // 遍历所有变量
    for (int a : dataFlowInfo.allVars) {
        // 跳过函数形参 (无定义)
        if (dataFlowInfo.defs->find(a) == dataFlowInfo.defs->end())
            continue;

        auto W = dataFlowInfo.defs->at(a);
        while (!W.empty()) {
            quad::QuadBlock* n_block = W.begin()->first;
            quad::QuadStm* n_stm = W.begin()->second;
            W.erase(W.begin());

            // 遍历当前块的所有支配边界
            for (int Y : domInfo->dominanceFrontiers[n_block->entry_label->num]) {
                // 如果该变量的Phi函数未定义
                // 并且该变量在Y的入口标签的liveout里
                auto Y_block = domInfo->labelToBlock[Y];
                auto Y_entry = Y_block->quadlist->at(0);
                auto& Y_liveout = dataFlowInfo.liveout->at(Y_entry);
                if (A_phi[Y].find(a) == A_phi[Y].end() && Y_liveout.find(a) != Y_liveout.end()) {
                    auto newTemp = new Temp(a);
                    auto tempExp = new TempExp(dataFlowInfo.varTypeMap[a], newTemp);
                    auto phi
                        = new QuadPhi(nullptr, tempExp, new vector<pair<Temp*, Label*>>(), new set<Temp*> { newTemp }, new set<Temp*>());
                    domInfo->labelToBlock[Y]->quadlist->insert(domInfo->labelToBlock[Y]->quadlist->begin() + 1, phi);
                    A_phi[Y].insert(a);
                }
            }
        }
    }
}

static map<int, int> Count;
static map<int, vector<Temp*>> Stack;

// 转换为原始变量号
static int convertOrigin(int num)
{
    if (num >= 10000)
        return num / 100;
    return num;
}

static void Rename(DataFlowInfo& dataFlowInfo, ControlFlowInfo* domInfo, int n)
{
    QuadBlock* block = domInfo->labelToBlock[n];
    map<int, int> StackTimes;

    for (QuadStm* S : *(block->quadlist)) {
        // 如果不是Phi函数, 则替换use
        if (S->kind != QuadKind::PHI) {
            for (Temp* i : *S->cloneTemps(S->use)) {
                if (!Stack[i->num].empty())
                    S->renameUse(i, Stack[i->num].back());
            }
        }

        // 替换def
        for (Temp* i : *S->cloneTemps(S->def)) {
            Count[i->num]++;
            Stack[i->num].push_back(new Temp(Count[i->num]));
            S->renameDef(i, Stack[i->num].back());

            // 记录压栈次数, 方便弹出
            if (StackTimes.find(i->num) == StackTimes.end())
                StackTimes[i->num] = 1;
            else
                StackTimes[i->num]++;
        }
    }

    // 遍历后继, 添加Phi函数的参数
    for (int Y : domInfo->successors[n]) {
        auto Y_block = domInfo->labelToBlock[Y];
        for (QuadStm* S : *(Y_block->quadlist)) {
            if (S->kind == QuadKind::PHI) {
                auto phi = static_cast<QuadPhi*>(S);
                auto num = convertOrigin(phi->temp->temp->num);
                if (!Stack[num].empty()) {
                    phi->args->push_back(make_pair(Stack[num].back(), domInfo->labelToBlock[n]->entry_label));
                    phi->use->insert(Stack[num].back());
                } else {
                    phi->args->push_back(make_pair(new Temp(num), domInfo->labelToBlock[n]->entry_label));
                    phi->use->insert(new Temp(num));
                }
            }
        }
    }

    // 递归处理子块
    for (int X : domInfo->domTree[n])
        Rename(dataFlowInfo, domInfo, X);

    // 回溯 弹出栈
    for (auto record : StackTimes) {
        int i = record.first;
        int times = record.second;
        for (int j = 0; j < times; j++)
            Stack[i].pop_back();
    }
}

// 重命名变量(确保每个变量仅赋值一次)
static void renameVariables(ControlFlowInfo* domInfo)
{
    // 构造数据流信息
    DataFlowInfo dataFlowInfo(domInfo->func);
    dataFlowInfo.findAllVars();

    // 初始化变量的计数和栈
    Count.clear();
    Stack.clear();
    for (int a : dataFlowInfo.allVars) {
        Count[a] = a * 100 - 1;
        Stack[a] = vector<Temp*> {};
    }

    // 递归处理起始块
    Rename(dataFlowInfo, domInfo, domInfo->entryBlock);
}

QuadProgram* quad2ssa(QuadProgram* program)
{
    QuadProgram* ssaProgram = new QuadProgram(static_cast<tree::Program*>(program->node), new vector<QuadFuncDecl*>());
    for (auto func : *program->quadFuncDeclList) {
        // 计算控制流信息
        ControlFlowInfo* domInfo = new ControlFlowInfo(func);
        domInfo->computeEverything();

        // 删除不可达的基本块
        deleteUnreachableBlocks(domInfo);

        // 在支配边界插入Phi函数
        placePhi(domInfo);

        // 重命名变量
        renameVariables(domInfo);

        // 将处理后的函数添加到新程序
        ssaProgram->quadFuncDeclList->push_back(func);
    }

    return ssaProgram;
}