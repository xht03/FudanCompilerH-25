#define DEBUG
//#undef DEBUG

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include "config.hh"
#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "treep.hh"
#include "temp.hh"
#include "ast2tree.hh"

using namespace std;
//using namespace fdmj;
//using namespace tree;


tree::Program* generate_a_testIR_ast2tree(); //forward decl

// you need to code this function!
tree::Program* ast2tree(fdmj::Program* prog, AST_Semant_Map* semant_map) {
    return generate_a_testIR_ast2tree();
}

//this is only for a test
tree::Program* generate_a_testIR_ast2tree() {
    Temp_map *tm = new Temp_map();
    tree::Label *entry_label = tm->newlabel();
    tree::LabelStm *label_stm = new tree::LabelStm(entry_label);
    tree::Move *move = new tree::Move(new tree::TempExp(tree::Type::INT, tm->newtemp()), new tree::Const(1));

    vector<tree::Stm*> *sl1 = new vector<tree::Stm*>(); sl1->push_back(move);

    vector<tree::Exp*> *args = new vector<tree::Exp*>();
    tree::Eseq *eseq = new tree::Eseq(tree::Type::INT, new tree::Seq(sl1), new tree::Const(19));
    args->push_back(eseq);

    tree::ExtCall *call = new tree::ExtCall(tree::Type::INT, "putchar", args);
    tree::Return *ret = new tree::Return(new tree::Const(100));
    vector<tree::Stm*> *sl = new vector<tree::Stm*>();
    sl->push_back(label_stm);
    sl->push_back(new tree::ExpStm(call));
    sl->push_back(ret);
    tree::Block *block = new tree::Block(entry_label, nullptr, sl);
    vector<tree::Block*> *bl = new vector<tree::Block*>();
    bl->push_back(block);
    tree::FuncDecl *fd = new tree::FuncDecl("_^main^_main", nullptr, bl, tree::Type::INT, 100, 100);
    vector<tree::FuncDecl*> *fdl = new vector<tree::FuncDecl*>();
    fdl->push_back(fd);
    return new tree::Program(fdl);
}
