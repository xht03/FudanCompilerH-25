// #define DEBUG
// #undef DEBUG

// #include <iostream>
// #include <string>
// #include <vector>
// #include <map>
// #include <set>
// #include "temp.hh"
// #include "ig.hh"
// #include "coloring.hh"

// bool isAnEdge(map<int, set<int>>& graph, int src, int dst) {
//     if (graph.find(src) == graph.end()) return false; //src not in the graph
//     if (graph[src].find(dst) == graph[src].end()) return false; //dst not in the graph
//     return true; //edge exists
// }

// // 移除度数小于 k 非预着色节点
// // 如果有任何节点被移除，返回 true
// bool Coloring::simplify() {
//     // 遍历图中所有节点
//     for (auto it = graph.begin(); it != graph.end(); it++) {
//         int node = it->first;
//         int degree = it->second.size();

//         // 如果 1.不是机器寄存器; 2.度数 <k; 3.不是移动指令
//         if (degree < k && !isMove(node)) {
//             simplifiedNodes.push(node);
//             eraseNode(node);    // 从图中删除节点
//             return true;
//         }
//     }
//     return false;
// }

// // 合并 move 指令对应的变量
// // 如果有任何节点被合并，返回 true
// bool Coloring::coalesce() {

//     // 遍历所有的移动指令对
//     for (auto it = movePairs.begin(); it != movePairs.end(); it++) {
//         int u = it->first;
//         int v = it->second;

//         // 如果存在干扰边，则不能合并
//         if (graph[u].find(v) != graph[u].end())
//             return false;

//         // Briggs 策略
//         set<int> union_neighbors;
//         union_neighbors.insert(graph[u].begin(), graph[u].end());
//         union_neighbors.insert(graph[v].begin(), graph[v].end());

//         int high_num = 0;
//         for (auto it = union_neighbors.begin(); it != union_neighbors.end(); it++)
//             if (getNeighbors(*it).size() >= k)
//                 high_num++;
//         if (high_num >= k)
//             continue;

//         if (isMachineReg(v)) swap(u, v); // 交换 u 和 v，使得 u 为机器寄存器

//         // 移除该移动对 (双向)
//         movePairs.erase(it);
//         for (auto it = movePairs.begin(); it != movePairs.end(); it++) {
//             if (it->first == v && it->second == u) {
//                 movePairs.erase(it);
//                 break;
//             }
//         }

//         eraseNode(v); // 从图中删除 v

//         for (auto it = graph[v].begin(); it != graph[v].end(); it++) 
//             addEdge(u, *it); // 将 v 的邻居添加到 u 的邻居中

//         coalescedMoves[u].insert(v); // 将 v 合并到 u 中
//         if (coalescedMoves.find(v) != coalescedMoves.end()) {
//             for (auto it = coalescedMoves[v].begin(); it != coalescedMoves[v].end(); it++) {
//                 coalescedMoves[u].insert(*it); // 将 v 的合并节点添加到 u 中
//             }
//         }

//         // 重命名移动对中的 v
//         std::set<std::pair<int, int>> newMovePairs;
//         for (const auto& p : movePairs) {
//             auto newPair = p;
//             if (newPair.first == v)
//                 newPair.first = u;
//             if (newPair.second == v)
//                 newPair.second = u;
//             newMovePairs.insert(newPair);
//         }
//         movePairs = std::move(newMovePairs);
//         return true;
//     }

//     return false;
// }

// // 冻结不能合并的移动指令
// // 如果有任何节点被冻结，返回 true
// bool Coloring::freeze() {
//     // 遍历所有的移动指令对
//     if (!movePairs.empty()) {
//         int u = movePairs.begin()->first;
//         int v = movePairs.begin()->second;

//         movePairs.erase(movePairs.begin());
//         for (auto it = movePairs.begin(); it != movePairs.end(); it++) {
//             if (it->first == v && it->second == u) {
//                 movePairs.erase(it);
//                 break;
//             }
//         }
//         return true;
//     }

//     return false;
// }

