#define DEBUG
#undef DEBUG

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "quad.hh"
#include "flowinfo.hh"
#include "color.hh"
#include "quad2rpi.hh"

static string current_funcname = "";

// 将函数名转换为汇编器可接受的格式 (将特殊字符替换为$)
string normalizeName(string name) {
    if (name == "_^main^_^main") { //speial case for main
        return "main";
    }
    for (char& c : name) {
        if (!isalnum(c)) {
            c = '$';
        }
    }
    return name;
}

bool rpi_isMachineReg(int n) {return (n >= 0 && n <= 15);}

// 将一个 QuadTerm 转换为 RPI 汇编代码中的字符串表示
string term2str(QuadTerm *term, Color *color, bool isLeft = true) {
    string result;
    if (term->kind == QuadTermKind::TEMP) {
        Temp *t = term->get_temp()->temp;
        // 如果溢出变量，则使用 r9 或 r10
        if (color->is_spill(t->num))
            result = "r" + to_string(isLeft ? 9 : 10);
        else
            result = "r" + to_string(color->color_of(t->num));
    } else if (term->kind == QuadTermKind::CONST) {
        result = "#" + to_string(term->get_const());
    } else if (term->kind == QuadTermKind::MAME) {
        result = "@" + term->get_name();
    } else {
        cerr << "Error: Unknown term kind" << endl;
        exit(EXIT_FAILURE);
    }
    return result;
}

// 始终使用函数名作为标签前缀
// 注意：Jump 和 CJump 的标签也应遵循此规则
string convert(QuadLabel* label, Color *c, int indent) {
    string result; 
    result = current_funcname + "$" + label->label->str() + ": \n";
    return result;
}

// 获取目标寄存器
int getDstReg(int dstNum, Color* color) {
    if (color->is_spill(dstNum)) return 10; // r10
    return color->color_of(dstNum);
}

// 获取源寄存器
int getSrcReg(QuadTerm* srcTerm, Color* color, bool isLeft) {
    if (srcTerm->kind == QuadTermKind::TEMP) {
        auto srcNum = srcTerm->get_temp()->temp->num;
        if (color->is_spill(srcNum)) return isLeft ? 9 : 10; // r9 or r10
        return color->color_of(srcNum);
    }
    return -1;
}

// 加载源寄存器从栈中 (如果溢出)
string loadSrcReg(QuadTerm* srcTerm, Color* color, int indent, bool isLeft) {
    if (srcTerm->kind == QuadTermKind::TEMP) {
        auto srcNum = srcTerm->get_temp()->temp->num;
        if (color->is_spill(srcNum))
            return string(indent, ' ') + "ldr r" + to_string(isLeft ? 9 : 10) + ", [fp, #"
                + to_string(-color->spill_offset.at(srcNum)) + "]\n";
    }
    return "";
}

// 存储目标寄存器到栈中 (如果溢出)
string storeDstReg(int dstNum, Color* color, int indent) {
    if (color->is_spill(dstNum))
        return string(indent, ' ') + "str r10, [fp, #" + to_string(-color->spill_offset.at(dstNum)) + "]\n";
    return "";
}



