#include "../../header/CppGenerator.h"
#include "../../header/Tokenizer.h"
#include "../../header/Parser.h"
#include <cassert>

int main() {
  std::string source =
      "FAH before function\n"
      "F_int_main[void] d FFAH body comment HAFF return 1/ b";
  Tokenizer tokenizer(source);
  Parser parsed(tokenizer);
  NodePtr tree = parsed.parse();

  CppGenerator generator;
  generator.generate(*static_cast<programNode*>(tree.get()));
  assert(generator.getCode() == "int main() {\n    return 1;\n}");
}
