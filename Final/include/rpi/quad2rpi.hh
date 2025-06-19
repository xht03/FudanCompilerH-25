#ifndef __QUAD2RPI_HH
#define __QUAD2RPI_HH
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "quad.hh"
#include "color.hh"

using namespace std;
using namespace quad;

void trace(QuadFuncDecl* func);     // 对单个函数的 Quad 基本块进行 trace ，将所有基本块合并为一个顺序块
void trace(QuadProgram* prog);      // 对整个程序的所有函数进行 trace

// 判断编号为 n 的寄存器是否为 RPI 的物理寄存器（机器寄存器）
bool rpi_isMachineReg(int n);
// 将 QuadProgram 转换为 RPI 汇编代码
string quad2rpi(QuadProgram* quadProgram, ColorMap *cm);
// 同上，并输出到指定文件
void quad2rpi(QuadProgram* quadProgram, ColorMap *cm, string filename);

#endif // __QUAD2RPI_HH