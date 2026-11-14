# include <iostream>
# include <string>
# include <memory>
# include <stdexcept>

# include "../../header/Tokenizer.h"
# include "../../header/parser.h"
# include "../../header/CppGenerator.h"
# include "../../header/SemanticAnalyser.h"
# include "../../header/Tree.h"

int main() {
    string source = R"( F_int_main[] d V_int_x = 5/ V_int_y = 10/ V_int_result = V_int_x + V_int_y/ return V_int_result/ b )";

    Tokenizer tokenizer(source);
    parser p(tokenizer);
    NodePtr ast = p.parse();
    auto* program = as<programNode>(ast.get());
    if (!program) {
        throw runtime_error("Invalid program AST");
    }
    SemanticAnalyser semantic;
    if (!semantic.analyse(*program)) {
        for (const string& err : semantic.getErrors()) {
            cout << err << '\n';
        }
        return 1;
    }
    CppGenerator generator;
    generator.generate(*program);
    std::cout << generator.getCode();
}