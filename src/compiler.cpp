#include "../header/Compiler.h"

#include <iostream>

int main(int argc, char* argv[]) {
    // runs the compiler class on an input and output directory
    const std::string inputPath = argc > 1 ? argv[1] : " tests/compilerIntegrationTests/input/basics.dc";
    const std::string outputPath = argc > 2 ? argv[2] : "output/generated.cpp";

    try {
        return Compiler::run(inputPath, outputPath);
    } catch (const std::exception& error) {
        std::cerr << "Compiler error: " << error.what() << '\n';
        return 1;
    }
}

