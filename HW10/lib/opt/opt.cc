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
        return getRtValue(temp_num);
    } else {
        return RtValue(ValueType::MANY_VALUES); // 对于 NAME 或其他类型，默认 MANY_VALUES
    }
}

void Opt::calculateBT() {

    // 所有块初始化为不可达
    for (auto& block : *func->quadblocklist) {
        int label_num = block->entry_label->num;
        block_executable[label_num] = false;
        label2block[label_num] = block;
    }

    // 函数参数初始化为 MANY_VALUES
    for (auto& param : *func->params) {
        int temp_num = param->num;
        temp_value[temp_num] = RtValue(ValueType::MANY_VALUES);
    }
    
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

        // 记录当前块的状态（用于判断到达不变点）
        auto old_temp_value = temp_value;
        auto old_block_executable = block_executable;

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
                
                RtValue left_val = getQuadTermRtValue(move_binop->left);
                RtValue right_val = getQuadTermRtValue(move_binop->right);

                // 如果左操作数和右操作数都是 ONE_VALUE
                if (left_val.getType() == ValueType::ONE_VALUE && right_val.getType() == ValueType::ONE_VALUE) {
                    int result = 0;
                    int left_int = left_val.getIntValue();
                    int right_int = right_val.getIntValue();
                    if (move_binop->binop == "+") {
                        result = left_int + right_int;
                    } else if (move_binop->binop == "-") {
                        result = left_int - right_int;
                    } else if (move_binop->binop == "*") {
                        result = left_int * right_int;
                    }
                    updateRtValue(dst_num, result);
                } 
                // 如果有一个是 NO_VALUES
                else if (left_val.getType() == ValueType::NO_VALUE || right_val.getType() == ValueType::NO_VALUE) {
                    temp_value[dst_num] = RtValue(ValueType::NO_VALUE);
                }
                else temp_value[dst_num] = RtValue(ValueType::MANY_VALUES);
            }

            // QuadPhi
            else if (quad->kind == QuadKind::PHI) {
                QuadPhi* phi = static_cast<QuadPhi*>(quad);
                set<int> src_vals;
                bool has_many_values = false;

                for (auto& arg : *phi->args) {
                    int src_num = arg.first->num;
                    RtValue src_val = getRtValue(src_num);
                    int src_label = arg.second->num;

                    if (!block_executable[src_label]) 
                        continue;

                    if (src_val.getType() == ValueType::NO_VALUE)
                        continue;
                    
                    if (src_val.getType() == ValueType::MANY_VALUES) {
                        has_many_values = true;
                        break; // 如果有一个 MANY_VALUES，就直接跳出
                    }
                    
                    src_vals.insert(src_val.getIntValue());
                }

                // 如果所有来源的值都相同，则更新为 ONE_VALUE
                if (src_vals.size() == 1)
                    updateRtValue(phi->temp->temp->num, *src_vals.begin());
                else if (has_many_values || src_vals.size() > 1)
                    temp_value[phi->temp->temp->num] = RtValue(ValueType::MANY_VALUES);
                else
                    temp_value[phi->temp->temp->num] = RtValue(ValueType::NO_VALUE);
            }

            // QuadMoveCall
            else if (quad->kind == QuadKind::MOVE_CALL) {
                QuadMoveCall* move_call = static_cast<QuadMoveCall*>(quad);
                int dst_num = move_call->dst->temp->num;
                temp_value[dst_num] = RtValue(ValueType::MANY_VALUES);
            }

            // QuadMoveExtCall
            else if (quad->kind == QuadKind::MOVE_EXTCALL) {
                QuadMoveExtCall* move_extcall = static_cast<QuadMoveExtCall*>(quad);
                int dst_num = move_extcall->dst->temp->num;
                temp_value[dst_num] = RtValue(ValueType::MANY_VALUES);
            }

            // QuadLoad
            else if (quad->kind == QuadKind::LOAD) {
                QuadLoad* load = static_cast<QuadLoad*>(quad);
                int dst_num = load->dst->temp->num;
                temp_value[dst_num] = RtValue(ValueType::MANY_VALUES);
            }

            // QuadJump
            else if (quad->kind == QuadKind::JUMP) {
                QuadJump* jump = static_cast<QuadJump*>(quad);
                int target_label = jump->label->num;
                block_executable[target_label] = true;
            }

            // QuadCJump
            else if (quad->kind == QuadKind::CJUMP) {
                QuadCJump* cjump = static_cast<QuadCJump*>(quad);
                RtValue left_val = getQuadTermRtValue(cjump->left);
                RtValue right_val = getQuadTermRtValue(cjump->right);

                // 如果 left 和 right 都是 ONE_VALUE
                if (left_val.getType() == ValueType::ONE_VALUE && right_val.getType() == ValueType::ONE_VALUE) {
                    int left_int = left_val.getIntValue();
                    int right_int = right_val.getIntValue();
                    bool condition_met = false;

                    if (cjump->relop == "==") {
                        condition_met = (left_int == right_int);
                    } else if (cjump->relop == "!=") {
                        condition_met = (left_int != right_int);
                    } else if (cjump->relop == "<") {
                        condition_met = (left_int < right_int);
                    } else if (cjump->relop == "<=") {
                        condition_met = (left_int <= right_int);
                    } else if (cjump->relop == ">") {
                        condition_met = (left_int > right_int);
                    } else if (cjump->relop == ">=") {
                        condition_met = (left_int >= right_int);
                    }

                    if (condition_met) 
                        block_executable[cjump->t->num] = true;
                    else
                        block_executable[cjump->f->num] = true;
                }
                // 如果 left 或 right 是 NO_VALUE
                else if (left_val.getType() == ValueType::NO_VALUE || right_val.getType() == ValueType::NO_VALUE) {
                    block_executable[cjump->t->num] = false;
                    block_executable[cjump->f->num] = false;
                }
                else {
                    block_executable[cjump->t->num] = true;
                    block_executable[cjump->f->num] = true;
                }
            }
        }

        // 如果当前块的可执行性或临时变量的取值状态发生变化，则添加该 block 的后继
        if (old_temp_value != temp_value || old_block_executable != block_executable) {  
            // 遍历 exit_labels，添加进队列
            for (auto& exit_label : *block->exit_labels) {
                int exit_label_num = exit_label->num;
                block_queue.push(exit_label_num);
            }
        }
    }
}

