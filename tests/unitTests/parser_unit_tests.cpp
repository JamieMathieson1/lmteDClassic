#include "test_framework.h"

#include "../../header/Parser.h"

static NodePtr parseSource(const std::string& source) {
  std::string input = source;
  Tokenizer tokenizer(input);
  Parser parser(tokenizer);
  return parser.parse();
}

TEST_CASE("parser builds a function with a return statement") {
  NodePtr ast = parseSource("F_int_main[void]d return 1/b");
  auto* program = as<programNode>(ast.get());
  REQUIRE(program != nullptr);
  REQUIRE_EQ(program->items.size(), size_t{1});
  auto* function = as<FunctionNode>(program->items[0].get());
  REQUIRE(function != nullptr);
  REQUIRE_EQ(function->id.name, std::string("main"));
  auto* body = as<BlockNode>(function->body.get());
  REQUIRE(body != nullptr);
  REQUIRE_EQ(body->stmts.size(), size_t{1});
  REQUIRE(as<ReturnNode>(body->stmts[0].get()) != nullptr);
}

TEST_CASE("parser respects arithmetic precedence") {
  NodePtr ast = parseSource("F_int_main[void]d return 1 + 2 * 3/b");
  auto* function = as<FunctionNode>(as<programNode>(ast.get())->items[0].get());
  auto* expression =
      as<BinaryNode>(as<ReturnNode>(
                           as<BlockNode>(function->body.get())->stmts[0].get())
                           ->value.get());
  REQUIRE(expression != nullptr);
  REQUIRE_EQ(expression->op, std::string("+"));
  REQUIRE_EQ(as<BinaryNode>(expression->rhs.get())->op, std::string("*"));
}

TEST_CASE("parser builds conditional and loop nodes") {
  NodePtr ast = parseSource(
      "F_int_main[void]d if[True]d return 1/b "
      "tested[True]d return 2/b return 3/b");
  auto* body = as<BlockNode>(as<FunctionNode>(
                                  as<programNode>(ast.get())->items[0].get())
                                  ->body.get());
  REQUIRE_EQ(body->stmts.size(), size_t{3});
  REQUIRE(as<IfNode>(body->stmts[0].get()) != nullptr);
  REQUIRE(as<WhileNode>(body->stmts[1].get()) != nullptr);
  REQUIRE(as<ReturnNode>(body->stmts[2].get()) != nullptr);
}

TEST_CASE("parser reports malformed blocks") {
  bool threw = false;
  try {
    parseSource("F_int_main[void]d return 1/");
  } catch (const parseError&) {
    threw = true;
  }
  REQUIRE(threw);
}

