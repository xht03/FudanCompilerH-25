#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include "treep.hh"
#include "quad.hh"
#include "quad2xml.hh"
#include "xml2quad.hh"
#include "prepareregalloc.hh"
#include "regalloc.hh"

using namespace std;
using namespace tree;
using namespace quad;
using namespace tinyxml2;

int main(int argc, const char *argv[]) {
    string file;

    int number_of_colors = 9; //default 9: r0-r8
    const bool noc = argc > 1 && std::strcmp(argv[1], "-k") == 0;

    if ((!noc && argc != 2) || (noc && argc != 4)) {
        cerr << "Usage: " << argv[0] << " [-k #colors] filename" << endl;
        return EXIT_FAILURE;
    }
    file = argv[argc - 1];
    if (argc == 4) number_of_colors = stoi(argv[2]);
    cout << "Number of colors to use = " << number_of_colors << endl;

    // boilerplate output filenames (used throughout the compiler pipeline)
    string file_quad_ssa_xml = file + ".4-ssa-xml.quad";
    //string file_quad_ssa = file + ".4-ssa-back.quad";
    string file_quad_prepared = file + ".4-prepared.quad";
    string file_quad_prepared_xml = file + ".4-prepared-xml.quad";
    string file_quad_color_xml = file + ".4-xml.clr";

    cout << "Reading Quad from xml: " << file_quad_ssa_xml << endl;
    quad::QuadProgram *x3 = xml2quad(file_quad_ssa_xml.c_str());
    if (x3 == nullptr) {
        cerr << "Error reading Quad from xml: " << file_quad_ssa_xml << endl;
        return EXIT_FAILURE;
    }
    //cout << "Writing is back (text) to: " << file_quad_ssa << endl;
    //quad2file(x3, file_quad_ssa, true);
    //cout << "Prepared for register allocation..." << endl;
    QuadProgram *x4 = prepareRegAlloc(x3); //using registers to prepare the quad for RPI
    cout << "Writing the prepared Quad Program to: " <<  file_quad_prepared << endl;
    quad2file(x4, file_quad_prepared.c_str(), true); //write the prepared quad to a file
    //cout << "Writing the prepared Quad Program (XML) to: " <<  file_quad_prepared_xml << endl;
    //quad2xml(x4, file_quad_prepared_xml.c_str());
    cout << "Coloring: " << file_quad_prepared << endl;
    bool print_ig = true;
    XMLDocument *x5 = coloring(x4, number_of_colors, print_ig); //coloring the quad program, print IGs along the way
    cout << "Writing regAlloc'ed (colors XML) to " << file_quad_color_xml << endl;
    x5->SaveFile(file_quad_color_xml.c_str());
    cout << "-----Done---" << endl;
    return EXIT_SUCCESS;
}