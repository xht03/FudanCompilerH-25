#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <filesystem>
#include <unistd.h>
#include "treep.hh"
#include "canon.hh"
#include "quad.hh"
#include "xml2tree.hh"
#include "tree2xml.hh"
#include "tree2quad.hh"

using namespace std;
using namespace tree;
using namespace quad;
using namespace tinyxml2;

int main(int argc, const char *argv[]) {
    // 切换到test目录
    // filesystem::path filePath(__FILE__);
    // filesystem::path directory = filePath.parent_path();
    // chdir(directory.c_str());
    // chdir("../../test");

    string file;
    string dir = "";
    file = dir + argv[argc - 1];
    // string dir = "HW4/";
    // file = dir + "hw4test09";

    string file_irp = file + ".3.irp";
    string file_irp_canon = file + ".3-canon.xml";
    string file_quad = file + ".4.quad-my";

    cout << "读取IR文件: " << file_irp << endl;
    tree::Program* ir = xml2tree(file_irp);
    if (ir == NULL) {
        cerr << "Error: " << file_irp << " not found or IR ill-formed" << endl;
        return EXIT_FAILURE;
    }

    cout << "规范化得到: " << file_irp_canon << endl;
    tree::Program* ir_canon = canon(ir);
    XMLDocument* doc = tree2xml(ir_canon);
    doc->SaveFile(file_irp_canon.c_str());

    cout << "将IR转换为Quad: " << file_quad << endl;
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
    out << "-----Done---" << endl;
    return EXIT_SUCCESS;
}