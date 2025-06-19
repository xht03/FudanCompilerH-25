#ifndef _TREE2QUAD_HH
#define _TREE2QUAD_HH

#include "treep.hh"
#include "quad.hh"

using namespace tree;
using namespace quad;

class Tree2Quad : public Visitor {
public:
    QuadProgram* quadprog = nullptr;

    vector<Quad*> visit_results;    // 语句返回值
    QuadTerm* visit_term = nullptr; // 表达式返回值

    // 用来生成新的临时变量和标签 (从last开始)
    Temp_map* temp_map = nullptr;
    
    QuadLoad* load_helper(Mem* node, TempExp* dst = nullptr);
    QuadMoveBinop* binop_helper(Binop* node, TempExp* dst = nullptr);

    QuadCall* call_helper(Call* node);
    QuadExtCall* extcall_helper(ExtCall* node);

    void visit(tree::Program* prog) override;
    void visit(tree::FuncDecl* func) override;
    void visit(tree::Block* block) override;
    void visit(tree::Jump* jump) override;
    void visit(tree::Cjump* cjump) override;
    void visit(tree::Move* move) override;
    void visit(tree::Seq* seq) override;
    void visit(tree::LabelStm* labelstm) override;
    void visit(tree::Return* ret) override;
    void visit(tree::Phi* phi) override;
    void visit(tree::ExpStm* exp) override;
    void visit(tree::Binop* binop) override;
    void visit(tree::Mem* mem) override;
    void visit(tree::TempExp* tempexp) override;
    void visit(tree::Eseq* eseq) override;
    void visit(tree::Name* name) override;
    void visit(tree::Const* const) override;
    void visit(tree::Call* call) override;
    void visit(tree::ExtCall* extcall) override;
};

QuadProgram* tree2quad(tree::Program* prog);

#endif