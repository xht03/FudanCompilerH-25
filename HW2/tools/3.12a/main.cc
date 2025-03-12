#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

enum TokenType { S, E, ID, PLUS, LPAREN, RPAREN, END };

int parse_file(const std::string& filename)
{
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Could not open the file!" << endl;
        return -1;
    }

    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    char* buffer = new char[size + 1];
    if (!file.read(buffer, size)) {
        cerr << "Could not read the file!" << endl;
        return -1;
    }
    buffer[size] = '\0';

    vector<TokenType> tokens;
    for (int i = 0; i < size; i++) {
        switch (buffer[i]) {
            case 'i':
                tokens.push_back(ID);
                break;
            case 'd':
                break;
            case '+':
                tokens.push_back(PLUS);
                break;
            case '(':
                tokens.push_back(LPAREN);
                break;
            case ')':
                tokens.push_back(RPAREN);
                break;
            case '$':
                tokens.push_back(END);
                break;
            default:
                cerr << "Invalid character in the file!" << endl;
                return -1;
        }
    }

    


    file.close();
    delete[] buffer;
    
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <filename>" << endl;
        return 1;
    }

    string filename = argv[1];
    int result = parse_file(filename);

    cout << "The result is: " << result << endl;

    return 0;
}