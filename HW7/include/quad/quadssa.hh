#ifndef __QUAD_SSA_HH
#define __QUAD_SSA_HH

#include <map>
#include <set>
#include <string>
#include <vector>
#include "temp.hh"
#include "treep.hh"
#include "quad.hh"

using namespace std;
using namespace quad;

class VersionedTemp {
public:

// 这里我们采用一种较简单的方法生成 Temps with versions
// 假设是版本数量较少 <= 100
// 更完善的实现可通过记录每个新版本的编号，并使用 map 关联原始变量编号。

static int versionedTempNum(int old_num, int version) {
    return old_num*100+version; //unique temp for each version
}

static int origTempNum(int versionedTempNum) {
    if (versionedTempNum >= 10000)
        return versionedTempNum/100;
    else
        return versionedTempNum; 
}

};

QuadProgram* quad2ssa(QuadProgram* program);

#endif // __QUAD_SSA_HH
