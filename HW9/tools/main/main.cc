#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <filesystem>
#include <unistd.h>

#include "treep.hh"
#include "quad.hh"
#include "xml2quad.hh"
#include "flowinfo.hh"
#include "color.hh"
#include "quad2rpi.hh"

using namespace std;
using namespace tree;
using namespace quad;
using namespace tinyxml2;

int main(int argc, const char *argv[]) {
    // 切换到test目录
    filesystem::path filePath(__FILE__);
    filesystem::path directory = filePath.parent_path();
    chdir(directory.c_str());
    chdir("../../test/input_example");

    string file;
    file = argv[argc - 1];
    // file = "hw8test06";

    string color_dir = "k5/"; // 选择着色数
    string file_quad_prepared_xml = file + ".4-prepared-xml.quad";
    string file_quad_ssa = file + ".4-prepared.txt";
    string file_quad_color_xml = color_dir + file + ".4-xml.clr";
    string file_rpi = file + ".my.s";

    cout << "读取quad: " << file_quad_prepared_xml << endl;
    quad::QuadProgram* x3 = xml2quad(file_quad_prepared_xml.c_str());
    if (x3 == nullptr) {
        cerr << "Error reading Quad from xml: " << file_quad_prepared_xml << endl;
        return EXIT_FAILURE;
    }
    quad2file(x3, file_quad_ssa, true);

    cout << "读取colors: " << file_quad_color_xml << endl;
    ColorMap* colormap = xml2colormap(file_quad_color_xml);
    // colormap->print();

    cout << "保存Rpi: " << file_rpi << endl;
    quad2rpi(x3, colormap, file_rpi);
    cout << "-----Done---" << endl;
    return EXIT_SUCCESS;
}