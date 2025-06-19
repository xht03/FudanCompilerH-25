#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <filesystem>
#include <unistd.h>

#include "config.hh"

#include "FDMJAST.hh"
#include "ast2xml.hh"
#include "xml2ast.hh"

#include "temp.hh"
#include "treep.hh"
#include "xml2tree.hh"
#include "tree2xml.hh"

#include "quad.hh"
#include "xml2quad.hh"
#include "quad2xml.hh"

// .fmj -> .2.ast
#include "ASTheader.hh"

// .2.ast -> .2-semant.ast
#include "namemaps.hh"
#include "semant.hh"

// TODO: 2-semant.ast -> .3.irp
#include "ast2tree.hh"

// .3.irp -> .3-canon.irp
#include "canon.hh"

// .3-canon.irp -> .4.quad
#include "tree2quad.hh"

// .4.quad -> .4-block.quad -> .4-ssa.quad
#include "blocking.hh"
#include "quadssa.hh"

// .4-ssa.quad -> .4-ssa-opt.quad
#include "opt.hh"

// .4-ssa-opt.quad -> .4-prepared.quad .4-xml.clr
#include "prepareregalloc.hh"
#include "regalloc.hh"

//
#include "color.hh"
#include "quad2rpi.hh"

using namespace std;
using namespace fdmj;
using namespace tree;
using namespace tinyxml2;

#define with_location_info false
XMLDocument* x;

int main(int argc, const char* argv[])
{
    // 切换到test目录
    filesystem::path filePath(__FILE__);
    filesystem::path directory = filePath.parent_path();
    chdir(directory.c_str());
    chdir("../../test");

    string file;
    file = argv[argc - 1];
    // file = "hw8test06";

    string file_fmj = file + ".fmj";
    string file_ast = file + ".2.ast";

    string file_irp = file + ".3.irp";
    string file_irp_canon = file + ".3.irp-canon";

    string file_quad = file + ".4.quad";
    string file_quad_xml = file + ".4-xml.quad";
    string file_quad_block = file + ".4-block.quad";
    string file_quad_ssa = file + ".4-ssa.quad";
    string file_quad_ssa_opt_xml = file + ".4-ssa-opt-xml.quad";

    string file_quad_prepared = file + ".4-prepared.quad";
    string file_quad_color_xml = file + ".4-xml.clr";

    string file_rpi = file + ".4.s";

    cout << "读取: " << file_fmj << endl;
    ifstream fmjfile(file_fmj);
    fdmj::Program* root = fdmj::fdmjParser(fmjfile, false);
    if (root == nullptr) {
        cout << "AST无效" << endl;
        return EXIT_FAILURE;
    }

    cout << "写入: " << file_ast << endl;
    AST_Semant_Map* semant_map = semant_analyze(root);
    // semant_map->getNameMaps()->print();
    x = ast2xml(root, semant_map, with_location_info, true);
    if (x->Error()) {
        cout << "AST无效" << endl;
        return EXIT_FAILURE;
    }
    x->SaveFile(file_ast.c_str());

    cout << "写入: " << file_irp << endl;
    tree::Program* ir = ast2tree(root, semant_map);
    x = tree2xml(ir);
    x->SaveFile(file_irp.c_str());

    // ----------------------------------------------------------------

    cout << "写入: " << file_irp_canon << endl;
    tree::Program* ir_canon = canon(ir);
    XMLDocument* doc = tree2xml(ir_canon);
    doc->SaveFile(file_irp_canon.c_str());

    cout << "写入: " << file_quad << endl;
    QuadProgram* qd = tree2quad(ir_canon);
    if (qd == nullptr) {
        cerr << "Error converting IR to Quad" << endl;
        return EXIT_FAILURE;
    }

    string temp_str;
    temp_str.reserve(50000);
    qd->print(temp_str, 0, true);
    ofstream qo(file_quad);
    if (!qo) {
        cerr << "Error opening file: " << file_quad << endl;
        return EXIT_FAILURE;
    }
    qo << temp_str;
    qo.flush();
    qo.close();

    ofstream out("/dev/tty");

    // ----------------------------------------------------------------

    // 需要重新写入并读取quad (摆脱Temp的指针同一，用于重命名)
    cout << "写入: " << file_quad_xml << endl;
    quad2xml(qd, file_quad_xml.c_str());

    cout << "读取: " << file_quad_xml << endl;
    quad::QuadProgram* x3 = xml2quad(file_quad_xml.c_str());

    // ----------------------------------------------------------------

    cout << "写入: " << file_quad_block << endl;
    QuadProgram* x4 = blocking(x3);
    temp_str.clear();
    temp_str.reserve(100000);
    ofstream out_block(file_quad_block);
    if (!out_block) {
        cerr << "Error opening file: " << file_quad_block << endl;
        return EXIT_FAILURE;
    }
    x4->print(temp_str, 0, true);
    out_block << temp_str;
    out_block.flush();
    out_block.close();

    cout << "写入: " << file_quad_ssa << endl;
    QuadProgram* x5 = quad2ssa(x4);
    if (x5 == nullptr) {
        cerr << "Error converting Quad to Quad-SSA" << endl;
        return EXIT_FAILURE;
    }
    ofstream out_ssa(file_quad_ssa);
    if (!out_ssa) {
        cerr << "Error opening file: " << file_quad_ssa << endl;
        return EXIT_FAILURE;
    }
    temp_str.clear();
    temp_str.reserve(10000);
    x5->print(temp_str, 0, true);
    out_ssa << temp_str;
    out_ssa.flush();
    out_ssa.close();

    // ----------------------------------------------------------------

    cout << "写入: " << file_quad_ssa_opt_xml << endl;
    QuadProgram* x6 = optProg(x5);
    quad2xml(x6, file_quad_ssa_opt_xml.c_str());

    // ----------------------------------------------------------------
    
    int number_of_colors = 9;

    cout << "写入: " << file_quad_prepared << endl;
    QuadProgram* x7 = xml2quad(file_quad_ssa_opt_xml.c_str());
    QuadProgram* x8 = prepareRegAlloc(x7);
    quad2file(x8, file_quad_prepared.c_str(), true);

    cout << "写入: " << file_quad_color_xml << endl;
    XMLDocument* x9 = coloring(x8, number_of_colors, false);
    x9->SaveFile(file_quad_color_xml.c_str());

    // ----------------------------------------------------------------

    cout << "写入: " << file_rpi << endl;
    ColorMap* colormap = xml2colormap(file_quad_color_xml);
    quad2rpi(x8, colormap, file_rpi);

    // ----------------------------------------------------------------

    cout << "-----Done---" << endl << endl;
    return EXIT_SUCCESS;
}
