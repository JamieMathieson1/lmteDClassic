#include "test_framework.h"

#include "../../header/CppGenerator.h"

// helper functions to create mock objects
static Token token(const std::string& text) {
  return Token(text, TokenType::LITERAL, 1, 1);
}

static NodePtr literal(const std::string& text) {
  auto node = std::make_unique<LiteralNode>(token(text));
  node->text = text;
  return node;
}

static NodePtr functionReturning(const std::string& name, NodePtr value) {
  auto function = std::make_unique<FunctionNode>(token("F_int_" + name));
  function->id = Ident::decode("F_int_" + name);
  function->body = std::make_unique<BlockNode>(token("d"));
  auto ret = std::make_unique<ReturnNode>(token("return"));
  ret->value = std::move(value);
  as<BlockNode>(function->body.get())->stmts.push_back(std::move(ret));
  return function;
}

TEST_CASE("code generator emits a function and return") {
  programNode program;
  program.items.push_back(functionReturning("main", literal("1")));
  CppGenerator generator;
  generator.generate(program);
  REQUIRE_EQ(generator.getCode(), std::string("int main() {\n    return 1;\n}"));
}

TEST_CASE("code generator emits binary expressions with grouping") {
  auto expression = std::make_unique<BinaryNode>(token("+"));
  expression->lhs = literal("1");
  auto rhs = std::make_unique<BinaryNode>(token("*"));
  rhs->lhs = literal("2");
  rhs->rhs = literal("3");
  expression->rhs = std::move(rhs);
  programNode program;
  program.items.push_back(functionReturning("main", std::move(expression)));
  CppGenerator generator;
  generator.generate(program);
  REQUIRE(generator.getCode().find("return 1 + 2 * 3;") != std::string::npos);
}

TEST_CASE("code generator emits classes with public members") {
  auto cls = std::make_unique<ClassNode>(token("C_Box"));
  cls->id = Ident::decode("C_Box");
  auto memberStatement = std::make_unique<ExprStmtNode>(token("/"));
  auto member = std::make_unique<NameNode>(token("V_int_value"));
  member->id = Ident::decode("V_int_value");
  member->isDecl = true;
  memberStatement->expr = std::move(member);
  cls->members.push_back(std::move(memberStatement));
  programNode program;
  program.items.push_back(std::move(cls));
  CppGenerator generator;
  generator.generate(program);
  REQUIRE(generator.getCode().find("class Box") != std::string::npos);
  REQUIRE(generator.getCode().find("int value;") != std::string::npos);
}

