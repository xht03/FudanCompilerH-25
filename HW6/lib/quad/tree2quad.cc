#define DEBUG
#undef DEBUG

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "treep.hh"
#include "quad.hh"
#include "tree2quad.hh"
#include "canon.hh"

using namespace std;
using namespace tree;
using namespace quad;


/*
我们使用指令选择方法 (模式匹配) 将 IR 树转换为 Quad
Quad 是一种简化的树节点/子结构，每个四元式代表一种树模式：
Move:  temp <- temp
Load:  temp <- mem(temp)
Store: mem(temp) <- temp
MoveBinop: temp <- temp op temp
Call:  ExpStm(call)         //忽略结果
ExtCall: ExpStm(extcall)    //忽略结果
MoveCall: temp <- call
MoveExtCall: temp <- extcall
Label: label
Jump: jump label
CJump: cjump relop temp, temp, label, label
Phi:  temp <- list of {temp, label}     // 与树中的 Phi 节点相同
*/


QuadProgram* tree2quad(Program* prog) {
    // visitor 初始化
    Tree2Quad visitor;
    visitor.quad_prog = nullptr;
    visitor.quad_func = nullptr;
    visitor.quad_block = nullptr;
    visitor.visit_result = new vector<QuadStm*>();
    visitor.output_term = nullptr;
    visitor.temp_map = new Temp_map();
    // 将 IR 转换为 Quad
    prog->accept(visitor);
    return visitor.quad_prog;
}


// 程序
void Tree2Quad::visit(Program* prog) {
    // 所有函数声明
    vector<QuadFuncDecl*> *qfdl = new vector<QuadFuncDecl*>();
    if (prog && prog->funcdecllist) {
        for (auto func : *(prog->funcdecllist)) {
            func->accept(*this);
            if (quad_func)
                qfdl->push_back(quad_func);
        }
    }

    QuadProgram *quadProgram = new QuadProgram(prog, qfdl);
    quad_prog = quadProgram;
}


// 函数声明
void Tree2Quad::visit(FuncDecl* node) {
    visit_result->clear();

    // 重置临时变量映射
    if (temp_map) delete temp_map;
    temp_map = new Temp_map();
    temp_map->next_temp = node->last_temp_num + 1;
    temp_map->next_label = node->last_label_num + 1;

    vector<QuadBlock*> *qbl = new vector<QuadBlock*>();
    if (node->blocks) {
        for (auto block : *(node->blocks)) {
            quad_block = nullptr;
            block->accept(*this);
            if (quad_block)
                qbl->push_back(quad_block);
        }
    }

    QuadFuncDecl *qfd = new QuadFuncDecl(node, node->name, node->args, qbl, temp_map->next_label - 1, temp_map->next_temp - 1);
    quad_func = qfd;
}


// 块
void Tree2Quad::visit(Block *block) {
    visit_result->clear();

    if (block == nullptr)
        return;

    vector<QuadStm*> *quadlist = new vector<QuadStm*>();

    if (block->sl) {
        for (auto stm : *(block->sl)) {
            stm->accept(*this);
            if (!visit_result->empty()) {
                quadlist->insert(quadlist->end(), visit_result->begin(), visit_result->end());
            }
        }
    }

    QuadBlock *quadblock = new QuadBlock(block, quadlist, block->entry_label, block->exit_labels);
    quad_block = quadblock;
}


// 跳转
void Tree2Quad::visit(Jump* node) {
    visit_result->clear();

    set<Temp*> *def = new set<Temp*>(); // 跳转不会定义新的临时变量
    set<Temp*> *use = new set<Temp*>();

    QuadJump *jump = new QuadJump(node, node->label, def, use);
    visit_result->push_back(jump);
    output_term = nullptr;
}


// 条件跳转
// only one tile pattern matches this CJump
void Tree2Quad::visit(tree::Cjump* node) {
    visit_result->clear();

    set<Temp*> *def = new set<Temp*>(); // 条件跳转不会定义新的临时变量
    set<Temp*> *use = new set<Temp*>();

    // 左操作数
    node->left->accept(*this);
    QuadTerm *left_term = output_term;
    if (left_term->kind == QuadTermKind::TEMP)
        use->insert(left_term->get_temp()->temp);

    // 处理右操作数
    node->right->accept(*this);
    QuadTerm *right_term = output_term;
    if (right_term->kind == QuadTermKind::TEMP)
        use->insert(right_term->get_temp()->temp);

    QuadCJump *cjump = new QuadCJump(node, node->relop, left_term, right_term, node->t, node->f, def, use);
    visit_result->push_back(cjump);
    output_term = nullptr;
}

