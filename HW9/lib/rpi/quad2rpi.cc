#define DEBUG
#undef DEBUG

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "quad.hh"
#include "flowinfo.hh"
#include "color.hh"
#include "quad2rpi.hh"

static string current_funcname = "";

//This is to convert the names of the functions to a format that is acceptable by the assembler
string normalizeName(string name) {
    // Normalize the name by replacing special characters with underscores
    if (name == "_^main^_^main") { //speial case for main
        return "main";
    }
    for (char& c : name) {
        if (!isalnum(c)) {
            c = '$';
        }
    }
    return name;
}

bool rpi_isMachineReg(int n) {
    // Check if a node is a machine register
    return (n >= 0 && n <= 15);
}

string term2str(QuadTerm *term, Color *color) {
    string result;
    if (term->kind == QuadTermKind::TEMP) {
        Temp *t = term->get_temp()->temp;
        result = "r" + to_string(color->color_of(t->num));
    } else if (term->kind == QuadTermKind::CONST) {
        result = "#" + to_string(term->get_const());
    } else if (term->kind == QuadTermKind::MAME) {
        result = "@" + term->get_name();
    } else {
        cerr << "Error: Unknown term kind" << endl;
        exit(EXIT_FAILURE);
    }
    return result;
}

//Always use function name to prefix a label
//Note that you should do the same for Jump and CJump labels
string convert(QuadLabel* label, Color *c, int indent) {
#ifdef DEBUG
    cout << "In convert Label" << endl;
#endif
    string result; 
    result = current_funcname + "$" + label->label->str() + ": \n";
    return result;
}

/*************************************************** */
/*************************************************** */
/*************************************************** */
/*************************************************** */
/*************************************************** */
/*************************************************** */

string convert(QuadFuncDecl* func, DataFlowInfo *dfi, Color *color, int indent) {
    string result; 
    current_funcname = func->funcname; //set the global variable
    /**** This is where you need your code *** */
    /**** This is where you need your code *** */
    /**** This is where you need your code *** */
    /**** This is where you need your code *** */
    /**** This is where you need your code *** */
    /**** This is where you need your code *** */
    /**** This is where you need your code *** */
    /**** This is where you need your code *** */
    /**** This is where you need your code *** */
    /**** This is where you need your code *** */
    /**** This is where you need your code *** */
    /**** This is where you need your code *** */
    /**** This is where you need your code *** */
    /**** This is where you need your code *** */
    // Iterate through Quads in the function and convert
    // one statement at a time (sometimes two statemetns can be combined into one, try that!)
    // 1) Set up the frame stack correctly at beginning of the function, and return
    // 2) Replace temp with register using the color map
    // 3) Take care of the spilled temps
    // 4) and other details
    return result;
}

string quad2rpi(QuadProgram* quadProgram, ColorMap *cm) {// Convert a QuadProgram to RPI format with k registers
    string result; result.reserve(10000);
    // Iterate through the function declarations in the Quad program
    result = ".section .note.GNU-stack\n\n@ Here is the RPI code\n\n";
    for (QuadFuncDecl* func : *quadProgram->quadFuncDeclList) {
        //get the data flow info for the function
        DataFlowInfo *dfi = new DataFlowInfo(func);
        dfi->computeLiveness(); //liveness useful in some cases. Has to be done before trace otherwise this func code won't work!
        trace(func); //trace it (merge all blocks into one)
        current_funcname = func->funcname; //set the global variable
        //get the color for the function
        Color *c = cm->color_map[func->funcname]; 
        int indent = 8;
        result += convert(func, dfi, c, indent) + "\n";
    }
    //put the global functions at the end
    result += ".global malloc\n";
    result +=".global getint\n";
    result += ".global putint\n";
    result += ".global putch\n";
    result += ".global putarray\n";
    result += ".global getch\n";
    result += ".global getarray\n";
    result += ".global starttime\n";
    result += ".global stoptime\n";
    return result;
}

// Print the RPI code to the output file
void quad2rpi(QuadProgram* quadProgram, ColorMap *cm, string filename) {
    ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << quad2rpi(quadProgram, cm);
        outfile.flush();
        outfile.close();
    } else {
        cerr << "Error: Unable to open file " << filename << endl;
    }
}