// 将临时变量转为常量（若为常量），并在可能时移除指令和代码块
// void Opt::modifyFunc() {

//     vector<QuadBlock*>* new_blocks = new vector<QuadBlock*>();

//     for (auto block : *func->quadblocklist) {
//         // 如果该块不可执行，则跳过
//         int label_num = block->entry_label->num;
//         if (!block_executable[label_num]) continue;

//         vector<Label*>* new_exit_labels = new vector<Label*>();
//         for (auto exit_label : *block->exit_labels) {
//             if (block_executable[exit_label->num]) {
//                 new_exit_labels->push_back(exit_label);
//             }
//         }

//         vector<QuadStm*>* new_quadlist = new vector<QuadStm*>();
//         for (auto stm : *block->quadlist) {
//             // QuadMove
//             if (stm->kind == QuadKind::MOVE) {
//                 QuadMove* move = static_cast<QuadMove*>(stm);
//                 int dst_num = move->dst->temp->num;
//                 RtValue dst_val = getRtValue(dst_num);

//                 // 如果 dst 是 ONE_VALUE，删除该指令
//                 if (dst_val.getType() == ValueType::ONE_VALUE)
//                     continue;
//                 else
//                     new_quadlist->push_back(move->clone());
//             }

//             // QuadMoveBinop
//             else if (stm->kind == QuadKind::MOVE_BINOP) {
//                 QuadMoveBinop* move_binop = static_cast<QuadMoveBinop*>(stm);
//                 int dst_num = move_binop->dst->temp->num;
//                 RtValue dst_val = getRtValue(dst_num);

//                 // 如果 dst 是 ONE_VALUE，删除该指令
//                 if (dst_val.getType() == ValueType::ONE_VALUE)
//                     continue;
//                 else
//                     new_quadlist->push_back(move_binop->clone());
//             }

//             // QuadMoveCall
//             else if (stm->kind == QuadKind::MOVE_CALL) {
//                 QuadMoveCall* move_call = static_cast<QuadMoveCall*>(stm);
//                 int dst_num = move_call->dst->temp->num;
//                 RtValue dst_val = getRtValue(dst_num);

//                 // 如果 dst 是 ONE_VALUE，删除该指令
//                 if (dst_val.getType() == ValueType::ONE_VALUE)
//                     continue;
//                 else
//                     new_quadlist->push_back(move_call->clone());
//             }

//             // QuadMoveExtCall
//             else if (stm->kind == QuadKind::MOVE_EXTCALL) {
//                 QuadMoveExtCall* move_extcall = static_cast<QuadMoveExtCall*>(stm);
//                 int dst_num = move_extcall->dst->temp->num;
//                 RtValue dst_val = getRtValue(dst_num);