// 遍历函数中的四元式并逐个转换
// 有时可以将两条语句合并为一条（尝试这样做！）
// 1) 在函数开头正确设置栈帧，并处理返回
// 2) 使用颜色映射将临时变量替换为寄存器
// 3) 处理溢出的临时变量
// 4) 以及其他细节
string convert(QuadFuncDecl* func, DataFlowInfo *dfi, Color *color, int indent) {
    current_funcname = normalizeName(func->funcname);
    
    string result; 
    result.reserve(10000);
    result += "@ Here's function: " + func->funcname + "\n\n";
    result += ".balign 4\n";
    result += ".global " + current_funcname + "\n";
    result += ".section .text\n\n";
    result += current_funcname + ":\n";
    result += string(indent, ' ') + "push {r4-r10, fp, lr}\n";
    result += string(indent, ' ') + "add fp, sp, #32\n";

    // 为溢出的临时变量分配栈空间
    if (color->spills.size() > 0)
        result += string(indent, ' ') + "sub sp, sp, #" + to_string(color->spills.size() * 4) + "\n";

    // 遍历每个基本块
    for (auto bk : *func->quadblocklist) {
        auto ql = bk->quadlist;
        // 遍历每条语句
        for (auto iter = ql->begin(); iter != ql->end(); iter++) {
            QuadStm *stm = *iter;
            QuadStm *nextStm = (iter + 1 != ql->end()) ? *(iter + 1) : nullptr;
            auto nextLiveout = nextStm ? &dfi->liveout->at(nextStm) : nullptr;

            QuadLabel *nextLabel = (nextStm && nextStm->kind == QuadKind::LABEL) ? static_cast<QuadLabel*>(nextStm) : nullptr;
            QuadLoad *nextLoad = (nextStm && nextStm->kind == QuadKind::LOAD) ? static_cast<QuadLoad*>(nextStm) : nullptr;
            QuadStore *nextStore = (nextStm && nextStm->kind == QuadKind::STORE) ? static_cast<QuadStore*>(nextStm) : nullptr;

            // 如果当前语句是标签，则直接转换为 RPI 格式
            if (stm->kind == QuadKind::LABEL)
                result += convert(static_cast<QuadLabel*>(stm), color, indent);
            
            // QuadMove
            else if (stm->kind == QuadKind::MOVE) {
                auto moveStm = static_cast<QuadMove*>(stm);
                auto dstNum = moveStm->dst->temp->num;
                auto dstReg = getDstReg(dstNum, color);
                auto srcReg = getSrcReg(moveStm->src, color, true);
            
                // 如果源和目标寄存器相同，则不需要移动
                if (dstReg == srcReg) continue;

                // 从栈中加载溢出变量 (如果 src 是溢出变量)
                result += loadSrcReg(moveStm->src, color, indent, true);

                // 添加 Move 指令
                result += string(indent, ' ') + "mov r" + to_string(dstReg) + ", " + term2str(moveStm->src, color) + "\n";
                
                // 存储溢出变量到栈中 (如果 dst 是溢出变量)
                result += storeDstReg(dstNum, color, indent);
            }

            // QuadMoveBinop 
            else if (stm->kind == QuadKind::MOVE_BINOP) {
                auto moveBinopStm = static_cast<QuadMoveBinop*>(stm);
                auto dstNum = moveBinopStm->dst->temp->num;
                auto dstReg = getDstReg(dstNum, color);
                auto leftTerm = moveBinopStm->left;
                auto rightTerm = moveBinopStm->right;
                auto binop = moveBinopStm->binop;

                if (binop == "+" && rightTerm->kind == QuadTermKind::CONST) {
                    // 如果下一条语句是 Load 或 Store，且加法结果“仅仅只会”用于下一条指令
                    // ldr dst [left, right]
                    if (nextLoad && nextLoad->src->kind == QuadTermKind::TEMP
                        && nextLoad->src->get_temp()->temp->num == dstNum && nextLiveout
                        && nextLiveout->find(dstNum) == nextLiveout->end()) {
                        
                        // 使用下一条 Load 的目标寄存器
                        dstNum = nextLoad->dst->temp->num;
                        dstReg = getDstReg(dstNum, color);

                        // 从栈中加载溢出变量 (如果 dst 是溢出变量)
                        result += loadSrcReg(nextLoad->src, color, indent, true);

                        // 添加 Load 指令
                        result += string(indent, ' ') + "ldr r" + to_string(dstReg) + ", ["
                            + term2str(leftTerm, color) + ", " + term2str(rightTerm, color) + "]\n";
                        
                        // 存储溢出变量到栈中 (如果 dst 是溢出变量)
                        result += storeDstReg(dstNum, color, indent);
                        iter++;
                        continue;
                    }
                    // str src [left, right]
                    if (nextStore && nextStore->dst->kind == QuadTermKind::TEMP
                        && nextStore->dst->get_temp()->temp->num == dstNum && nextLiveout
                        && nextLiveout->find(dstNum) == nextLiveout->end()) {
                        
                        // 如果 src 或 leftTerm 是溢出变量，则从栈中加载
                        result += loadSrcReg(nextStore->src, color, indent, true);
                        result += loadSrcReg(leftTerm, color, indent, false);

                        // 添加 Store 指令
                        result += string(indent, ' ') + "str " + term2str(nextStore->src, color, true) + ", ["
                            + term2str(leftTerm, color, false) + ", " + term2str(rightTerm, color) + "]\n";

                        iter++;
                        continue;
                    }
                }

                // MoveBinop 的一般情况
                result += loadSrcReg(leftTerm, color, indent, true);
                result += loadSrcReg(rightTerm, color, indent, false);

                if (binop == "+")
                    result += string(indent, ' ') + "add ";
                else if (binop == "-")
                    result += string(indent, ' ') + "sub ";
                else if (binop == "*")
                    result += string(indent, ' ') + "mul ";
                
                result += "r" + to_string(dstReg) + ", " + term2str(moveBinopStm->left, color, true) 
                    + ", " + term2str(moveBinopStm->right, color, false) + "\n";

                // 存储溢出变量到栈中 (如果 dst 是溢出变量)
                result += storeDstReg(dstNum, color, indent);
            }

            // QuadLoad
            else if (stm->kind == QuadKind::LOAD) {
                auto loadStm = static_cast<QuadLoad*>(stm);
                auto dstNum = loadStm->dst->temp->num;
                auto dstReg = getDstReg(dstNum, color);
                auto srcTerm = loadStm->src;

                // 从栈中加载溢出变量 (如果 dst 是溢出变量)
                result += loadSrcReg(srcTerm, color, indent, true);

                // 添加 Load 指令
                result += string(indent, ' ') + "ldr r" + to_string(dstReg) + ", ";
                if (srcTerm->kind == QuadTermKind::TEMP) {
                    // 如果是 temp ，则加[]表示“从这个地址加载”
                    result += "[" + term2str(srcTerm, color) + "]\n";
                } else {
                    // 如果是常量或名称，则直接加载
                    result += term2str(srcTerm, color) + "\n";
                }
                
                // 存储溢出变量到栈中 (如果 dst 是溢出变量)
                result += storeDstReg(dstNum, color, indent);
            }

            // QuadStore
            else if (stm->kind == QuadKind::STORE) {
                auto storeStm = static_cast<QuadStore*>(stm);
                auto srcTerm = storeStm->src;
                auto dstTerm = storeStm->dst;

                result += loadSrcReg(srcTerm, color, indent, true);
                result += loadSrcReg(dstTerm, color, indent, false);

                // 添加 Store 指令
                // str src [dst]
                result += string(indent, ' ') + "str " + term2str(srcTerm, color, true) + ", [" + term2str(dstTerm, color, false) + "]\n";
            }

            // QuadCall
            else if (stm->kind == QuadKind::CALL) {
                auto callStm = static_cast<QuadCall*>(stm);
                result += loadSrcReg(callStm->obj_term, color, indent, true);
                result += string(indent, ' ') + "blx " + term2str(callStm->obj_term, color) + "\n";
            }

            // QuadMoveCall
            else if (stm->kind == QuadKind::MOVE_CALL) {
                auto moveCallStm = static_cast<QuadMoveCall*>(stm);
                result += loadSrcReg(moveCallStm->call->obj_term, color, indent, true);
                result += string(indent, ' ') + "blx " + term2str(moveCallStm->call->obj_term, color) + "\n";
            }

            // QuadExtCall
            else if (stm->kind == QuadKind::EXTCALL) {
                auto extCallStm = static_cast<QuadExtCall*>(stm);
                result += string(indent, ' ') + "bl " + extCallStm->extfun + "\n";
            }

            // QuadJump
            else if (stm->kind == QuadKind::JUMP) {
                auto jumpStm = static_cast<QuadJump*>(stm);
                if (nextLabel && nextLabel->label->num == jumpStm->label->num) continue;    // 省略没必要的跳转
                result += string(indent, ' ') + "b " + current_funcname + "$" + jumpStm->label->str() + "\n";
            }

            // QuadCJump
            else if (stm->kind == QuadKind::CJUMP) {
                auto cjumpStm = static_cast<QuadCJump*>(stm);
                auto relop = cjumpStm->relop;

                result += loadSrcReg(cjumpStm->left, color, indent, true);
                result += loadSrcReg(cjumpStm->right, color, indent, false);

                // 添加 cmp 指令
                result += string(indent, ' ') + "cmp " + term2str(cjumpStm->left, color, true) 
                    + ", " + term2str(cjumpStm->right, color, false) + "\n";

                // 添加条件跳转指令
                result += string(indent, ' ');
                if (relop == "==")
                    result += "beq ";
                else if (relop == "!=")
                    result += "bne ";
                else if (relop == "<")
                    result += "blt ";
                else if (relop == "<=")
                    result += "ble ";
                else if (relop == ">")
                    result += "bgt ";
                else if (relop == ">=")
                    result += "bge ";
                result += current_funcname + "$" + cjumpStm->t->str() + "\n";

                // 添加跳转到 false 标签
                if (nextLabel && nextLabel->label->num == cjumpStm->f->num) continue; // 省略没必要的跳转
                result += string(indent, ' ') + "b " + current_funcname + "$" + cjumpStm->f->str() + "\n";
            }

            // QuadReturn
            else if (stm->kind == QuadKind::RETURN) {
                result += string(indent, ' ') + "sub sp, fp, #32\n";
                result += string(indent, ' ') + "pop {r4-r10, fp, pc}\n"; // 恢复寄存器和栈帧
            }
        }
    }

    return result;
}

