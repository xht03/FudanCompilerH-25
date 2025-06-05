#define DEBUG
//#undef DEBUG

#include <string>
#include <stack>
#include <variant>
#include <vector>
#include <map>
#include <queue>
#include "quad.hh"
#include "opt.hh"

void Opt::updateRtValue(int temp_num, int val) {
    // 如果当前 NO_VALUE
    if (temp_value[temp_num].getType() == ValueType::NO_VALUE) {
        // 则直接设置为 ONE_VALUE
        temp_value[temp_num] = RtValue(val);
    } 

    // 如果当前是 ONE_VALUE
    else if (temp_value[temp_num].getType() == ValueType::ONE_VALUE) {
        // 如果新值!=现有值，则变为 MANY_VALUES
        if (temp_value[temp_num].getIntValue() != val) {
            temp_value[temp_num] = RtValue(ValueType::MANY_VALUES);
        }
        // 如果新值==现有值，则不做任何修改
    }

    // 如果当前是 MANY_VALUES，不做任何修改
}

RtValue Opt::getQuadTermRtValue(QuadTerm* term) {
    if (term->kind == QuadTermKind::CONST) {
        return RtValue(term->get_const());
    } else if (term->kind == QuadTermKind::TEMP) {
        int temp_num = term->get_temp()->temp->num;
        RtValue rt_value = getRtValue(temp_num);
    } else {
        return RtValue(ValueType::MANY_VALUES); // 对于 NAME 或其他类型，默认 MANY_VALUES
    }
}

