#define DEBUG
#undef DEBUG

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <queue>
#include <algorithm>
#include "temp.hh"
#include "quad.hh"
#include "flowinfo.hh"

using namespace std;
using namespace quad;

// 收集函数中所有 block 信息
void ControlFlowInfo::computeAllBlocks() {
    if (func == nullptr || func->quadblocklist == nullptr) {
        return ; // Nothing to do
    }

    // 初始化控制流信息
    allBlocks = set<int>();                 //empty set
    labelToBlock = map<int, QuadBlock*>();  //empty map

    // 遍历函数中所有 block
    for (auto block : *func->quadblocklist) {
        if (block->entry_label) {
            allBlocks.insert(block->entry_label->num);
            labelToBlock[block->entry_label->num] = block;
        }
    }
}

// 计算不可达的 block 集合
void ControlFlowInfo::computeUnreachableBlocks() {
    if (func == nullptr || func->quadblocklist == nullptr) {
        return ;
    }

    unreachableBlocks = set<int>();

    if (successors.empty()) computeSuccessors(); // 确保已计算 successors 关系

    // 广度优先搜索
    set<int> visited = set<int>();      // 可访问的块
    set<int> worklist = set<int>();     // 待访问的块
    worklist.insert(entryBlock);
    while (!worklist.empty())
    {   int block = *worklist.begin(); 
        worklist.erase(worklist.begin());
        visited.insert(block);
        for (auto succ : successors[block]) {
            if (visited.find(succ) == visited.end()) {
                worklist.insert(succ);
            }
        }
    }

    if (allBlocks.empty()) computeAllBlocks(); // 确保已计算 allBlocks 集合

    // 遍历所有块，找出不可达的块
    unreachableBlocks = set<int>();
    for (auto block : allBlocks) {
        if (visited.find(block) == visited.end()) {
            unreachableBlocks.insert(block);
        }
    }
}

// 删除不可达的块，并重新计算控制流信息
void ControlFlowInfo::eliminateUnreachableBlocks() {
    if (func == nullptr || func->quadblocklist == nullptr) {
        return ;
    }

    if (!unreachableBlocks.empty()) computeUnreachableBlocks();     // 确保已计算 unreachableBlocks 集合
    
    // 遍历函数中所有块，删除不可达的块
    int i = 0;
    while (i < func->quadblocklist->size()) {
        int block = func->quadblocklist->at(i)->entry_label->num;
        if (unreachableBlocks.find(block) == unreachableBlocks.end()) {
            i++;
            continue;
        } else {
            // 删除不可达的块
            func->quadblocklist->erase(func->quadblocklist->begin() + i);
        }
    }

    // 重新计算控制流信息
    if (!allBlocks.empty()) computeAllBlocks();
    if (!unreachableBlocks.empty()) computeUnreachableBlocks();
    if (!successors.empty()) computeSuccessors();
    if (!predecessors.empty()) computePredecessors();
    if (!dominators.empty()) computeDominators();
    if (!immediateDominator.empty()) computeImmediateDominator();
    if (!dominanceFrontiers.empty()) computeDominanceFrontiers();
    if (!domTree.empty()) computeDomTree();
}

// 打印控制流图中的映射关系
static void pMap(map<int, set<int>>& map2set, bool rightarrow) {
    for (auto& pair : map2set) {
        cout << "Block: " << pair.first << (rightarrow?" -> ":" <- ");
        for (auto dom : pair.second) {
            cout << dom << " ";
        }
        cout << endl;
    }
}

void ControlFlowInfo::printPredecessors() {
    cout << "Predecessors:" << endl;
    pMap(predecessors, false); //leftarrow
} 

void ControlFlowInfo::printSuccessors() {
    cout << "Successors:" << endl;
    pMap(successors, true); //rightarrow
} 

void ControlFlowInfo::printDominators() {
    pMap(dominators, false); //leftarrow (been dominated)
}

void ControlFlowInfo::printImmediateDominators() {
    for (auto& pair : immediateDominator) {
        cout << "Block: " << pair.first << " -> " << pair.second << endl;
    }
}

void ControlFlowInfo::printDominanceFrontier() {
    cout << "Dominance Frontier:" << endl;
    pMap(dominanceFrontiers, true); //rightarrow
}

