#include "test_framework.h"

#include "../../header/SemanticAnalyser.h"

static Token token(const std::string& text) {
  return Token(text, TokenType::LITERAL, 1, 1);
}

static NodePtr name(const std::string& text, bool declaration = false) {
  auto node = std::make_unique<NameNode>(token(text));
  node->id = Ident::decode(text);
  node->isDecl = declaration;
  return node;
}

static NodePtr literal(const std::string& text, LitKind kind = LitKind::INT) {
  auto node = std::make_unique<LiteralNode>(token(text));
  node->text = text;
  node->litKind = kind;
  return node;
}

static NodePtr assignment(NodePtr target, NodePtr value,
                          const std::string& op = "=") {
  auto node = std::make_unique<AssignNode>(token(op));
  node->op = op;
  node->target = std::move(target);
  node->value = std::move(value);
  return node;
}

static programNode programWith(NodePtr statement) {
  programNode program;
  auto function = std::make_unique<FunctionNode>(token("F_int_main"));
  function->id = Ident::decode("F_int_main");
  function->body = std::make_unique<BlockNode>(token("d"));
  as<BlockNode>(function->body.get())->stmts.push_back(std::move(statement));
  program.items.push_back(std::move(function));
  return program;
}

TEST_CASE("semantic analyser accepts a matching declaration and return") {
  auto body = std::make_unique<BlockNode>(token("d"));
  body->stmts.push_back(
      std::make_unique<ExprStmtNode>(token("/")));
  auto* statement = as<ExprStmtNode>(body->stmts.back().get());
  statement->expr = assignment(name("V_int_value", true), literal("7"));
  auto ret = std::make_unique<ReturnNode>(token("return"));
  ret->value = name("V_int_value");
  body->stmts.push_back(std::move(ret));
  auto function = std::make_unique<FunctionNode>(token("F_int_main"));
  function->id = Ident::decode("F_int_main");
  function->body = std::move(body);
  programNode program;
  program.items.push_back(std::move(function));
  SemanticAnalyser analyser;
  REQUIRE(analyser.analyse(program));
  REQUIRE(!analyser.hasErrors());
}

TEST_CASE("semantic analyser rejects incompatible assignment") {
  programNode program;
  auto function = std::make_unique<FunctionNode>(token("F_int_main"));
  function->id = Ident::decode("F_int_main");
  auto body = std::make_unique<BlockNode>(token("d"));
  auto expr = std::make_unique<ExprStmtNode>(token("/"));
  expr->expr = assignment(name("V_int_value", true),
                          literal("False", LitKind::BOOL));
  body->stmts.push_back(std::move(expr));
  function->body = std::move(body);
  program.items.push_back(std::move(function));
  SemanticAnalyser analyser;
  REQUIRE(!analyser.analyse(program));
  REQUIRE(analyser.hasErrors());
}

TEST_CASE("semantic analyser rejects unknown variables") {
  programNode program = programWith(name("V_int_missing"));
  SemanticAnalyser analyser;
  REQUIRE(!analyser.analyse(program));
  REQUIRE(!analyser.getErrors().empty());
}

TEST_CASE("semantic analyser rejects non-boolean conditions") {
  auto ifNode = std::make_unique<IfNode>(token("if"));
  ifNode->cond = literal("1");
  ifNode->thenBlock = std::make_unique<BlockNode>(token("d"));
  programNode program = programWith(std::move(ifNode));
  SemanticAnalyser analyser;
  REQUIRE(!analyser.analyse(program));
  REQUIRE(analyser.getErrors()[0].find("boolean") != std::string::npos);
}

