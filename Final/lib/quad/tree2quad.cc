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

// TODO: 外部接口函数
QuadProgram* tree2quad(Program* prog)
{
    Tree2Quad visitor = Tree2Quad();
    visitor.visit(prog);
    return visitor.quadprog;
}

// 程序
// Program: FunctionDeclaration
void Tree2Quad::visit(Program* prog)
{
    // 遍历函数声明列表
    auto qfdl = new vector<QuadFuncDecl*>();
    for (auto fd : *prog->funcdecllist) {
        fd->accept(*this);
        assert(visit_results.size() == 1);
        qfdl->push_back((QuadFuncDecl*)visit_results[0]);
        visit_results.clear();
    }

    // 构造四元式程序
    quadprog = new QuadProgram(prog, qfdl);
}

// 函数声明
// FunctionDeclaration: Blocks
void Tree2Quad::visit(FuncDecl* node)
{
    // 从下一个开始使用
    temp_map = new Temp_map(node->last_temp_num + 1, node->last_label_num + 1);

    // 遍历块列表
    auto qbl = new vector<QuadBlock*>();
    for (auto block : *node->blocks) {
        block->accept(*this);
        assert(visit_results.size() == 1);
        qbl->push_back((QuadBlock*)visit_results[0]);
        visit_results.clear();
    }

    // 构造函数声明
    int ltn = temp_map->next_temp - 1;
    int lln = temp_map->next_label - 1;
    visit_results.push_back(new QuadFuncDecl(node, node->name, node->args, qbl, lln, ltn));
}

// 代码块: 语句列表
// Block: Statements
void Tree2Quad::visit(Block* block)
{
    // 遍历语句列表
    auto qsl = new vector<QuadStm*>();
    for (auto stm : *block->sl) {
        stm->accept(*this);
        for (auto q : visit_results) {
            assert(q != nullptr);
            qsl->push_back((QuadStm*)q);
        }
        visit_results.clear();
    }

    // 构造块
    visit_results.push_back(new QuadBlock(block, qsl, block->entry_label, block->exit_labels));
}

// ------------------------------------------------

// 语句->语句列表
// Sequence
void Tree2Quad::visit(Seq* node)
{
    // 遍历语句列表
    auto qsl = new vector<Quad*>();
    for (auto stm : *node->sl) {
        stm->accept(*this);
        for (auto q : visit_results) {
            assert(q != nullptr);
            qsl->push_back(q);
        }
        visit_results.clear();
    }

    // 重新赋给visit_results
    visit_results = *qsl;
}

// 语句->表达式 (忽略返回值)
void Tree2Quad::visit(ExpStm* node) { node->exp->accept(*this); }

// 语句->标签
// Label
void Tree2Quad::visit(LabelStm* node) { visit_results.push_back(new QuadLabel(node, node->label, new set<Temp*>(), new set<Temp*>())); }

// 语句->跳转
// Jump
void Tree2Quad::visit(Jump* node) { visit_results.push_back(new QuadJump(node, node->label, new set<Temp*>(), new set<Temp*>())); }

// 语句->条件跳转
// CJump
void Tree2Quad::visit(tree::Cjump* node)
{
    node->left->accept(*this);
    assert(visit_term->kind == QuadTermKind::TEMP || visit_term->kind == QuadTermKind::CONST);
    auto left = visit_term;
    visit_term = nullptr;

    node->right->accept(*this);
    assert(visit_term->kind == QuadTermKind::TEMP || visit_term->kind == QuadTermKind::CONST);
    auto right = visit_term;
    visit_term = nullptr;

    // 构造使用集合
    auto use = new set<Temp*>();
    if (left->kind == QuadTermKind::TEMP) {
        auto left_temp = get<TempExp*>(left->term);
        use->insert(left_temp->temp);
    }
    if (right->kind == QuadTermKind::TEMP) {
        auto right_temp = get<TempExp*>(right->term);
        use->insert(right_temp->temp);
    }

    // 构造条件跳转
    visit_results.push_back(new QuadCJump(node, node->relop, left, right, node->t, node->f, new set<Temp*>(), use));
}

