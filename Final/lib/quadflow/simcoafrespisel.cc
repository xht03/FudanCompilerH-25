#define DEBUG
#undef DEBUG

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "temp.hh"
#include "ig.hh"
#include "coloring.hh"

bool isAnEdge(map<int, set<int>>& graph, int src, int dst) {
    if (graph.find(src) == graph.end()) return false;           //src not in the graph
    if (graph[src].find(dst) == graph[src].end()) return false; //dst not in the graph
    return true; //edge exists
}

// 移除度数小于 k 非预着色节点
// 如果有任何节点被移除，返回 true
bool Coloring::simplify()
{
    for (auto it = graph.begin(); it != graph.end(); it++) {
        int node = it->first;
        // 如果结点度数 < k 且不是 movePairs 中的结点
        if (getNeighbors(node).size() < k && !isMove(node)) {
            simplifiedNodes.push(node);
            eraseNode(node);
            return true;
        }
    }
    return false;
}

// 合并 move 指令对应的变量
// 如果有任何节点被合并，返回 true
bool Coloring::coalesce()
{
    for (auto it = movePairs.begin(); it != movePairs.end(); it++) {
        int u = it->first;
        int v = it->second;

        if (isAnEdge(graph, v, u))
            return false;

        /**
         * Briggs 策略：
         * 
         * 1. 考虑两个节点 u 和 v 所有邻居的并集
         * 2. 如果邻居并集中，度数 >= k 的节点数量小于 k，则合并
         * 
         * 为什么有效：
         * 
         * 1. 合并后，新节点的“高难度邻居”（度数>=k的节点）很少（少于k个）
         * 2. 因此在后续着色时，总能找到可用的颜色（因为最多有 k-1 个高难度邻居可能限制颜色选择）。
         */
        auto neighborsU = getNeighbors(u);
        auto neighborsV = getNeighbors(v);
        set<int> union_neighbors;
        union_neighbors.insert(neighborsU.begin(), neighborsU.end());
        union_neighbors.insert(neighborsV.begin(), neighborsV.end());

        int high_cnt = 0;
        for (auto it = union_neighbors.begin(); it != union_neighbors.end(); it++)
            if (getNeighbors(*it).size() >= k)
                high_cnt++;
        if (high_cnt >= k)
            continue;

        if (isMachineReg(v)) swap(u, v);

        // 移除该移动对
        // 不仅是 (u,v)，还包括 (v,u)
        movePairs.erase(it);
        for (auto it = movePairs.begin(); it != movePairs.end(); it++) {
            if (it->first == v && it->second == u) {
                movePairs.erase(it);
                break;
            }
        }

        // 移除src结点
        eraseNode(v);

        // 将 v 的邻居添加到 u 的邻居中
        for (auto it = neighborsV.begin(); it != neighborsV.end(); it++)
            addEdge(u, *it);

        // 将 v 的合并集 添加到 u
        coalescedMoves[u].insert(v);
        if (coalescedMoves.find(v) != coalescedMoves.end()) {
            auto Vcoalesced = coalescedMoves[v];
            coalescedMoves[u].insert(Vcoalesced.begin(), Vcoalesced.end());
        }

        // 重命名移动对中的 v
        std::set<std::pair<int, int>> newMovePairs;
        for (const auto& p : movePairs) {
            auto newPair = p;
            if (newPair.first == v)
                newPair.first = u;
            if (newPair.second == v)
                newPair.second = u;
            newMovePairs.insert(newPair);
        }
        movePairs = std::move(newMovePairs);
        return true;
    }

    return false;
}

/**
 * 冻结不能合并的移动指令
 * @note 用于处理：无法通过 simplify 或 coalesce 解决的僵局
 * @return 如果有任何节点被冻结，返回 true
 */
bool Coloring::freeze()
{
    if (!movePairs.empty()) {
        // 移除该移动对
        int u = movePairs.begin()->first;
        int v = movePairs.begin()->second;
        movePairs.erase(movePairs.begin());
        for (auto it = movePairs.begin(); it != movePairs.end(); it++) {
            if (it->first == v && it->second == u) {
                movePairs.erase(it);
                break;
            }
        }

        return true;
    }
    return false;
}

/**
 * 选择一个度数最大的结点，将其从图中移除并压入简化栈
 * @note 这是一个"软溢出"操作，表现得像无事发生。真正的溢出发生在后续的 select & coloring 阶段
 */
bool Coloring::spill()
{
    int max_degree = 0;
    int max_node = -1;
    for (auto it = graph.begin(); it != graph.end(); it++) {
        int node = it->first;
        if (isMachineReg(node))
            continue;
        int degree = getNeighbors(node).size();
        if (degree > max_degree) {
            max_degree = degree;
            max_node = node;
        }
    }

    if (max_node != -1) {
        simplifiedNodes.push(max_node);
        eraseNode(max_node);
        return true;
    }

    return false;
}

/**
 * 尝试为所有节点分配寄存器，并检查着色有效性
 * @note select 是基于原始图进行的着色，而不是简化后的图
 */
bool Coloring::select()
{
    graph = ig->graph;
    movePairs = ig->movePairs;

    // 为机器寄存器预着色
    for (auto& pair : ig->graph) {
        int node = pair.first;
        if (isMachineReg(node)) {
            colors[node] = node; // 机器寄存器颜色固定 
            if (coalescedMoves.find(node) != coalescedMoves.end()) {
                for (int merged : coalescedMoves[node]) {
                    colors[merged] = node; // 合并的节点也着色为主节点的颜色
                }
            }
        }
    }

    while (!simplifiedNodes.empty()) {
        int node = simplifiedNodes.top();
        simplifiedNodes.pop();

        // 跳过机器寄存器
        if (isMachineReg(node))
            continue;

        // 找出该节点在原始图中的所有邻居 (包括合并的邻居)
        set<int> neighbors = getNeighbors(node);
        for (auto it = coalescedMoves[node].begin(); it != coalescedMoves[node].end(); it++) {
            auto coalesced_neighbors = getNeighbors(*it);
            neighbors.insert(coalesced_neighbors.begin(), coalesced_neighbors.end());
        }

        // 已被邻居使用的颜色
        set<int> usedColors;
        for (int neighbor : neighbors)
            if (colors.find(neighbor) != colors.end())
                usedColors.insert(colors[neighbor]);

        // 找到一个未使用的颜色 (0~k-1)
        for (int color = 0; color < k; color++) {
            if (usedColors.find(color) == usedColors.end()) {
                colors[node] = color; // 选择该颜色
                for (int coalesced : coalescedMoves[node])
                    colors[coalesced] = color; // 合并的节点也着色为该颜色
                break;
            }
        }
        
        // 如果没有可用颜色，则标记为溢出
        if (colors.find(node) == colors.end()) {
            spilled.insert(node);
            for (int coalesced : coalescedMoves[node])
                spilled.insert(coalesced); // 合并的节点也标记为溢出
        }
    }

    return checkColoring();
}