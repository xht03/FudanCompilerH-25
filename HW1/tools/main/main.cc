#include "ASTheader.hh"
#include "FDMJAST.hh"
#include "MinusIntConverter.hh"
#include "constantPropagation.hh"
#include "executor.hh"
#include "ast2xml.hh"
#include "xml2ast.hh"
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;
using namespace fdmj;
using namespace tinyxml2;

#define with_location_info false
// false means no location info in the AST XML files

Program* prog();

int main(int argc, const char* argv[])
{
    string file;

    chdir("../../../test");
    file = "test4";

    // const bool debug = argc > 1 && std::strcmp(argv[1], "--debug") == 0;

    // if ((!debug && argc != 2) || (debug && argc != 3)) {
    //   cerr << "Usage: " << argv[0] << " [--debug] filename" << endl;
    //   return EXIT_FAILURE;
    // }
    // file = argv[argc - 1];

    // boilerplate output filenames (used throutghout the compiler pipeline)
    string file_fmj = file + ".fmj";            // input source file
    string file_out = file + ".out";

    string file_src = file + ".src";            // source file
    string file_src2 = file + ".1.debug.src";   // debug source file

    string file_ast = file + ".2.ast";          // ast in xml
    string file_ast2 = file + ".2-debug.ast";   // debug ast in xml

    string file_ast3 = file + ".2-debug3.ast";  // minus int converter
    string file_ast4 = file + ".2-debug4.ast";  // constant propagation
    string file_irp = file + ".3.irp";
    string file_stm = file + ".4.stm";
    string file_liv = file + ".5.liv";

    // --- parse source file, generate AST(root), and save it to XML(file_ast) ---

    cout << "------Parsing fmj source file: " << file_fmj << "------------" << endl;
    std::ifstream fmjfile(file_fmj);
    Program* root = fdmjParser(fmjfile, false); // false means no debug info from parser
    if (root == nullptr) {
        std::cout << "AST is not valid!" << endl;
        return EXIT_FAILURE;
    }

    cout << "Convert AST  to XML..." << endl;
    XMLDocument* x = ast2xml(root, with_location_info);
    delete root;
    if (x->Error()) {
        std::cout << "AST is not valid!" << endl;
        return EXIT_FAILURE;
    }

    cout << "Saving AST (XML) to: " << file_ast << endl;
    x->SaveFile(file_ast.c_str());

    // --- clone a debug AST, save it to XML(file_ast) ---

    cout << "Loading AST (XML) from: " << file_ast << endl;
    delete x;
    x = new XMLDocument();
    x->LoadFile(file_ast.c_str());
    Program* root2 = xml2ast(x->RootElement());
    if (root2 == nullptr) {
        std::cout << "AST is not valid!" << endl;
        return EXIT_FAILURE;
    }
    XMLDocument* y = ast2xml(root2, with_location_info);
    delete root2;

    cout << "Saving AST (XML) to: " << file_ast2 << endl;
    y->SaveFile(file_ast2.c_str());


    // --- load debug AST, clone it for test ---

    cout << "Loading AST (XML) from: " << file_ast2 << endl;
    XMLDocument *z = new XMLDocument();
    z->LoadFile(file_ast2.c_str());
    root2 = xml2ast(z->RootElement());

    cout << "Clone debug AST ..." << endl;
    Program* root3 = root2->clone();
    delete root2;

    // --- minus int converter ---  

    cout << "minIntConverter Rewriting AST..." << endl;
    Program* root4 = minusIntRewrite(root3);
    delete root3;

    cout << "Convert rewrote AST to XML..." << endl;
    XMLDocument *w = ast2xml(root4, with_location_info);

    cout << "Saving AST (XML) to: " << file_ast4 << endl;
    w->SaveFile(file_ast3.c_str());

    // --- constant propagation ---

    cout << "constantPropagation Rewriting AST..." << endl;
    Program* root5 = constantPropagate(root4);
    delete root4;

    cout << "Convert rewrote AST to XML..." << endl;
    w = ast2xml(root5, with_location_info);

    cout << "Saving AST (XML) to: " << file_ast4 << endl;
    w->SaveFile(file_ast4.c_str());

    // --- executor ---

    cout << "Executor Rewriting AST..." << endl;
    int ret = execute(root5);
    delete root5;

    cout << "Return value: " << ret << endl;
    cout << "-----Done---" << endl;
    return EXIT_SUCCESS;
}