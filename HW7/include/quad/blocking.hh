#ifndef __QUAD_BLOCKING_HH
#define __QUAD_BLOCKING_HH

#include "temp.hh"
#include "treep.hh"
#include "quad.hh"

using namespace quad;

/**
 * QuadProgram 中的函数进行分块
 * 
 * block 是一段连续执行的代码序列，且：
 * 1. 第一个指令一定是 Label
 * 2. Jump/CJump/Return 只可能出现在块的最后一条指令
 * 3. 块内指令顺序执行，没有分支进入或跳出
 */
QuadProgram *blocking(QuadProgram *program);

#endif // __QUAD_BLOCKING_HH