// 类方法调用 辅助函数
QuadCall* Tree2Quad::call_helper(Call* node)
{
    auto def = new set<Temp*>();
    auto use = new set<Temp*>();

    node->obj->accept(*this);
    assert(visit_term->kind == QuadTermKind::TEMP);
    auto obj_term = visit_term;
    visit_term = nullptr;
    use->insert(get<TempExp*>(obj_term->term)->temp);

    auto args = new vector<QuadTerm*>();
    for (auto arg : *node->args) {
        arg->accept(*this);
        assert(visit_term->kind == QuadTermKind::TEMP || visit_term->kind == QuadTermKind::CONST);
        auto arg_term = visit_term;
        visit_term = nullptr;

        args->push_back(arg_term);
        if (arg_term->kind == QuadTermKind::TEMP)
            use->insert(get<TempExp*>(arg_term->term)->temp);
    }

    return new QuadCall(node, node->id, obj_term, args, def, use);
}

// 外部函数调用 辅助函数
QuadExtCall* Tree2Quad::extcall_helper(ExtCall* node)
{
    auto def = new set<Temp*>();
    auto use = new set<Temp*>();

    auto args = new vector<QuadTerm*>();
    for (auto arg : *node->args) {
        arg->accept(*this);
        assert(visit_term->kind == QuadTermKind::TEMP || visit_term->kind == QuadTermKind::CONST);
        auto arg_term = visit_term;
        visit_term = nullptr;

        args->push_back(arg_term);
        if (arg_term->kind == QuadTermKind::TEMP)
            use->insert(get<TempExp*>(arg_term->term)->temp);
    }

    return new QuadExtCall(node, node->extfun, args, def, use);
}

// 二元运算 辅助函数
QuadMoveBinop* Tree2Quad::binop_helper(Binop* node, TempExp* dst)
{
    auto def = new set<Temp*>();
    auto use = new set<Temp*>();

    node->left->accept(*this);
    assert(visit_term->kind == QuadTermKind::TEMP || visit_term->kind == QuadTermKind::CONST);
    auto left = visit_term;
    visit_term = nullptr;

    node->right->accept(*this);
    assert(visit_term->kind == QuadTermKind::TEMP || visit_term->kind == QuadTermKind::CONST);
    auto right = visit_term;
    visit_term = nullptr;

    // 构造使用集合
    if (left->kind == QuadTermKind::TEMP)
        use->insert(get<TempExp*>(left->term)->temp);
    if (right->kind == QuadTermKind::TEMP)
        use->insert(get<TempExp*>(right->term)->temp);

    // 构造返回临时变量, 并加入定义集合
    if (dst == nullptr)
        dst = new TempExp(node->type, temp_map->newtemp());

    def->insert(dst->temp);
    return new QuadMoveBinop(node, dst, left, node->op, right, def, use);
}

// 读内存 辅助函数
// Load: temp <- mem(term)
QuadLoad* Tree2Quad::load_helper(Mem* node, TempExp* dst)
{
    auto def = new set<Temp*>();
    auto use = new set<Temp*>();

    node->mem->accept(*this);
    assert(visit_term->kind == QuadTermKind::TEMP);
    auto mem_term = visit_term;
    visit_term = nullptr;

    // 构造使用集合
    if (mem_term->kind == QuadTermKind::TEMP) {
        auto mem_temp = get<TempExp*>(mem_term->term);
        use->insert(mem_temp->temp);
    }

    // 构造返回临时变量, 并加入定义集合
    if (dst == nullptr)
        dst = new TempExp(node->type, temp_map->newtemp());

    def->insert(dst->temp);
    return new QuadLoad(node, dst, mem_term, def, use);
}

