#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "../header/CppGenerator.h"
#include "../header/SemanticAnalyser.h"
#include "../header/Tokenizer.h"
#include "../header/Tree.h"
#include "../header/parser.h"

int main() {
  // Class-free source code testing heap allocation ('beg') and deallocation
  // ('donate')
  std::string source = R"(
        F_int_main[] d
            V_int_ptr = beg 5/
            donate V_int_ptr/
            return 0/
        b
    )";

  std::cout << "--- INPUT SOURCE CODE ---\n" << source << "\n\n";

  try {
    Tokenizer tokenizer(source);
    parser p(tokenizer);
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

    std::cout << "--- GENERATED C++ CODE ---\n";
    std::cout << generator.getCode() << "\n";

  } catch (const std::exception& e) {
    std::cerr << "Exception caught during compilation: " << e.what() << '\n';
    return 1;
  }

  return 0;
}