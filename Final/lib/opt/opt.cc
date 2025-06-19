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

// 计算哪些基本块可执行、每个临时变量的取值状态
void Opt::calculateBT()
{
    block_executable[func->quadblocklist->at(0)->entry_label->num] = true;

start:

    for (auto block : *func->quadblocklist) {
        if (!block_executable[block->entry_label->num])
            continue; // 如果块不可达，跳过

        for (auto stm : *block->quadlist) {
            // 语句->标签
            if (stm->kind == QuadKind::LABEL) {
                QuadLabel* label = static_cast<QuadLabel*>(stm);
                auto block = label2block[label->label->num];
                if (block_executable[label->label->num] && block->exit_labels->size() == 1) {
                    auto next_label = block->exit_labels->at(0);
                    if (setExecutable(next_label->num, true))
                        goto start;
                }
            }

            // 语句->条件跳转
            else if (stm->kind == QuadKind::CJUMP) {
                QuadCJump* cjump = static_cast<QuadCJump*>(stm);
                auto left = cjump->left, right = cjump->right;
                bool left_one = false;
                bool right_one = false;
                int left_val = 0;
                int right_val = 0;

                if (left->kind == QuadTermKind::CONST) {
                    left_one = true;
                    left_val = get<int>(left->term);
                } else if (left->kind == QuadTermKind::TEMP) {
                    auto left_temp = get<TempExp*>(left->term);
                    auto left_rtvalue = getRtValue(left_temp->temp->num);
                    if (left_rtvalue.getType() == ValueType::ONE_VALUE) {
                        left_one = true;
                        left_val = left_rtvalue.getIntValue();
                    } else if (left_rtvalue.getType() == ValueType::MANY_VALUES) {
                        if (setExecutable(cjump->t->num, true) || setExecutable(cjump->f->num, true))
                            goto start;
                    }
                }

                if (right->kind == QuadTermKind::CONST) {
                    right_one = true;
                    right_val = get<int>(right->term);
                } else if (right->kind == QuadTermKind::TEMP) {
                    auto right_temp = get<TempExp*>(right->term);
                    auto right_rtvalue = getRtValue(right_temp->temp->num);
                    if (right_rtvalue.getType() == ValueType::ONE_VALUE) {
                        right_one = true;
                        right_val = right_rtvalue.getIntValue();
                    } else if (right_rtvalue.getType() == ValueType::MANY_VALUES) {
                        if (setExecutable(cjump->t->num, true) || setExecutable(cjump->f->num, true))
                            goto start;
                    }
                }

                // 如果左边和右边都是单值
                if (left_one && right_one) {
                    bool condition_met = false;
                    if (cjump->relop == "==")
                        condition_met = (left_val == right_val);
                    else if (cjump->relop == "!=")
                        condition_met = (left_val != right_val);
                    else if (cjump->relop == "<")
                        condition_met = (left_val < right_val);
                    else if (cjump->relop == "<=")
                        condition_met = (left_val <= right_val);
                    else if (cjump->relop == ">")
                        condition_met = (left_val > right_val);
                    else if (cjump->relop == ">=")
                        condition_met = (left_val >= right_val);

                    if (condition_met == true) {
                        if (setExecutable(cjump->t->num, true))
                            goto start;
                    } 
                    else if (condition_met == false) {
                        if (setExecutable(cjump->f->num, true))
                            goto start;
                    }
                }
            }

            // 语句->赋值
            else if (stm->kind == QuadKind::MOVE) {
                QuadMove* move = static_cast<QuadMove*>(stm);
                if (move->src->kind == QuadTermKind::CONST) {
                    if (setRtValue(move->dst->temp->num, RtValue(get<int>(move->src->term))))
                        goto start;
                }

                else if (move->src->kind == QuadTermKind::TEMP) {
                    auto src_rtvalue = getRtValue(get<TempExp*>(move->src->term)->temp->num);
                    if (setRtValue(move->dst->temp->num, src_rtvalue))
                        goto start;
                }
            }

            // 语句->二元操作赋值
            else if (stm->kind == QuadKind::MOVE_BINOP) {
                QuadMoveBinop* move_binop = static_cast<QuadMoveBinop*>(stm);
                auto left = move_binop->left, right = move_binop->right;
                bool left_one = false, right_one = false;
                int left_value = 0, right_value = 0;

                if (left->kind == QuadTermKind::CONST) {
                    left_one = true;
                    left_value = get<int>(left->term);
                } else if (left->kind == QuadTermKind::TEMP) {
                    auto left_temp = get<TempExp*>(left->term);
                    auto left_rtvalue = getRtValue(left_temp->temp->num);
                    if (left_rtvalue.getType() == ValueType::ONE_VALUE) {
                        left_one = true;
                        left_value = left_rtvalue.getIntValue();
                    } else if (left_rtvalue.getType() == ValueType::MANY_VALUES) {
                        if (setRtValue(move_binop->dst->temp->num, RtValue(ValueType::MANY_VALUES)))
                            goto start;
                    }
                }

                if (right->kind == QuadTermKind::CONST) {
                    right_one = true;
                    right_value = get<int>(right->term);
                } else if (right->kind == QuadTermKind::TEMP) {
                    auto right_temp = get<TempExp*>(right->term);
                    auto right_rtvalue = getRtValue(right_temp->temp->num);
                    if (right_rtvalue.getType() == ValueType::ONE_VALUE) {
                        right_one = true;
                        right_value = right_rtvalue.getIntValue();
                    } else if (right_rtvalue.getType() == ValueType::MANY_VALUES) {
                        if (setRtValue(move_binop->dst->temp->num, RtValue(ValueType::MANY_VALUES)))
                            goto start;
                    }
                }

                if (left_one && right_one) {
                    int result = 0;
                    if (move_binop->binop == "+")
                        result = left_value + right_value;
                    else if (move_binop->binop == "-")
                        result = left_value - right_value;
                    else if (move_binop->binop == "*")
                        result = left_value * right_value;

                    if (setRtValue(move_binop->dst->temp->num, RtValue(result)))
                        goto start;
                }
            }

            // 语句->Phi函数
            else if (stm->kind == QuadKind::PHI) {
                int prev_value = 0;
                bool has_value = false;
                QuadPhi* phi = static_cast<QuadPhi*>(stm);
                for (auto& arg : *phi->args) {
                    auto temp = arg.first;
                    auto label = arg.second;
                    auto rtvalue = getRtValue(temp->num);

                    if (rtvalue.getType() == ValueType::MANY_VALUES && block_executable[label->num]) {
                        if (setRtValue(phi->temp->temp->num, RtValue(ValueType::MANY_VALUES)))
                            goto start;
                        has_value = false;
                        break;
                    }
                    else if (rtvalue.getType() == ValueType::ONE_VALUE && block_executable[label->num]) {
                        int cur_value = rtvalue.getIntValue();
                        if (has_value && prev_value != cur_value) {
                            if (setRtValue(phi->temp->temp->num, RtValue(ValueType::MANY_VALUES)))
                                goto start;
                            has_value = false;
                            break;
                        }
                        has_value = true;
                        prev_value = cur_value;
                    }
                }

                if (has_value) {
                    if (setRtValue(phi->temp->temp->num, RtValue(prev_value)))
                        goto start;
                }
            }

            // 语句->读内存
            else if (stm->kind == QuadKind::LOAD) {
                auto load = static_cast<QuadLoad*>(stm);
                if (setRtValue(load->dst->temp->num, RtValue(ValueType::MANY_VALUES)))
                    goto start;
            }

            // 语句->类方法函数调用 (需要赋值)
            else if (stm->kind == QuadKind::MOVE_CALL) {
                auto move_call = static_cast<QuadMoveCall*>(stm);
                if (setRtValue(move_call->dst->temp->num, RtValue(ValueType::MANY_VALUES)))
                    goto start;
            }

            // 语句->外部函数调用 (需要赋值)
            else if (stm->kind == QuadKind::MOVE_EXTCALL) {
                auto move_extcall = static_cast<QuadMoveExtCall*>(stm);
                if (setRtValue(move_extcall->dst->temp->num, RtValue(ValueType::MANY_VALUES)))
                    goto start;
            }
        }
    }
}