// 语句->赋值
// Move: Temp Val
// QuadMove QuadLoad QuadStore QuadMoveBinop QuadMoveCall QuadMoveExtCall
void Tree2Quad::visit(Move* node)
{
    TempExp* dst = nullptr;
    auto def = new set<Temp*>();
    auto use = new set<Temp*>();

    // QuadStore
    // dst: 写内存
    // Store: mem(term) <- term

    // 如果目标是内存
    QuadTerm* memDst = nullptr;
    if (node->dst->getTreeKind() == Kind::MEM) {
        static_cast<Mem*>(node->dst)->mem->accept(*this);
        assert(visit_term->kind == QuadTermKind::TEMP);
        memDst = visit_term;
        visit_term = nullptr;

        // 如果可以直接写入内存
        if (node->src->getTreeKind() == Kind::TEMPEXP || node->src->getTreeKind() == Kind::CONST
            || node->src->getTreeKind() == Kind::NAME) {
        
            node->src->accept(*this);
            assert(visit_term->kind == QuadTermKind::TEMP || visit_term->kind == QuadTermKind::CONST || visit_term->kind == QuadTermKind::MAME);
            auto src = visit_term;
            visit_term = nullptr;

            // 构造使用集合
            use->insert(memDst->get_temp()->temp);
            if (src->kind == QuadTermKind::TEMP)
                use->insert(src->get_temp()->temp);
            
            visit_results.push_back(new QuadStore(node, src, memDst, def, use));
            return;
        }

        dst = new TempExp(node->dst->type, temp_map->newtemp());
        def->insert(dst->temp);
    }

    // 其他情况
    else {
        node->dst->accept(*this);
        assert(visit_term->kind == QuadTermKind::TEMP);
        dst = get<TempExp*>(visit_term->term);
        visit_term = nullptr;
        def->insert(dst->temp);
    }

    // --------------------------------

    // QuadLoad
    // src: 读内存
    // Load: temp <- mem(term)
    if (node->src->getTreeKind() == Kind::MEM) {
        visit_results.push_back(load_helper(static_cast<Mem*>(node->src), dst));
        return;
    }

    // QuadMoveBinop
    // src: 二元运算
    else if (node->src->getTreeKind() == Kind::BINOP) {
        visit_results.push_back(binop_helper(static_cast<Binop*>(node->src), dst));
        return;
    }

    // QuadMoveCall
    // src: 类方法调用
    else if (node->src->getTreeKind() == Kind::CALL) {
        auto call = call_helper(static_cast<Call*>(node->src));
        visit_results.push_back(new QuadMoveCall(node, dst, call, def, call->use));
    }

    // QuadMoveExtCall
    // src: 外部函数调用
    else if (node->src->getTreeKind() == Kind::EXTCALL) {
        auto extcall = extcall_helper(static_cast<ExtCall*>(node->src));
        visit_results.push_back(new QuadMoveExtCall(node, dst, extcall, def, extcall->use));
    }

    // QuadMove
    // src: 其他类型
    else {
        node->src->accept(*this);
        assert(visit_term->kind == QuadTermKind::TEMP || visit_term->kind == QuadTermKind::CONST);
        auto src = visit_term;
        visit_term = nullptr;

        // 构造使用集合
        if (src->kind == QuadTermKind::TEMP)
            use->insert(get<TempExp*>(src->term)->temp);

        // 构造赋值语句
        visit_results.push_back(new QuadMove(node, dst, src, def, use));
    }
}

void Tree2Quad::visit(Phi* node) { }

// 语句->返回语句
// Return: Temp
void Tree2Quad::visit(Return* node)
{
    node->exp->accept(*this);
    assert(visit_term->kind == QuadTermKind::TEMP || visit_term->kind == QuadTermKind::CONST);
    auto exp = visit_term;
    visit_term = nullptr;

    // 构造使用集合
    auto use = new set<Temp*>();
    if (exp->kind == QuadTermKind::TEMP) {
        auto src_temp = get<TempExp*>(exp->term);
        use->insert(src_temp->temp);
    }

    // 构造返回语句
    visit_results.push_back(new QuadReturn(node, exp, new set<Temp*>(), use));
}

// ------------------------------------------------

// 表达式->二元运算
// BinOp: Val Val
void Tree2Quad::visit(Binop* node)
{
    auto qmb = binop_helper(node);
    visit_results.push_back(qmb);
    visit_term = new QuadTerm(qmb->dst);
}

// 表达式->访存表达式
// 数组布局 <size,a[0],a[1]...>
// Memory: Temp
// Load: temp <- mem(term)
void Tree2Quad::visit(Mem* node)
{
    auto load = load_helper(node);
    visit_results.push_back(load);
    visit_term = new QuadTerm(load->dst);
}

// 表达式->临时变量表达式
void Tree2Quad::visit(TempExp* node) { visit_term = new QuadTerm(node); }

// 表达式->逃逸表达式
// the following is useless since IR is canon
void Tree2Quad::visit(Eseq* node) { }

// 类方法标签 (实例化时使用)
// convert the label to a QuadTerm(name)
void Tree2Quad::visit(Name* node) { visit_term = new QuadTerm(node); }

// 表达式->常量
void Tree2Quad::visit(Const* node) { visit_term = new QuadTerm(node); }

// 表达式->类方法调用
void Tree2Quad::visit(Call* node) { visit_results.push_back(call_helper(node)); }

// 表达式->外部函数调用
void Tree2Quad::visit(ExtCall* node) { visit_results.push_back(extcall_helper(node)); }