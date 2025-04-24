#define DEBUG
#undef DEBUG

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "treep.hh"
#include "quad.hh"
#include "tree2quad.hh"

using namespace std;
using namespace tree;
using namespace quad;


/*
We use an instruction selection method (pattern matching) to convert the IR tree to Quad.
The Quad is a simplified tree node/substructure, each Quad is a tree pattern:
Move:  temp <- temp
Load:  temp <- mem(temp)
Store: mem(temp) <- temp
MoveBinop: temp <- temp op temp
Call:  ExpStm(call) //ignore the result
ExtCall: ExpStm(extcall) //ignore the result
MoveCall: temp <- call
MoveExtCall: temp <- extcall
Label: label
Jump: jump label
CJump: cjump relop temp, temp, label, label
Phi:  temp <- list of {temp, label} //same as the Phi in the tree
*/

QuadProgram* tree2quad(Program* prog) {
#ifdef DEBUG
    cout << "in Tree2Quad::Converting IR to Quad" << endl;
#endif
    //You need to write the code. Now it's a fake one
    Temp_map* temp_map = new Temp_map();
    Label *l = temp_map->newlabel();
    QuadReturn* quad_ret = new QuadReturn(new tree::Const(0), new QuadTerm(new Const(0)), new set<Temp*>(), new set<Temp*>());
    QuadBlock* quad_block = new QuadBlock(nullptr, new vector<QuadStm*>(1, quad_ret), l, new vector<Label*>());
    QuadFuncDecl* quad_func_decl = new QuadFuncDecl(nullptr, string("main"), new vector<Temp*>(), new vector<QuadBlock*>(1, quad_block), 100, 100);
    return new QuadProgram(prog, new vector<QuadFuncDecl*>(1, quad_func_decl));
}

//You need to write all the visit functions below. Now they are all "dummies".
void Tree2Quad::visit(Program* prog) {
#ifdef DEBUG
    cout << "Converting to Quad: Program" << endl;
#endif
    visit_result = nullptr;
    output_term = nullptr;
    return; //no need to visit the program
}

void Tree2Quad::visit(FuncDecl* node) {
#ifdef DEBUG
    cout << "Converting to Quad: FunctionDeclaration" << endl;
#endif
    visit_result = nullptr;
    return; //no need to visit the function declaration
}

void Tree2Quad::visit(Block *block) {
#ifdef DEBUG
    cout << "Converting to Quad: Block" << endl;
#endif
    visit_result = nullptr;
    return; //no need to visit the block
}

void Tree2Quad::visit(Jump* node) {
#ifdef DEBUG
    cout << "Converting to Quad: Jump" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    visit_result = nullptr;
    output_term = nullptr;
    return;
}

void Tree2Quad::visit(tree::Cjump* node) {
#ifdef DEBUG
    cout << "Converting to Quad: CJump" << endl;
#endif
//only one tile pattern matches this CJump
    if (node == nullptr || node->right == nullptr || node->left == nullptr) {
        visit_result = nullptr;
        return;
    }
    visit_result = nullptr;
    output_term = nullptr;
    return;
}

void Tree2Quad::visit(Move* node) {
#ifdef DEBUG
    cout << "Converting to Quad: Move" << endl;
#endif
    visit_result = nullptr;
    output_term = nullptr;
    return;
}

void Tree2Quad::visit(Seq* node) {
#ifdef DEBUG
    cout << "Converting to Quad: Sequence" << endl;
#endif
    visit_result = nullptr;
    output_term = nullptr;
    return; 
}

void Tree2Quad::visit(LabelStm* node) {
#ifdef DEBUG
    cout << "Converting to Quad: Label" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    visit_result = nullptr;
    output_term = nullptr;
    return;
}

void Tree2Quad::visit(Return* node) {
#ifdef DEBUG
    cout << "Converting to Quad: Return" << endl;
#endif
    if (node == nullptr || node->exp == nullptr) {
        visit_result = nullptr;
        return;
    }
    visit_result = nullptr;
    output_term = nullptr;
    return;
}

void Tree2Quad::visit(Phi* node) {
#ifdef DEBUG
    cout << "Converting to Quad: Phi" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    visit_result = nullptr;
    output_term = nullptr;
    return;
}

void Tree2Quad::visit(ExpStm* node) {
#ifdef DEBUG
    cout << "Converting to Quad: ExpressionStatement" << endl;
#endif
    if (node == nullptr || node->exp == nullptr) {
        visit_result = nullptr;
        return;
    }
    visit_result = nullptr;
    output_term = nullptr;
    return;
}

void Tree2Quad::visit(Binop* node) {
#ifdef DEBUG
    cout << "Converting to Quad: BinaryOperation" << endl;
#endif
    if (node == nullptr || node->left == nullptr || node->right == nullptr) {
        visit_result = nullptr;
        return;
    }
    visit_result = nullptr; 
    output_term = nullptr;
}

//convert the memory address to a load quad
void Tree2Quad::visit(Mem* node) {
#ifdef DEBUG
    cout << "Converting to Quad: Memory" << endl;
#endif
    if (node == nullptr || node->mem == nullptr) {
        cerr << "Error: memory address is missing!" << endl;
        visit_result = nullptr;
        return;
    }
    visit_result = nullptr;
    output_term = nullptr;
    return;
}

void Tree2Quad::visit(TempExp* node) {
#ifdef DEBUG
    cout << "Converting to Quad: Temp" << endl;
#endif
    if (node == nullptr) {
        cerr << "Error: Temp is null!" << endl;
        visit_result = nullptr;
        return;
    }
    visit_result = nullptr;
    output_term = nullptr;
    return;
}

//the following is useless since IR is canon
void Tree2Quad::visit(Eseq* node) {
#ifdef DEBUG
    cout << "Converting to Quad: ESeq" << endl;
#endif
    visit_result = nullptr;
    output_term = nullptr;
    return;
}

//convert the label to a QuadTerm(name)
void Tree2Quad::visit(Name* node) {
#ifdef DEBUG
    cout << "Converting to Quad: Name" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    visit_result = nullptr;
    output_term = nullptr;
    return;
}

void Tree2Quad::visit(Const* node) {
#ifdef DEBUG
    cout << "Converting to Quad: Const" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    visit_result = nullptr;
    output_term = nullptr;
    return;
}

void Tree2Quad::visit(Call* node) {
#ifdef DEBUG
    cout << "Converting to Quad: Call" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    visit_result = nullptr;
    output_term = nullptr;
}

void Tree2Quad::visit(ExtCall* node) {
#ifdef DEBUG
    cout << "Converting to Quad: ExtCall" << endl;
#endif
    if (node == nullptr) {
        visit_result = nullptr;
        return;
    }
    visit_result = nullptr;
    output_term = nullptr;
    return;
}