// 赋值
// QuadMoveExtCall QuadMoveCall QuadMoveBinop QuadMove QuadLoad QuadStore
void Tree2Quad::visit(Move* node) {
    visit_result->clear();

    set<Temp*> *def = new set<Temp*>();
    set<Temp*> *use = new set<Temp*>();

    /* 若 dst 是临时变量 */
    if (node->dst->getTreeKind() == Kind::TEMPEXP) {
        TempExp* dst_temp = static_cast<TempExp*>(node->dst);
        def->insert(dst_temp->temp);

        /* 若 src 是内存 (QuadLoad) */
        if (node->src->getTreeKind() == Kind::MEM) {
            Mem *memNode = static_cast<Mem*>(node->src);
            memNode->mem->accept(*this);
            // 内存地址
            QuadTerm* addr_term = output_term;
            if (addr_term->kind == QuadTermKind::TEMP)
                use->insert(addr_term->get_temp()->temp);
            // 生成 QuadLoad
            QuadLoad *load = new QuadLoad(node, dst_temp, addr_term, def, use);
            visit_result->push_back(load);
        }

        /* 若 src 是函数调用 (QuadMoveCall) */
        else if (node->src->getTreeKind() == Kind::CALL) {
            Call *callNode = static_cast<Call*>(node->src);
            callNode->accept(*this);
            QuadCall *call = static_cast<QuadCall*>(visit_result->back());
            visit_result->clear();

            def->insert(call->def->begin(), call->def->end());
            use->insert(call->use->begin(), call->use->end());
            QuadMoveCall *move_call = new QuadMoveCall(node, dst_temp, call, def, use);
            visit_result->push_back(move_call);
        }

        /* 若 src 是外部函数调用 (QuadMoveExtCall) */
        else if (node->src->getTreeKind() == Kind::EXTCALL) {
            ExtCall *extcallNode = static_cast<ExtCall*>(node->src);
            extcallNode->accept(*this);
            QuadExtCall *extcall = static_cast<QuadExtCall*>(visit_result->back());
            visit_result->clear();

            def->insert(extcall->def->begin(), extcall->def->end());
            use->insert(extcall->use->begin(), extcall->use->end());
            QuadMoveExtCall *move_extcall = new QuadMoveExtCall(node, dst_temp, extcall, def, use);
            visit_result->push_back(move_extcall);
        }

        /* 若 src 是二元操作 (QuadMoveBinop) */ 
        else if (node->src->getTreeKind() == Kind::BINOP) {
            Binop *binopNode = static_cast<Binop*>(node->src);
            // 左操作数
            binopNode->left->accept(*this);
            QuadTerm *left_term = output_term;
            if (left_term->kind == QuadTermKind::TEMP)
                use->insert(left_term->get_temp()->temp);

            // 右操作数
            binopNode->right->accept(*this);
            QuadTerm *right_term = output_term;
            if (right_term->kind == QuadTermKind::TEMP)
                use->insert(right_term->get_temp()->temp);

            QuadMoveBinop *move_binop = new QuadMoveBinop(node, dst_temp, left_term, binopNode->op, right_term, def, use);
            visit_result->push_back(move_binop);
        }

        // 若 src 是临时变量 (QuadMove)
        else if (node->src->getTreeKind() == Kind::TEMPEXP || 
                 node->src->getTreeKind() == Kind::CONST) {
            TempExp *tempNode = static_cast<TempExp*>(node->src);
            tempNode->accept(*this);
            QuadTerm *src_term = output_term;
            if (src_term->kind == QuadTermKind::TEMP)
                use->insert(src_term->get_temp()->temp);

            QuadMove *move = new QuadMove(node, dst_temp, src_term, def, use);
            visit_result->push_back(move);
        }
        else {
            cerr << "Error: Move statement with unknown source type." << endl;
        }
    }

    // 若 dst 是内存 (QuadStore)
    else if (node->dst->getTreeKind() == Kind::MEM) {
        // 目的地址
        Mem *memNode = static_cast<Mem*>(node->dst);
        memNode->mem->accept(*this);
        QuadTerm *addr_term = output_term;
        if (addr_term->kind == QuadTermKind::TEMP)
            use->insert(addr_term->get_temp()->temp);
        
        // 源操作数
        node->src->accept(*this);
        QuadTerm *src_term = output_term;
        if (src_term->kind == QuadTermKind::TEMP)
            use->insert(src_term->get_temp()->temp);

        QuadStore *store = new QuadStore(node, src_term, addr_term, def, use);
        visit_result->push_back(store);
    }

    else {
        cerr << "Error: Move statement with unknown destination type." << endl;
    }

    output_term = nullptr;
}

// 语句序列
void Tree2Quad::visit(Seq* node) {
    visit_result->clear();

    // 语句列表
    vector<QuadStm*> *stmlist = new vector<QuadStm*>();
    if (node && node->sl) {
        for (auto stm : *(node->sl)) {
            stm->accept(*this);
            if (!visit_result->empty())
                stmlist->insert(stmlist->end(), visit_result->begin(), visit_result->end());
        }
    }

    visit_result = stmlist;
    output_term = nullptr;
}


