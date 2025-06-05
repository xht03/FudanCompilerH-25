#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include "treep.hh"
#include "quad.hh"
#include "quad2xml.hh"
#include "xml2quad.hh"
#include "opt.hh"

using namespace std;
using namespace tree;
using namespace quad;
using namespace tinyxml2;

int main(int argc, const char *argv[]) {

    string file;
    int number_of_colors = 9; //vvdefault 9: r0-r8

    // file = argv[argc - 1];
    file = "/home/keats/FudanCompilerH2025/HW10/test/input_example/hw8test02";

    // boilerplate output filenames (used throughout the compiler pipeline)
    string file_quad_ssa_xml = file + ".4-ssa-xml.quad";
    string file_quad_ssa_opt = file + ".4-ssa-opt.quad";
    string file_quad_ssa_opt_xml = file + ".4-ssa-opt-xml.quad";

    cout << "Reading Quad from xml: " << file_quad_ssa_xml << endl;
    quad::QuadProgram *x3 = xml2quad(file_quad_ssa_xml.c_str());
    if (x3 == nullptr) {
        cerr << "Error reading Quad from xml: " << file_quad_ssa_xml << endl;
        return EXIT_FAILURE;
    }

    QuadProgram *x4 = optProg(x3); // using registers to prepare the quad for RPI

    cout << "Writing the optimized Quad Program to: " <<  file_quad_ssa_opt << endl;
    quad2file(x4, file_quad_ssa_opt.c_str(), true); //write the prepared quad to a file

    cout << "Writing the opt Program (XML) to: " <<  file_quad_ssa_opt_xml << endl;
    quad2xml(x4, file_quad_ssa_opt_xml.c_str());
    cout << "-----Done---" << endl;
    return EXIT_SUCCESS;
}