void ControlFlowInfo::printDomTree() {
    cout << "Dominator Tree:" << endl;
    pMap(domTree, true); //rightarrow
} 


// 计算每个块的前驱块集合
void ControlFlowInfo::computePredecessors() {
    if (func == nullptr || func->quadblocklist == nullptr) {
        return ;
    }

    predecessors = map<int, set<int>>();

    for (auto block : *func->quadblocklist) {
        if (!block->entry_label) continue;
        
        for (auto predBlock : *func->quadblocklist) {
            if (!predBlock->exit_labels) continue;
            
            for (auto exitLabel : *predBlock->exit_labels) {
                if (exitLabel->num == block->entry_label->num) {
                    predecessors[block->entry_label->num].insert(predBlock->entry_label->num);
                }
            }
        }
    }
}


// 计算每个块的后继块集合
void ControlFlowInfo::computeSuccessors() {
    if (func == nullptr || func->quadblocklist == nullptr) {
        return ;
    }

    successors = map<int, set<int>>(); //empty set

    for (auto block : *func->quadblocklist) {
        if (!block->entry_label) continue;
        
        if (block->exit_labels == nullptr || block->exit_labels->empty()) {
            continue; // No exit labels, no successors
        }
        for (auto exitLabel : *block->exit_labels) {
            if (successors[block->entry_label->num].empty()) {
                successors[block->entry_label->num] = set<int>();
            }
            successors[block->entry_label->num].insert(exitLabel->num);
        }
    }
}

// 计算每个块的支配关系
void ControlFlowInfo::computeDominators() {
    if (successors.empty()) computeSuccessors();    // 确保已计算 successors 关系
    
    dominators = map<int, set<int>>();              // block -> { block 被哪些块支配 }

    /**
     * 初始化
     * 1. 所有块初始被所有其他块支配 (即除了自己)
     * 2. 将入口块重置为仅被自身支配
     */
    // Initially, each block is dominated by all blocks (except itself)
    for (auto block : *func->quadblocklist) {
        if (block->entry_label) {
            set<int> allBlocks;
            for (auto otherBlock : *func->quadblocklist) {
                if (otherBlock->entry_label) {
                    allBlocks.insert(otherBlock->entry_label->num);
                }
            }
            dominators[block->entry_label->num] = allBlocks;
        }
    }
    
    // Entry block is only dominated by itself
    int entryBlock;
    if (func->quadblocklist && func->quadblocklist->size()>0) {
        if (func->quadblocklist->at(0)->entry_label) {
            entryBlock = func->quadblocklist->at(0)->entry_label->num;
            dominators[entryBlock].clear(); // Clear the entry block's dominator set
            dominators[entryBlock].insert(entryBlock); // Add itself
        } else {
            cerr << "Error: Entry block has no entry label!" << endl;
            return ;
        }
    }
    
    /**
     * 迭代计算
     * 
     * while (changed) {
     *     changed = false;
     *     for (每个非入口块 B) {
     *         新支配集 = 所有前驱的支配集的交集
     *         新支配集.加入(B本身)     // 块总是支配自己
     *         if (新支配集 != 原支配集)
     *             更新支配集
     *             changed = true;    // 标记为有变化
     *     }
     * }
     * 
     */
    bool changed = true;
    while (changed) {
        changed = false;
        
        for (auto block : *func->quadblocklist) {
            if (!block->entry_label) continue;
            int blockNum = block->entry_label->num;
            
            // Skip entry block
            if (blockNum == entryBlock) continue;
            
            // get the predecessors of this block
            set<int> preds = predecessors[blockNum];
            if (preds.empty()) continue; // No predecessors, can't compute dominators
            
            // Compute new dominator set: intersection of all predecessors' dominators
            set<int> newDom;
            
            bool first = true;
            for (auto pred : preds) {
                if (first) {
                    // For first predecessor, start with its dominator set
                    newDom.insert(dominators[pred].begin(), dominators[pred].end());
                    first = false;
                } else {
                    // For subsequent predecessors, intersect with their dominator sets
                    set<int> intersection= newDom;
                    for (auto dom : newDom) {
                        if (dominators[pred].find(dom) == dominators[pred].end()) {
                            //if not found in the pred's dominator set, remove it
                            intersection.erase(dom);
                        }
                    }
                    newDom = intersection;
                }
            }

            newDom.insert(blockNum); // Add itself
            
            // Check if dominator set has changed
            if (newDom == dominators[blockNum]) { //set equality
                continue; // No change
            } else{
                dominators[blockNum] = newDom;
                changed = true;
            }
        }
    }
}

