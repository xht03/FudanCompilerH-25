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

//return true if any node is removed
bool Coloring::simplify() {
    return false;
}

//return true if changed anything, false otherwise
bool Coloring::coalesce() {
    return false;
}

//freeze the moves that are not coalesced
//return true if changed anything, false otherwise
bool Coloring::freeze() {
    return false;
}

//This is a soft spill: we just remove the node from the graph and add it to the simplified nodes
//as if nothing happened. The actual spill happens when select&coloring
bool Coloring::spill() {
    return false;
}

//now try to select the registers for the nodes
//finally check the validity of the coloring
bool Coloring::select() {
    return checkColoring();
}
