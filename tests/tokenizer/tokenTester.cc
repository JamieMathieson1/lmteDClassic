#include "../header/Tokenizer.h"
#include "../header/Utils.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <vector>

using namespace std;

// Usage: ./tokenTester [output file] [input files...]
// Defaults: writes ../tests/tokens.txt from ../tests/simple.dc and ../tests/large.dc

// reads a whole file into code, returns false if it can't be opened
bool readFile(const string& path, string& code) {
    ifstream f(path);
    if (!f) return false;
    string buf;
    while (getline(f, buf)) {
        code.push_back('\n');
        code.append(buf);
    }
    code = Utils::trim(code);
    return true;
}

// writes one file's tokens as a table
void dumpTokens(const string& path, ostream& out) {
    const string rule(64, '=');
    const string thin(64, '-');
    out << rule << "\n FILE: " << path << "\n" << rule << "\n";

    string code;
    if (!readFile(path, code)) {
        out << " ERROR: can't open file\n\n";
        return;
    }

    try {
        Tokenizer t(code);
        out << left
            << setw(6) << " #" << setw(12) << "LINE:CHAR" << setw(14) << "TYPE" << "CONTENT\n"
            << thin << "\n";

        int n = 0;
        while (t.canPoll()) {
            Token ct = t.poll();
            string pos = to_string(ct.getLine()) + ":" + to_string(ct.getCharNum());
            out << " " << setw(5) << n++ << setw(12) << pos
                << setw(14) << Utils::toString(ct.getType()) << ct.getContent() << "\n";
        }
        out << thin << "\n " << n << " tokens\n\n";
    }
    catch (const exception& e) {
        out << " ERROR: tokenizer threw: " << e.what() << "\n\n";
    }
}

int main(int argc, char* argv[])
{
    string outPath = argc > 1 ? argv[1] : "../tests/tokens.txt";
    vector<string> inputs;
    for (int i = 2; i < argc; i++) inputs.push_back(argv[i]);
    if (inputs.empty()) inputs = {"../tests/simple.dc", "../tests/large.dc"};

    ofstream out(outPath);
    if (!out) {
        cout << "Can't open output file " << outPath << "\n";
        return 1;
    }
    for (const string& in : inputs) dumpTokens(in, out);

    cout << "Wrote " << outPath << "\n";
    return 0;
}