// 计算每个块的直接支配块
void ControlFlowInfo::computeImmediateDominator() {
    immediateDominator = map<int, int>();

    if (dominators.empty())  computeDominators();   // 确保已计算支配关系
    
    // 初始化每个块的直接支配者为 -1 (no immediate dominator)
    for (auto& pair : dominators) {
        int block = pair.first;
        immediateDominator[block] = -1;
    }

    // 计算每个块的直接支配者
    for (auto& pair : dominators) {
        int block = pair.first;
        set<int>& doms = pair.second;
        
        // 一个块的直接支配者是最接近的支配者
        int idom = -1;
        for (auto dom : doms) {
            if (dom == block) continue; // Skip self
            
            if (idom == -1) {
                idom = dom; // First dominator
            } else {
                // Check if dom dominates current idom
                if (dom != idom && dominators[dom].find(idom) != dominators[dom].end()) {
                    idom = dom;
                }
            }
        }
        
        if (idom!=-1) {
            immediateDominator[block] = idom;
        } else //no dominator other than self (entry node)
            immediateDominator[block] = -1;
    }
}

// 计算支配树(通过直接支配块)
void ControlFlowInfo::computeDomTree() {
        domTree = map<int, set<int>>();

        if (immediateDominator.empty()) computeImmediateDominator();    // 确保已计算直接支配关系
        
        // 初始化支配树，支配子节点集合为空
        for (auto& pair : immediateDominator) {
            int block = pair.first;
            domTree[block] = set<int>();
        }
        
        // 构建支配树，将每个节点添加到其直接支配者的子节点集合中
        for (auto& pair : immediateDominator) {
            int block = pair.first;
            int idom = pair.second;
            
            if (idom != -1) {
                domTree[idom].insert(block);
            }
        }  
}

// 计算每个块的支配边界
void ControlFlowInfo::computeDominanceFrontiers() {
    dominanceFrontiers = map<int, set<int>>();

    // 确保所有前置条件都已计算 (successors, dominators, domTree)
    if (successors.empty())  computeSuccessors();
    if (predecessors.empty())  computePredecessors();
    if (dominators.empty()) computeDominators();
    if (immediateDominator.empty()) computeImmediateDominator();
    if (domTree.empty()) computeDomTree();

    // 计算每个块的支配边界
    for (auto block : *func->quadblocklist) {
        if (!block->entry_label) continue;

        int blockNum = block->entry_label->num;
        
        //go over the dominator tree to find it's dominatees
        bool done = false;
        stack<int> worklist; worklist.push(blockNum);
        while (!done) {
            int dom = worklist.top(); worklist.pop();
            for (auto succ : successors[dom]) {
                if (dominators[succ].find(blockNum) == dominators[succ].end() ||
                            succ == blockNum) {
                    // If dom does not dominate succ and not itself, add to dominance frontier
                    if (dominanceFrontiers.find(blockNum) == dominanceFrontiers.end()) {
                        dominanceFrontiers[blockNum] = set<int>();
                    }
                    dominanceFrontiers[blockNum].insert(succ);
                }
            }
            for (auto child : domTree[dom]) {
                worklist.push(child);
            }
            if (worklist.empty()) {
                done = true; // No more dominator tree nodes
            } 
        }
    }
}

// 计算所有控制流信息
void ControlFlowInfo::computeEverything() {
    computeAllBlocks();
    computeUnreachableBlocks();
    eliminateUnreachableBlocks();

    computePredecessors(); 
#ifdef DEBUG
    printPredecessors();
#endif

    computeSuccessors(); 
#ifdef DEBUG
    printSuccessors();
#endif

    computeDominators(); 
#ifdef DEBUG
    printDominators();
#endif

    computeImmediateDominator();
#ifdef DEBUG
    printImmediateDominators();
#endif

    computeDomTree();
#ifdef DEBUG
    printDomTree();
#endif

    computeDominanceFrontiers();
#ifdef DEBUG
    printDominanceFrontier();
#endif
}