//                 // 如果 dst 是 ONE_VALUE，删除该指令
//                 if (dst_val.getType() == ValueType::ONE_VALUE)
//                     continue;
//                 else
//                     new_quadlist->push_back(move_extcall->clone());
//             }

//             // QuadCJump
//             else if (stm->kind == QuadKind::CJUMP) {
//                 QuadCJump* cjump = static_cast<QuadCJump*>(stm);
//                 RtValue left_val = getQuadTermRtValue(cjump->left);
//                 RtValue right_val = getQuadTermRtValue(cjump->right);

//                 if (left_val.getType() == ValueType::ONE_VALUE && right_val.getType() == ValueType::ONE_VALUE) {
//                     int left_int = left_val.getIntValue();
//                     int right_int = right_val.getIntValue();
//                     bool condition_met = false;

//                     if (cjump->relop == "==") {
//                         condition_met = (left_int == right_int);
//                     } else if (cjump->relop == "!=") {
//                         condition_met = (left_int != right_int);
//                     } else if (cjump->relop == "<") {
//                         condition_met = (left_int < right_int);
//                     } else if (cjump->relop == "<=") {
//                         condition_met = (left_int <= right_int);
//                     } else if (cjump->relop == ">") {
//                         condition_met = (left_int > right_int);
//                     } else if (cjump->relop == ">=") {
//                         condition_met = (left_int >= right_int);
//                     }

//                     // 如果条件满足，替换为 JUMP 到 t，否则替换为 JUMP 到 f
//                     if (condition_met) {
//                         new_quadlist->push_back(new QuadJump(cjump->node, cjump->t, nullptr, nullptr));
//                         // 从 exit_labels 中移除 f
//                         for (auto it = new_exit_labels->begin(); it != new_exit_labels->end();it++) {
//                             if ((*it)->num == cjump->f->num) {
//                                 it = new_exit_labels->erase(it);
//                                 break;
//                             }
//                         }
//                     } else {
//                         new_quadlist->push_back(new QuadJump(cjump->node, cjump->f, nullptr, nullptr));
//                         // 从 exit_labels 中移除 t
//                         for (auto it = new_exit_labels->begin(); it != new_exit_labels->end(); it++) {
//                             if ((*it)->num == cjump->t->num) {
//                                 it = new_exit_labels->erase(it);
//                                 break;
//                             }
//                         }
//                     }
//                 }
//                 else new_quadlist->push_back(cjump->clone());
//             }

//             // QuadPhi
//             else if (stm->kind == QuadKind::PHI) {
//                 QuadPhi* phi = static_cast<QuadPhi*>(stm);
//                 // 如果 phi 的 dst 是 ONE_VALUE，删除该指令
//                 if (temp_value[phi->temp->temp->num].getType() == ValueType::ONE_VALUE)
//                     continue;
//                 // 如果 phi 的 dst 是 MANY_VALUES
//                 else if (temp_value[phi->temp->temp->num].getType() == ValueType::MANY_VALUES) {
//                     for (auto& arg : *phi->args) {
//                         int src_num = arg.first->num;
//                         RtValue src_val = getRtValue(src_num);
//                         int src_label = arg.second->num;
//                         QuadBlock* src_block = label2block[arg.second->num];
                        
//                         // 如果来源是 ONE_VALUE
//                         if (src_val.getType() == ValueType::ONE_VALUE) {
//                             // 在 src 块末尾添加 move 指令
//                             Temp* new_temp = new Temp(func->last_temp_num++);
//                             auto new_def = new set<Temp*>();
//                             new_def->insert(new_temp);
//                             QuadMove* new_move = new QuadMove(nullptr, 
//                                                               new TempExp(Type::INT, new_temp), 
//                                                               new QuadTerm(src_val.getIntValue()), 
//                                                               new_def,
//                                                               new set<Temp*>());
//                             src_block->quadlist->push_back(new_move);
//                             // 更新 phi->args
//                             stm->renameUse(arg.first, new_temp);
//                         }
//                     }
//                     new_quadlist->push_back(phi->clone());
//                 } 
//                 // 如果是 NO_VALUE，暂不处理
//                 else {
//                     new_quadlist->push_back(phi->clone());
//                 }
//             }

//             // 对于其他类型的指令，直接克隆并添加到新的 quadlist
//             else {
//                 new_quadlist->push_back(static_cast<QuadStm*>(stm->clone()));
//             }
//         }

//         QuadBlock* new_block = new QuadBlock(block->node, new_quadlist, block->entry_label, new_exit_labels);
//         new_blocks->push_back(new_block);
//     }

//     func->quadblocklist = new_blocks;
// }

