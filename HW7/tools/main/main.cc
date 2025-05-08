#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <filesystem>
#include <unistd.h>

#include "treep.hh"
#include "quad.hh"
#include "xml2quad.hh"
#include "quadssa.hh"
#include "blocking.hh"

using namespace std;
using namespace tree;
using namespace quad;
using namespace tinyxml2;

int main(int argc, const char *argv[]) {

    // 切换到 test 目录
    // filesystem::path filePath(__FILE__);
    // filesystem::path directory = filePath.parent_path();
    // chdir(directory.c_str());
    // chdir("../../test");

    string file;
    file = argv[argc - 1];
    // string dir = "HW4/";
    // file = dir + "hw4test05";

    string file_quad_xml = file + ".4-xml.quad";
    string file_quad = file + ".4.quad";
    string file_quad_block = file + ".4-block.quad";
    string file_quad_ssa = file + ".4-ssa.quad-my";

    cout << "Reading Quad from xml: " << file_quad_xml << endl;
    quad::QuadProgram *x3 = xml2quad(file_quad_xml.c_str());
    cout << "Writing is back (text) to: " << file_quad << endl;
    ofstream out2(file_quad);
    string temp_str;
    temp_str.clear(); 
    temp_str.reserve(100000);
    x3->print(temp_str, 0, true);
    out2 << temp_str;
    out2.flush(); 
    out2.close();

    cout << "Blocking the quad" << endl;
    QuadProgram *x4 = blocking(x3);
    temp_str.clear();
    temp_str.reserve(100000);
    ofstream out_block(file_quad_block);
    if (!out_block) {
        cerr << "Error opening file: " << file_quad_block << endl;
        return EXIT_FAILURE;
    }
    x4->print(temp_str, 0, true);
    out_block << temp_str;
    out_block.flush(); out_block.close();
    
    cout << "Converting Quad to Quad-SSA" << endl;
    QuadProgram *x5 = quad2ssa(x4);
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
    cout << "-----Done---" << endl;
    return EXIT_SUCCESS;
}