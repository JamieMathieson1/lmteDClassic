#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../header/CppGenerator.h"
#include "../header/SemanticAnalyser.h"
#include "../header/Tokenizer.h"
#include "../header/Tree.h"
#include "../header/Parser.h"

int main() {
  // Read source code from input.txt
  std::ifstream inputFile("src/input.txt");
  if (!inputFile.is_open()) {
    std::cerr << "Error: Could not open input.txt\n";
    return 1;
  }

  std::stringstream buffer;
  buffer << inputFile.rdbuf();
  std::string source = buffer.str();
  inputFile.close();

  std::cout << "--- INPUT SOURCE CODE ---\n" << source << "\n\n";

  try {
    Tokenizer tokenizer(source);
    Parser p(tokenizer);
    NodePtr ast = p.parse();

    auto* program = as<programNode>(ast.get());
    if (!program) {
      std::cerr << "Error: Invalid program AST\n";
      return 1;
    }

    // 1. Run Semantic Analysis
    SemanticAnalyser semantic;
    if (!semantic.analyse(*program)) {
      std::cout << "--- SEMANTIC ERRORS ---\n";
      for (const std::string& err : semantic.getErrors()) {
        std::cout << err << '\n';
      }
      return 1;
    }
    std::cout << "Semantic analysis passed successfully!\n\n";

    // 2. Run Code Generation
    CppGenerator generator;
    generator.scope_list =
        &semantic.getScopeList();  // Link scope list for isHeap checks
    generator.generate(*program);

    // 3. Write generated code to output.cpp
    std::ofstream outputFile("output.cpp");
    if (!outputFile.is_open()) {
      std::cerr << "Error: Could not open output.cpp for writing\n";
      return 1;
    }
    outputFile << "#include <bits/stdc++.h> \n";
    outputFile << generator.getCode();
    outputFile.close();

    std::cout << "Generated C++ code written to output.cpp\n";

  } catch (const std::exception& e) {
    std::cerr << "Exception caught during compilation: " << e.what() << '\n';
    return 1;
  }

  return 0;
}