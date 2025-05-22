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
bool Coloring::simplify() {
    // 遍历图中所有节点
    for (auto it = graph.begin(); it != graph.end(); it++) {
        int node = it->first;
        int degree = it->second.size();

        // 如果 1.不是机器寄存器; 2.度数 <k; 3.不是移动指令
        if (isMachineReg(node) && degree < k && !isMove(node)) {
            simplifiedNodes.push(node);
            eraseNode(node);            // 从图中删除节点
            return true;
        }
    }

    return false;
}

// 合并 move 指令对应的变量
// 如果有任何节点被合并，返回 true
bool Coloring::coalesce() {

    // 遍历所有的移动指令对
    if (!movePairs.empty()) {
        int u = movePairs.begin()->first;
        int v = movePairs.begin()->second;
        bool canCoalesce = false;

        // 如果存在干扰边，则不能合并
        if (graph[u].find(v) != graph[u].end())
            return false;

        // 若有机器寄存器
        if (isMachineReg(u) || isMachineReg(v)) {
            // 确保 u 始终是机器寄存器
            if (isMachineReg(v)) std::swap(u, v);
            // 检查 v 的邻居是否与 u 冲突 (合并后，v 变成 u)
            bool conflict = false;
            for (auto& t : graph[v]) {
                // 如果邻居也是机器寄存器且与 u 不同，则有冲突
                if (isMachineReg(t) && t != u) {
                    conflict = true;
                    break;
                }
            }
            canCoalesce = !conflict;
        }
        // 若无预着色寄存器
        // 保守合并 (合并后度数 <k 则安全)
        else {
            set<int> mergedNeighbors;
            for (auto& t : graph[u]) if (t != v) mergedNeighbors.insert(t);
            for (auto& t : graph[v]) if (t != u) mergedNeighbors.insert(t);
            canCoalesce = (mergedNeighbors.size() < k);
        }

        // 如果可以合并
        if (canCoalesce) {
            // 合并 v 到 u
            if (coalescedMoves.find(u) == coalescedMoves.end())
                coalescedMoves[u] = set<int>();
            coalescedMoves[u].insert(v);
            if (coalescedMoves.find(v) != coalescedMoves.end()) {
                for (int coalesced : coalescedMoves[v]) {
                    coalescedMoves[u].insert(coalesced);
                }
            }

            // 将 v 的所有邻居连接到 u
            for (auto& t : graph[v])
                if (t != u) addEdge(u, t);
            
            // 从图中移除 v
            eraseNode(v);
            
            // 移除该移动对 (双向)
            movePairs.erase(movePairs.begin());
            for (auto it = movePairs.begin(); it != movePairs.end(); it++) {
                if (it->first == u || it->second == u) {
                    movePairs.erase(it);
                    break;
                }
            }

            // 重命名 movePairs 中的 v
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
    }
    return false;
}

// 冻结不能合并的移动指令
// 如果有任何节点被冻结，返回 true
bool Coloring::freeze() {
    // 遍历所有的移动指令对
    for (auto it = movePairs.begin(); it != movePairs.end(); ) {
        int u = it->first;
        int v = it->second;

        // 至少有一个低度数非预着色节点 -> 这些节点冻结后可以在下一轮被simplify()处理
        if ((!isMachineReg(u) && graph[u].size() < k) || 
            (!isMachineReg(v) && graph[v].size() < k)) {
            it = movePairs.erase(it);
            return true;
        } else {
            it++;
        }
    }

    return false;
}

// 这是一个"软溢出"操作：我们只是暂时将节点从图中移除并加入简化节点栈，表现得像无事发生。
// 真正的溢出发生在后续的 select&coloring 阶段
bool Coloring::spill() {
    // 寻找一个高度数节点进行溢出
    int maxDegree = -1;
    int spillNode = -1;

    // 遍历所有节点，寻找度数最高的非机器寄存器节点
    for (auto& pair : graph) {
        int node = pair.first;
        int degree = pair.second.size();
        
        // 排除机器寄存器（它们有固定的物理寄存器分配）
        if (!isMachineReg(node) && degree >= k && degree > maxDegree) {
            maxDegree = degree;
            spillNode = node;
        }
    }

    // 如果找到合适的溢出节点
    if (spillNode != -1) {
        simplifiedNodes.push(spillNode);    // 假装它被"简化"了
        eraseNode(spillNode);               // 断开其所有边，使其不再影响其他节点的着色
        return true;
    }
    return false;
}



// 尝试为所有节点分配寄存器，并检查着色有效性
bool Coloring::select() {
    // 为机器寄存器预着色
    for (auto& pair : ig->graph) {
        int node = pair.first;
        if (isMachineReg(node)) {
            colors[node] = node; // 机器寄存器颜色固定
            if (coalescedMoves.find(node) != coalescedMoves.end()) { // 给合并的节点上色
                for (int coalesced : coalescedMoves[node]) {
                    colors[coalesced] = node;
                }
            }
        }
    }

    // Debug: 打印 coalescedMoves
    // cout << "Coalesced moves:" << endl;
    // for (auto& pair : coalescedMoves) {
    //     cout << "Node " << pair.first << ": ";
    //     for (int coalesced : pair.second) {
    //         cout << coalesced << " ";
    //     }
    //     cout << endl;
    // }

    // 从简化栈中依次弹出节点，进行着色
    while (!simplifiedNodes.empty()) {
        int node = simplifiedNodes.top();
        simplifiedNodes.pop();
        
        // 找出该节点在原始图中的所有邻居
        set<int> neighbors = ig->graph[node];

        // 如果 node 是合并后的主节点
        if (coalescedMoves.find(node) != coalescedMoves.end())
            for (int coalesced : coalescedMoves[node])      // 对每一个被合并的节点
                for (int neighbor : ig->graph[coalesced])   // 遍历其邻居
                    if (neighbor != node) 
                        neighbors.insert(neighbor);
                       
        // 已被邻居使用的颜色
        set<int> usedColors;
        for (int neighbor : neighbors)
            if (colors.find(neighbor) != colors.end())
                usedColors.insert(colors[neighbor]);
        
        // 找到一个未使用的颜色 (0~k-1)
        int selectedColor = -1;
        for (int color = 0; color < k; color++) {
            if (usedColors.find(color) == usedColors.end()) {
                selectedColor = color;
                break;
            }
        }
        
        // 如果找到可用颜色，进行着色 (否则标记为溢出)
        if (selectedColor != -1)
            colors[node] = selectedColor;
        else
            spilled.insert(node);

        // 颜色传播 + 溢出传播
        for (auto& pair : coalescedMoves) {
            int target = pair.first;
            if (colors.find(target) != colors.end()) {
                // 如果主节点已着色，则将合并的节点着色为主节点的颜色
                for (int coalesced : pair.second) 
                    colors[coalesced] = colors[target];   
            }
            else if (spilled.find(target) != spilled.end()) {
                // 如果主节点溢出，则将合并的节点标记为溢出
                for (int coalesced : pair.second) 
                    spilled.insert(coalesced);
            }
        }
    }

    return checkColoring();
}
