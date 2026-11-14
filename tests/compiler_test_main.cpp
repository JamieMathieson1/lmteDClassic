#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../header/optimizer.h"
#include "../header/Tree.h"

// Parser header names vary between projects. Support the common casing.
#if __has_include("Parser.h")
    #include "Parser.h"
#elif __has_include("parser.h")
    #include "parser.h"
#else
    #error "Could not find Parser.h or parser.h. Add your parser header to the include path."
#endif

using namespace std;

static string readFile(const string& path) {
    ifstream input(path);
    if (!input) {
        throw runtime_error("Could not open input file: " + path);
    }

    ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

static void writeFile(const string& path, const string& contents) {
    ofstream output(path);
    if (!output) {
        throw runtime_error("Could not create output file: " + path);
    }

    output << contents;
    if (!output) {
        throw runtime_error("Failed while writing output file: " + path);
    }
}

int main(int argc, char* argv[]) {
    const string inputPath = (argc >= 2)
        ? argv[1]
        : "tests/optimizer_pipeline.dc";

    const string outputPath = (argc >= 3)
        ? argv[2]
        : "tests/generated_cpp.txt";

    try {
        // 1. Read the D Classic source file.
        string source = readFile(inputPath);

        // 2. Parse the source into your existing AST.
        // Assumption: Parser accepts source text and exposes parse().
        // If your Parser constructor accepts tokens instead, only this
        // adapter section needs to be changed to match Parser.h.
        Tokenizer tokenizer(source);
        parser parsed(tokenizer);
        NodePtr ast = parsed.parse();

        if (!ast) {
            throw runtime_error("Parser returned an empty AST.");
        }

        if (!as<programNode>(ast.get())) {
            throw runtime_error("Parser did not return a programNode root.");
        }

        cout << "[OK] Parsed: " << inputPath << '\n';

        // 3. Run the optimiser: constant folding, loop unrolling,
        //    and dead code elimination. Constant propagation is excluded.
        Optimizer optimizer;
        optimizer.optimize(ast);

        cout << "[OK] Optimisation passes completed.\n";

        // 4. Generate C++ by visiting the optimised AST.
        CppGenerator generator;
        const string generatedCpp = ast->callVisit(generator);

        if (generatedCpp.empty()) {
            throw runtime_error("Code generator returned empty output.");
        }

        // 5. Save generated C++ into a .txt file as requested.
        writeFile(outputPath, generatedCpp);

        cout << "[OK] Generated C++ saved to: " << outputPath << '\n';
        cout << "\n========== GENERATED C++ ==========\n";
        cout << generatedCpp;
        if (generatedCpp.back() != '\n') {
            cout << '\n';
        }
        cout << "====================================\n";

        return 0;
    }
    catch (const exception& error) {
        cerr << "[ERROR] " << error.what() << '\n';
        return 1;
    }
}
