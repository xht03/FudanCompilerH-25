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
    bool removed = false;

    // 遍历图中所有节点
    for (auto it = graph.begin(); it != graph.end(); ) {
        int node = it->first;
        int degree = it->second.size();

        // 如果 1.不是机器寄存器; 2.度数 <k; 3.不是移动指令
        if (!isMachineReg(node) && degree < k && !isMove(node)) {
            simplifiedNodes.push(node);
            eraseNode(node);            // 从图中删除节点
            removed = true;             // 标记已移除节点
            break;
        } else {
            it++;
        }
    }
    return removed;
}

// 合并 move 指令对应的变量
// 如果有任何节点被合并，返回 true
bool Coloring::coalesce() {

    // 遍历所有的移动指令对
    for (auto it = movePairs.begin(); it != movePairs.end(); ) {
        int u = it->first;
        int v = it->second;
        bool canCoalesce = false;
    
        // 如果有一个节点不在图中，跳过
        if (graph.find(u) == graph.end() || graph.find(v) == graph.end()) {
            it = movePairs.erase(it);   // 因为它们已经不在图中了
            continue;
        }

        // 如果存在干扰边，则不能合并
        if (graph[u].find(v) != graph[u].end()) {
            ++it;
            continue;
        }

        // 若有预着色寄存器
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
            // 记录合并信息
            if (coalescedMoves.find(u) == coalescedMoves.end())
                coalescedMoves[u] = set<int>();
            coalescedMoves[u].insert(v);
            
            // 将 v 的所有邻居连接到 u
            for (auto& t : graph[v])
                if (t != u) addEdge(u, t);
            
            // 从图中移除 v
            eraseNode(v);
            
            // 移除此移动指令对
            it = movePairs.erase(it);
            
            return true;
        } else {
            it++;
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
        }
    }
    // 从简化栈中依次弹出节点，进行着色
    while (!simplifiedNodes.empty()) {
        int node = simplifiedNodes.top();
        simplifiedNodes.pop();
        
        // 找出该节点在原始图中的所有邻居
        set<int> neighbors = ig->graph[node];
        
        // 已被邻居使用的颜色
        set<int> usedColors;
        for (int neighbor : neighbors) {
            // 检查邻居是否已被着色
            if (colors.find(neighbor) != colors.end())
                usedColors.insert(colors[neighbor]);

            // 如果邻居是被合并了的节点，检查它的主节点是否已被着色
            for (auto& pair : coalescedMoves) {
                if (pair.second.find(neighbor) != pair.second.end() && 
                    colors.find(pair.first) != colors.end()) {
                    usedColors.insert(colors[pair.first]);
                }
            }
        }
        
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

        // 颜色传播
        for (auto& pair : coalescedMoves) {
            int target = pair.first;
            if (colors.find(target) != colors.end()) {
                // 如果主节点已着色，则将合并的节点着色为主节点的颜色
                for (int merged : pair.second) {
                    colors[merged] = colors[target];
                }
            }
        }
    }
    return checkColoring();
}