void Opt::modifyFunc()
{
modifyFunc:
    Temp_map temp_map(func->last_temp_num + 1, func->last_label_num + 1);

    auto& blocks = *func->quadblocklist;
    for (auto blockIt = blocks.begin(); blockIt != blocks.end(); blockIt++) {
        auto block = *blockIt;

        // 移除不可达块
        if (!block_executable[block->entry_label->num]) {
            blockIt = blocks.erase(blockIt);
            goto modifyFunc;
        }

        auto& stmts = *block->quadlist;
        for (auto stmIt = stmts.begin(); stmIt != stmts.end(); stmIt++) {
            auto stm = *stmIt;

            // 语句->赋值
            // 如果目标变量是单值，则删除该语句
            if (stm->kind == QuadKind::MOVE) {
                auto move = static_cast<QuadMove*>(stm);
                if (getRtValue(move->dst->temp->num).getType() == ValueType::ONE_VALUE) {
                    stmIt = stmts.erase(stmIt);
                    goto modifyFunc;
                }
            }

            // 语句->二元操作赋值
            // 如果目标变量是单值，则删除该语句
            else if (stm->kind == QuadKind::MOVE_BINOP) {
                auto move_binop = static_cast<QuadMoveBinop*>(stm);
                if (getRtValue(move_binop->dst->temp->num).getType() == ValueType::ONE_VALUE) {
                    stmIt = stmts.erase(stmIt);
                    goto modifyFunc;
                }
            }

            // 语句->Phi函数
            // 如果目标变量是单值，则删除该语句
            // 如果参数有单值, 则在前继块添加变量, 并重命名
            else if (stm->kind == QuadKind::PHI) {
                auto phi = static_cast<QuadPhi*>(stm);
                if (getRtValue(phi->temp->temp->num).getType() == ValueType::ONE_VALUE) {
                    stmIt = stmts.erase(stmIt);
                    goto modifyFunc;
                }

                for (auto& arg : *phi->args) {
                    auto temp = arg.first;
                    auto label = arg.second;
                    auto rtValue = getRtValue(temp->num);
                    // 单值参数需要在前继块添加变量, 并重命名
                    if (rtValue.getType() == ValueType::ONE_VALUE) {
                        auto newTemp = temp_map.newtemp();
                        int value = rtValue.getIntValue();
                        auto preBlock = label2block[label->num];
                        preBlock->quadlist->insert(preBlock->quadlist->end() - 1,
                            new QuadMove(nullptr, new TempExp(Type::INT, newTemp), new QuadTerm(value),
                                new set<Temp*> { newTemp }, new set<Temp*> {}));
                        stm->renameUse(temp, newTemp);
                    }
                }
            }

            // 如果有使用的单值变量，则将其替换为常数
            for (auto useTemp : *stm->cloneTemps(stm->use)) {
                auto rtValue = getRtValue(useTemp->num);
                if (rtValue.getType() == ValueType::ONE_VALUE) {
                    int value = rtValue.getIntValue();
                    stm->constantUse(useTemp, value);
                }
            }

            // 语句->条件跳转 (在常量替换后进行更新)
            if (stm->kind == QuadKind::CJUMP) {
                auto cjump = static_cast<QuadCJump*>(stm);
                auto left = cjump->left, right = cjump->right;
                // 如果可以左右都是常量, 则把CJump替换为Jump
                if (left->kind == QuadTermKind::CONST && right->kind == QuadTermKind::CONST) {
                    int left_value = get<int>(left->term);
                    int right_value = get<int>(right->term);
                    bool result = false;
                    if (cjump->relop == "==")
                        result = (left_value == right_value);
                    else if (cjump->relop == "!=")
                        result = (left_value != right_value);
                    else if (cjump->relop == "<")
                        result = (left_value < right_value);
                    else if (cjump->relop == "<=")
                        result = (left_value <= right_value);
                    else if (cjump->relop == ">")
                        result = (left_value > right_value);
                    else if (cjump->relop == ">=")
                        result = (left_value >= right_value);

                    // 替换为Jump
                    auto target_label = result ? cjump->t : cjump->f;
                    stmIt = stmts.erase(stmIt);
                    stmts.push_back(new QuadJump(nullptr, target_label, new set<Temp*> {}, new set<Temp*> {}));

                    // 更新出口标签
                    block->exit_labels->clear();
                    block->exit_labels->push_back(target_label);
                    goto modifyFunc;
                }
            }
        }
    }

    // 更新标签和变量计数
    func->last_label_num = temp_map.next_label - 1;
    func->last_temp_num = temp_map.next_temp - 1;
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