// 将 QuadProgram 转换为带 k 个寄存器的 RPI 格式
string quad2rpi(QuadProgram* quadProgram, ColorMap *cm) {
    string result; result.reserve(10000);
    // 遍历Quad程序中的函数声明
    result = ".section .note.GNU-stack\n\n@ Here is the RPI code\n\n";
    for (QuadFuncDecl* func : *quadProgram->quadFuncDeclList) {
        //获取函数的数据流信息
        DataFlowInfo *dfi = new DataFlowInfo(func);
        dfi->computeLiveness(); //活跃性分析在某些情况下有用。必须在trace前完成！
        trace(func); //合并所有基本块
        current_funcname = func->funcname; //设置全局变量
        //获取该函数的寄存器分配方案
        Color *c = cm->color_map[func->funcname]; 
        int indent = 8;
        result += convert(func, dfi, c, indent) + "\n";
    }
    //在末尾添加全局函数声明
    result += ".global malloc\n";
    result +=".global getint\n";
    result += ".global putint\n";
    result += ".global putch\n";
    result += ".global putarray\n";
    result += ".global getch\n";
    result += ".global getarray\n";
    result += ".global starttime\n";
    result += ".global stoptime\n";
    return result;
}

// 将 RPI 代码输出到文件
void quad2rpi(QuadProgram* quadProgram, ColorMap *cm, string filename) {
    ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << quad2rpi(quadProgram, cm);
        outfile.flush();
        outfile.close();
    } else {
        cerr << "Error: Unable to open file " << filename << endl;
    }
}