// 标签语句
void Tree2Quad::visit(LabelStm* node) {
    visit_result->clear();

    set<Temp*> *def = new set<Temp*>();
    set<Temp*> *use = new set<Temp*>();

    QuadLabel* label = new QuadLabel(node, node->label, def, use);
    visit_result->push_back(label);
    output_term = nullptr;
}


// 返回语句
void Tree2Quad::visit(Return* node) {
    visit_result->clear();

    set<Temp*> *def = new set<Temp*>();
    set<Temp*> *use = new set<Temp*>();

    // 返回值
    node->exp->accept(*this);
    QuadTerm* return_term = output_term;
    if (return_term->kind == QuadTermKind::TEMP)
        use->insert(return_term->get_temp()->temp);
    
    QuadReturn* ret = new QuadReturn(node, return_term, def, use);
    visit_result->push_back(ret);
    output_term = nullptr;
}


// Phi
void Tree2Quad::visit(Phi* node) {
    // TODO
    return;
}


// 表达式语句 (只关心副作用，不关心返回值)
void Tree2Quad::visit(ExpStm* node) {
    visit_result->clear();  // 准备存储新的语句
    node->exp->accept(*this);
}


// 二元操作表达式
void Tree2Quad::visit(Binop* node) {
    set<Temp*> *def = new set<Temp*>();
    set<Temp*> *use = new set<Temp*>();
    
    // 左操作数
    node->left->accept(*this);
    QuadTerm* left_term = output_term;
    if (left_term->kind == QuadTermKind::TEMP)
        use->insert(left_term->get_temp()->temp);

    // 右操作数
    node->right->accept(*this);
    QuadTerm* right_term = output_term;
    if (right_term->kind == QuadTermKind::TEMP)
        use->insert(right_term->get_temp()->temp);

    // 计算结果
    TempExp* result_temp = new TempExp(node->type, temp_map->newtemp());
    def->insert(result_temp->temp);

    QuadMoveBinop* move_binop = new QuadMoveBinop(node, result_temp, left_term, node->op, right_term, def, use);
    visit_result->push_back(move_binop);
    output_term = new QuadTerm(result_temp);
}


// 访存
// convert the memory address to a load quad
void Tree2Quad::visit(Mem* node) {
    set<Temp*> *def = new set<Temp*>();
    set<Temp*> *use = new set<Temp*>();
    
    // 内存地址
    node->mem->accept(*this);
    QuadTerm* addr_term = output_term;
    if (addr_term->kind == QuadTermKind::TEMP)
        use->insert(addr_term->get_temp()->temp);

    // 计算结果
    TempExp* result_temp = new TempExp(node->type, temp_map->newtemp());
    def->insert(result_temp->temp);

    QuadLoad* load = new QuadLoad(node, result_temp, addr_term, def, use);
    visit_result->push_back(load);
    output_term = new QuadTerm(result_temp);
}


// 临时变量
void Tree2Quad::visit(TempExp* node) { output_term = new QuadTerm(node); }

// Name (将标签转换为 QuadTerm)
void Tree2Quad::visit(Name* node) { output_term = new QuadTerm(node); }

// 常量
void Tree2Quad::visit(Const* node) { output_term = new QuadTerm(node); }

// 正则化之后不会有 Eseq 节点
void Tree2Quad::visit(Eseq* node) {
    visit_result = nullptr;
    output_term = nullptr;
    return;
}

// 函数调用
void Tree2Quad::visit(Call* node) {
    set<Temp*> *def = new set<Temp*>();     // 定义的临时变量集合
    set<Temp*> *use = new set<Temp*>();     // 使用的临时变量集合
    
    // 对象实例
    node->obj->accept(*this);
    QuadTerm* obj_term = output_term;
    use->insert(obj_term->get_temp()->temp);


    // 参数列表
    vector<QuadTerm*> *args = new vector<QuadTerm*>();
    if (node->args) {
        for (auto arg : *(node->args)) {
            arg->accept(*this);
            args->push_back(output_term);
            // 如果参数是 temp ，添加到 use 中
            if (output_term->kind == QuadTermKind::TEMP)
                use->insert(output_term->get_temp()->temp);
        }
    }

    QuadCall* call = new QuadCall(node, nullptr, node->id, obj_term, args, def, use);
    visit_result->push_back(call);
    output_term = nullptr;
}


// 外部函数调用
void Tree2Quad::visit(ExtCall* node) {    
    set<Temp*> *def = new set<Temp*>();
    set<Temp*> *use = new set<Temp*>();

    // 参数列表
    vector<QuadTerm*> *args = new vector<QuadTerm*>();
    if (node->args) {
        for (auto arg : *(node->args)) {
            arg->accept(*this);
            args->push_back(output_term);
            // 如果参数是临时变量，添加到use中
            if (output_term->kind == QuadTermKind::TEMP)
                use->insert(output_term->get_temp()->temp);
        }
    }

    QuadExtCall* extcall = new QuadExtCall(node, nullptr, node->extfun, args, def, use);
    visit_result->push_back(extcall);
    output_term = nullptr;
}