// 将临时变量替换为常量（若为常量），并尽可能删除指令和基本块。
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
            if (stm->kind == QuadKind::MOVE) {
                auto move = static_cast<QuadMove*>(stm);
                if (getRtValue(move->dst->temp->num).getType() == ValueType::ONE_VALUE) {
                    stmIt = stmts.erase(stmIt);
                    goto modifyFunc;
                }
            }

            // 语句->二元操作赋值
            else if (stm->kind == QuadKind::MOVE_BINOP) {
                auto move_binop = static_cast<QuadMoveBinop*>(stm);
                if (getRtValue(move_binop->dst->temp->num).getType() == ValueType::ONE_VALUE) {
                    stmIt = stmts.erase(stmIt);
                    goto modifyFunc;
                }
            }

            // 语句->Phi函数
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
                    bool condition_met = false;
                    if (cjump->relop == "==")
                        condition_met = (left_value == right_value);
                    else if (cjump->relop == "!=")
                        condition_met = (left_value != right_value);
                    else if (cjump->relop == "<")
                        condition_met = (left_value < right_value);
                    else if (cjump->relop == "<=")
                        condition_met = (left_value <= right_value);
                    else if (cjump->relop == ">")
                        condition_met = (left_value > right_value);
                    else if (cjump->relop == ">=")
                        condition_met = (left_value >= right_value);

                    // 替换为Jump
                    auto target_label = condition_met ? cjump->t : cjump->f;
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