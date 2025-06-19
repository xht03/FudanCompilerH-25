#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <filesystem>
#include <unistd.h>

#include "ASTheader.hh"
#include "FDMJAST.hh"

#include "ast2xml.hh"
#include "xml2ast.hh"

#include "config.hh"
#include "temp.hh"
#include "treep.hh"
#include "namemaps.hh"
#include "semant.hh"

#include "canon.hh"
#include "quad.hh"
#include "quadssa.hh"
#include "blocking.hh"

#include "opt.hh"

#include "prepareregalloc.hh"
#include "regalloc.hh"

#include "flowinfo.hh"
#include "color.hh"
#include "quad2rpi.hh"

#include "ast2tree.hh"
#include "tree2xml.hh"
#include "xml2tree.hh"
#include "tree2quad.hh"

#include "xml2quad.hh"
#include "quad2xml.hh"

using namespace std;
using namespace fdmj;
using namespace tree;
using namespace tinyxml2;

#define with_location_info false
XMLDocument* x;

int main(int argc, const char *argv[]) {

    // 切换到 test 目录
    filesystem::path filePath(__FILE__);
    filesystem::path directory = filePath.parent_path();
    chdir(directory.c_str());
    chdir("../../test");

    string file;
    // string dir = "";
    // file = dir + argv[argc - 1];
    string dir = "HW5/";
    file = dir + "test0";

    string file_fmj = file + ".fmj";
    string file_ast_semant = file + ".2-semant.ast";
    string file_irp = file + ".3.irp";
    string file_quad_xml = file + ".4-xml.quad";
    string file_quad_block = file + ".4-block.quad";
    string file_quad_ssa_xml = file + ".4-ssa-xml.quad";
    string file_quad_ssa_opt_xml = file + ".4-ssa-opt-xml.quad";
    string file_quad_prepared_xml = file + ".4-prepared-xml.quad";
    string file_quad_color_xml = file + ".4-xml.clr";
    string file_rpi = file + ".s";

    // FMJ -> AST
    cout << "读取FMJ文件:" << file_fmj << endl;
    ifstream fmjfile(file_fmj);
    fdmj::Program* root = fdmj::fdmjParser(fmjfile, false);
    if (root == nullptr) {
        cout << "FMJ->AST失败" << endl;
        return EXIT_FAILURE;
    }

    // semantic analysis
    AST_Semant_Map* semant_map = semant_analyze(root);
    x = ast2xml(root, semant_map, with_location_info, true);
    if (x->Error()) {
        cout << "AST无效" << endl;
        return EXIT_FAILURE;
    }
    x->SaveFile(file_ast_semant.c_str());
    cout << "写入:" << file_ast_semant << endl;

    // AST-semant -> IR
    tree::Program* ir = ast2tree(root, semant_map);
    x = tree2xml(ir);
    x->SaveFile(file_irp.c_str());
    cout << "写入:" << file_irp << endl;

    // IR -> Quad
    tree::Program* ir_canon = canon(ir);    // canonicalize IR
    QuadProgram* qd = tree2quad(ir_canon);
    if (qd == nullptr) {
        cout << "IR -> Quad失败" << endl;
        return EXIT_FAILURE;
    }
    if (!quad2xml(qd, file_quad_xml.c_str())) {
        cout << "写入Quad-xml失败" << endl;
        return EXIT_FAILURE;
    }
    cout << "写入:" << file_quad_xml << endl;

    // QUAD -> QUAD-SSA
    QuadProgram* qd_block = blocking(qd);
    QuadProgram* qd_ssa = quad2ssa(qd_block);
    if (qd_ssa == nullptr) {
        cout << "Quad -> Quad-SSA失败" << endl;
        return EXIT_FAILURE;
    }
    if (!quad2xml(qd_ssa, file_quad_ssa_xml.c_str())) {
        cout << "写入Quad-SSA-xml失败" << endl;
        return EXIT_FAILURE;
    }
    cout << "写入:" << file_quad_ssa_xml << endl;

    // QUAD-SSA -> QUAD-OPT
    quad::QuadProgram* qd_ssa_ = xml2quad(file_quad_ssa_xml.c_str());
    if (qd_ssa_ == nullptr) {
        cout << "读取Quad-SSA失败" << endl;
        return EXIT_FAILURE;
    }
    quad::QuadProgram* qd_opt = optProg(qd_ssa_);
    if (qd_opt == nullptr) {
        cout << "Quad-SSA -> Quad-OPT失败" << endl;
        return EXIT_FAILURE;
    }
    if (!quad2xml(qd_opt, file_quad_ssa_opt_xml.c_str())) {
        cout << "写入Quad-OPT-xml失败" << endl;
        return EXIT_FAILURE;
    }
    cout << "写入:" << file_quad_ssa_opt_xml << endl;

    // register allocation
    quad::QuadProgram* qd_opt_ = xml2quad(file_quad_ssa_opt_xml.c_str());
    if (qd_opt_ == nullptr) {
        cout << "读取Quad-OPT失败" << endl;
        return EXIT_FAILURE;
    }

    int number_of_colors = 9;
    bool print_ig = true;

    quad::QuadProgram* qd_prepared = prepareRegAlloc(qd_opt_);
    XMLDocument* x_color = coloring(qd_prepared, number_of_colors, print_ig);

    if (!quad2xml(qd_prepared, file_quad_prepared_xml.c_str())) {
        cout << "写入Quad-Prepared-xml失败" << endl;
        return EXIT_FAILURE;
    }

    x_color->SaveFile(file_quad_color_xml.c_str());
    cout << "写入:" << file_quad_color_xml << endl;

    // QUAD -> assembly
    quad::QuadProgram* qd_prepared_ = xml2quad(file_quad_prepared_xml.c_str());
    if (qd_prepared_ == nullptr) {
        cout << "读取Quad-Prepared失败" << endl;
        return EXIT_FAILURE;
    }

    ColorMap* colormap = xml2colormap(file_quad_color_xml);
    quad2rpi(qd_prepared_, colormap, file_rpi);

    cout << "-----Done---" << endl;
    return EXIT_SUCCESS;
}
