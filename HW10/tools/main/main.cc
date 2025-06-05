#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <filesystem>
#include <unistd.h>

#include "treep.hh"
#include "quad.hh"
#include "quad2xml.hh"
#include "xml2quad.hh"
#include "opt.hh"

using namespace std;
using namespace tree;
using namespace quad;
using namespace tinyxml2;

void exec(string file);
int main(int argc, const char* argv[])
{
    // 切换到test目录
    filesystem::path filePath(__FILE__);
    filesystem::path directory = filePath.parent_path();
    chdir(directory.c_str());
    chdir("../../test/input_example");

    vector<string> files;
    files = { "hw8/hw8test02" };

    // hw10测试文件
    // for (int i = 0; i <= 7; i++) {
    //     char file_name[100];
    //     sprintf(file_name, "hw10/hw10test%02d", i);
    //     files.push_back(string(file_name));
    // }

    // hw8测试文件
    // for (int i = 0; i <= 12; i++) {
    //     char file_name[100];
    //     sprintf(file_name, "hw8/hw8test%02d", i);
    //     files.push_back(string(file_name));
    // }

    for (auto file : files) {
        cout << " >" << file << endl;
        exec(file);
    }
}

void exec(string file)
{
    string file_quad_ssa_xml = file + ".4-ssa-xml.quad";
    string file_quad_ssa = file + ".4-ssa.txt";
    string file_quad_ssa_opt = file + ".4-zopt.txt";

    cout << "读取quad: " << file_quad_ssa_xml << endl;
    quad::QuadProgram* x3 = xml2quad(file_quad_ssa_xml.c_str());
    if (x3 == nullptr) {
        cerr << "Error reading Quad from xml: " << file_quad_ssa_xml << endl;
        exit(1);
    }

    quad2file(x3, file_quad_ssa, true);
    QuadProgram* x4 = optProg(x3);

    quad2file(x4, file_quad_ssa_opt.c_str(), true);
    cout << "-----Done----" << endl;
}