void Opt::calculateBT() {
    
    // 起始块 B1 设为可执行
    int B1 = func->quadblocklist->front()->entry_label->num;
    block_executable[B1] = true;

    queue<int> block_queue;
    block_queue.push(B1);

    while (!block_queue.empty()) {
        // 取出队首的 block
        int label = block_queue.front();
        block_queue.pop();
        QuadBlock* block = label2block[label];

        // 如果该 block 已被标记为不可执行，则跳过
        if (!block_executable[label]) continue;

        // 对于该块中的每一条 quad 指令
        for(auto& quad: *block->quadlist) {
            // QuadMove
            if (quad->kind == QuadKind::MOVE) {
                QuadMove* move = static_cast<QuadMove*>(quad);
                int dst_num = move->dst->temp->num;

                // 如果 src 是 CONST
                if (move->src->kind == QuadTermKind::CONST) {
                    int val = move->src->get_const();
                    updateRtValue(dst_num, val);
                }
                // 如果 src 是 TEMP
                else if (move->src->kind == QuadTermKind::TEMP) {
                    int src_num = move->src->get_temp()->temp->num;
                    RtValue src_val = getRtValue(src_num);
                    if (src_val.getType() == ValueType::ONE_VALUE) {
                        updateRtValue(dst_num, src_val.getIntValue());
                    } else {
                        temp_value[dst_num] = RtValue(ValueType::MANY_VALUES);
                    }
                }
                // 如果 src 是 NAME 或其他类型，默认处理为 MANY_VALUES
                else temp_value[dst_num] = RtValue(ValueType::MANY_VALUES);
            }

            // QuadMoveBinop
            else if (quad->kind == QuadKind::MOVE_BINOP) {
                QuadMoveBinop* move_binop = static_cast<QuadMoveBinop*>(quad);
                int dst_num = move_binop->dst->temp->num;
                
                // 左操作数
                ValueType left_type;
                int left_val = 0;
                if (move_binop->left->kind == QuadTermKind::CONST) {
                    left_type = ValueType::ONE_VALUE;
                    left_val = move_binop->left->get_const();
                }
                else if (move_binop->left->kind == QuadTermKind::TEMP) {
                    if (getRtValue(move_binop->left->get_temp()->temp->num).getType() == ValueType::ONE_VALUE) {
                        left_type = ValueType::ONE_VALUE;
                        left_val = getRtValue(move_binop->left->get_temp()->temp->num).getIntValue();
                    }
                    else if (getRtValue(move_binop->left->get_temp()->temp->num).getType() == ValueType::NO_VALUE) {
                        left_type = ValueType::NO_VALUE;
                    }
                    else
                        left_type = ValueType::MANY_VALUES;
                }
                else left_type = ValueType::MANY_VALUES; // 对于其他类型，默认 MANY_VALUES
                
                // 右操作数
                ValueType right_type;
                int right_val = 0;
                if (move_binop->right->kind == QuadTermKind::CONST) {
                    right_type = ValueType::ONE_VALUE;
                    right_val = move_binop->right->get_const();
                }
                else if (move_binop->right->kind == QuadTermKind::TEMP) {
                    if (getRtValue(move_binop->right->get_temp()->temp->num).getType() == ValueType::ONE_VALUE) {
                        right_type = ValueType::ONE_VALUE;
                        right_val = getRtValue(move_binop->right->get_temp()->temp->num).getIntValue();
                    }
                    else if (getRtValue(move_binop->right->get_temp()->temp->num).getType() == ValueType::NO_VALUE) {
                        right_type = ValueType::NO_VALUE;
                    }
                    else
                        right_type = ValueType::MANY_VALUES;
                }
                else right_type = ValueType::MANY_VALUES; // 对于其他类型，默认 MANY_VALUES

                // 如果左操作数和右操作数都是 ONE_VALUE
                if (left_type == ValueType::ONE_VALUE && right_type == ValueType::ONE_VALUE) {
                    int result = 0;
                    if (move_binop->binop == "+") {
                        result = left_val + right_val;
                    } else if (move_binop->binop == "-") {
                        result = left_val - right_val;
                    } else if (move_binop->binop == "*") {
                        result = left_val * right_val;
                    } else if (move_binop->binop == "/") {
                        if (right_val != 0) result = left_val / right_val; // 避免除以零
                        else result = 0; // 除以零的情况，默认结果为 0
                    }
                    updateRtValue(dst_num, result);
                } 
                // 如果有一个是 NO_VALUES
                else if (left_type == ValueType::NO_VALUE || right_type == ValueType::NO_VALUE) {
                    temp_value[dst_num] = RtValue(ValueType::NO_VALUE);
                }
                else temp_value[dst_num] = RtValue(ValueType::MANY_VALUES);
            }

            // QuadPhi
            else if (quad->kind == QuadKind::PHI) {
                QuadPhi* phi = static_cast<QuadPhi*>(quad);
                set<int> src_vals;

                for (auto& arg : *phi->args) {
                    int src_num = arg.first->num;
                    RtValue src_val = getRtValue(src_num);
                    int src_label = arg.second->num;

                    if (!block_executable[src_label]) 
                        continue;
                    
                    if (src_val.getType() == ValueType::MANY_VALUES) {
                            temp_value[phi->temp->temp->num] = RtValue(ValueType::MANY_VALUES);
                            break; // 如果有一个 MANY_VALUES，就直接跳出
                    }
                    
                    src_vals.insert(src_val.getIntValue());
                }

                // 如果所有来源的值都相同，则更新为 ONE_VALUE
                if (src_vals.size() == 1)
                    updateRtValue(phi->temp->temp->num, *src_vals.begin());
                else
                    temp_value[phi->temp->temp->num] = RtValue(ValueType::MANY_VALUES);
            }

            // QuadMoveCall
            else if (quad->kind == QuadKind::MOVE_CALL) {
                QuadMoveCall* move_call = static_cast<QuadMoveCall*>(quad);
                int dst_num = move_call->dst->temp->num;
                temp_value[dst_num] = RtValue(ValueType::MANY_VALUES);
            }

            // QuadLoad
            else if (quad->kind == QuadKind::LOAD) {
                QuadLoad* load = static_cast<QuadLoad*>(quad);
                int dst_num = load->dst->temp->num;
                temp_value[dst_num] = RtValue(ValueType::MANY_VALUES);
            }

            // 
            else if (quad->kind == QuadKind::CJUMP) {
                QuadCJump* cjump = static_cast<QuadCJump*>(quad);
                // 如果 left 和 right 都是 CONST
                if (cjump->left->kind == QuadTermKind::CONST && cjump->right->kind == QuadTermKind::CONST) {
                    int left_val = cjump->left->get_const();
                    int right_val = cjump->right->get_const();

                    bool condition_met = false;
                    if (cjump->relop == "==") {
                        condition_met = (left_val == right_val);
                    } else if (cjump->relop == "!=") {
                        condition_met = (left_val != right_val);
                    } else if (cjump->relop == "<") {
                        condition_met = (left_val < right_val);
                    } else if (cjump->relop == "<=") {
                        condition_met = (left_val <= right_val);
                    } else if (cjump->relop == ">") {
                        condition_met = (left_val > right_val);
                    } else if (cjump->relop == ">=") {
                        condition_met = (left_val >= right_val);
                    }

                    if (condition_met) {
                        block_executable[cjump->t->num] = true;
                        if (label2block.find(cjump->t->num) != label2block.end())
                            block_queue.push(cjump->t->num);   
                    } 
                    else {
                        block_executable[cjump->f->num] = true;
                        if (label2block.find(cjump->f->num) != label2block.end())
                            block_queue.push(cjump->f->num);
                    }
                }
                // 如果
                else {}
            }
        }
    }
}


void Opt::modifyFunc() {
    //your code here
    //this is to change the temp values to consts (if it is const), and remove instructions and blocks when possible
 
}

QuadFuncDecl* Opt::optFunc() {
    // Initialize the block_executable map
    for (auto& block : *func->quadblocklist) {
        block_executable[block->entry_label->num] = false;
        label2block[block->entry_label->num] = block;
    }

    // Initialize the temp_value map for parameters
    for (auto& temp : *func->params) {
        temp_value[temp->num] = RtValue(ValueType::MANY_VALUES); // Initialize to many values for all parameters
    }

    calculateBT();

    printRtValue(); 
    printBlockExecutable();

    modifyFunc();

    return func;
}

QuadProgram* optProg(QuadProgram* prog) {
    QuadProgram* newProg = new QuadProgram(nullptr, new vector<QuadFuncDecl*>());
    for (int i=0; i < prog->quadFuncDeclList->size(); i++) {
        Opt optthis(prog->quadFuncDeclList->at(i));
        newProg->quadFuncDeclList->push_back(optthis.optFunc());
    }
    return newProg;
}