// // 这是一个"软溢出"操作：我们只是暂时将节点从图中移除并加入简化节点栈，表现得像无事发生。
// // 真正的溢出发生在后续的 select&coloring 阶段
// bool Coloring::spill() {
//     // 寻找一个高度数节点进行溢出
//     int maxDegree = -1;
//     int spillNode = -1;

//     // 遍历所有节点，寻找度数最高的非机器寄存器节点
//     for (auto& pair : graph) {
//         int node = pair.first;
//         int degree = pair.second.size();

//         // 排除机器寄存器（它们有固定的物理寄存器分配）
//         if (isMachineReg(node))
//             continue;
        
//         if (degree > maxDegree) {
//             maxDegree = degree;
//             spillNode = node;
//         }
//     }

//     // 如果找到合适的溢出节点
//     if (spillNode != -1) {
//         simplifiedNodes.push(spillNode);    // 假装它被"简化"了
//         eraseNode(spillNode);               // 断开其所有边，使其不再影响其他节点的着色
//         return true;
//     }
//     return false;
// }

// // 尝试为所有节点分配寄存器，并检查着色有效性
// bool Coloring::select() {
//     // 为机器寄存器预着色
//     for (auto& pair : ig->graph) {
//         int node = pair.first;
//         if (isMachineReg(node)) {
//             colors[node] = node; // 机器寄存器颜色固定 
//             if (coalescedMoves.find(node) != coalescedMoves.end()) {
//                 for (int merged : coalescedMoves[node]) {
//                     colors[merged] = node; // 合并的节点也着色为主节点的颜色
//                 }
//             }
//         }
//     }

//     // 从简化栈中依次弹出节点，进行着色
//     while (!simplifiedNodes.empty()) {
//         int node = simplifiedNodes.top();
//         simplifiedNodes.pop();

//         // 跳过机器寄存器
//         if (isMachineReg(node))
//             continue;
        
//         // 找出该节点在原始图中的所有邻居
//         set<int> neighbors = getNeighbors(node);
//         for (auto it = coalescedMoves[node].begin(); it != coalescedMoves[node].end(); it++) {
//             auto coalesced_neighbors = getNeighbors(*it);
//             neighbors.insert(coalesced_neighbors.begin(), coalesced_neighbors.end());
//         }

        
//         // 已被邻居使用的颜色
//         set<int> usedColors;
//         for (int neighbor : neighbors)
//             if (colors.find(neighbor) != colors.end())
//                 usedColors.insert(colors[neighbor]);
        
        
//         // 找到一个未使用的颜色 (0~k-1)
//         for (int color = 0; color < k; color++) {
//             if (usedColors.find(color) == usedColors.end()) {
//                 colors[node] = color; // 选择该颜色
//                 for (int coalesced : coalescedMoves[node])
//                     colors[coalesced] = color; // 合并的节点也着色为该颜色
//                 break;
//             }
//         }
        
//         // 如果没有可用颜色，则标记为溢出
//         if (colors.find(node) == colors.end()) {
//             spilled.insert(node);
//             for (int coalesced : coalescedMoves[node])
//                 spilled.insert(coalesced); // 合并的节点也标记为溢出
//         }
//     }
    
//     return checkColoring();
// }

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
    if (graph.find(src) == graph.end()) return false; //src not in the graph
    if (graph[src].find(dst) == graph[src].end()) return false; //dst not in the graph
    return true; //edge exists
}

// 移除度数小于 k 非预着色节点
// 如果有任何节点被移除，返回 true
bool Coloring::simplify()
{
    for (auto it = graph.begin(); it != graph.end(); it++) {
        int node = it->first;
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

        // Briggs策略
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

// 冻结不能合并的移动指令
// 如果有任何节点被冻结，返回 true
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

// 这是一个"软溢出"操作：我们只是暂时将节点从图中移除并加入简化节点栈，表现得像无事发生。
// 真正的溢出发生在后续的 select&coloring 阶段
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

// 尝试为所有节点分配寄存器，并检查着色有效性
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