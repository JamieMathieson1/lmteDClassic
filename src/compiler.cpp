#include "../header/Compiler.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "../header/CppGenerator.h"
#include "../header/Optimizer.h"
#include "../header/SemanticAnalyser.h"
#include "../header/Tree.h"
#include "../header/Tokenizer.h"
#include "../header/Parser.h"

namespace {

std::string readFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Could not open input file: " + path);
    }

    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

void writeFile(const std::string& path, const std::string& contents) {
    const std::filesystem::path outputPath(path);
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Could not create output file: " + path);
    }

    output << contents;
    if (!output) {
        throw std::runtime_error("Failed while writing output file: " + path);
    }
}

}

int Compiler::run(const std::string& inputPath, const std::string& outputPath) {
    std::string source = readFile(inputPath);
    Tokenizer tokenizer(source);
    Parser parsed(tokenizer);
    NodePtr ast = parsed.parse();
    auto* program = as<programNode>(ast.get());
    if (!program) {
        throw std::runtime_error("Parser did not return a programNode.");
    }

    SemanticAnalyser semantic;
    if (!semantic.analyse(*program)) {
        std::ostringstream errors;
        for (const std::string& error : semantic.getErrors()) {
            errors << error << '\n';
        }
        throw std::runtime_error(errors.str());
    }

    Optimizer optimizer;
    optimizer.optimize(ast);

    CppGenerator generator;
    program = as<programNode>(ast.get());
    if (!program) {
        throw std::runtime_error("Optimizer did not preserve a programNode.");
    }
    generator.generate(*program);
    if (generator.getCode().empty()) {
        throw std::runtime_error("Code generator returned empty output.");
    }

    writeFile(outputPath, generator.getCode());
    std::cout << "Generated C++: " << outputPath << '\n';
    return 0;
}

int main(int argc, char* argv[]) {
    const std::string inputPath = argc > 1 ? argv[1] : "tests/optimizer_pipeline.dc";
    const std::string outputPath = argc > 2 ? argv[2] : "output/generated.cpp";

    try {
        return Compiler::run(inputPath, outputPath);
    } catch (const std::exception& error) {
        std::cerr << "Compiler error: " << error.what() << '\n';
        return 1;
    }
}
