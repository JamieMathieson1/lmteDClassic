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

static NodePtr expressionStatement(NodePtr expression) {
  auto statement = std::make_unique<ExprStmtNode>(token("/"));
  statement->expr = std::move(expression);
  return statement;
}

static NodePtr binary(const std::string& op, NodePtr lhs, NodePtr rhs) {
  auto node = std::make_unique<BinaryNode>(token(op));
  node->op = op;
  node->lhs = std::move(lhs);
  node->rhs = std::move(rhs);
  return node;
}

static NodePtr unary(const std::string& op, NodePtr operand) {
  auto node = std::make_unique<UnaryNode>(token(op));
  node->op = op;
  node->operand = std::move(operand);
  return node;
}

static NodePtr call(const std::string& function,
                    std::vector<NodePtr> args) {
  auto node = std::make_unique<CallNode>(token(function));
  node->callee = Ident::decode(function);
  node->args = std::move(args);
  return node;
}

static std::vector<NodePtr> one(NodePtr statement) {
  std::vector<NodePtr> statements;
  statements.push_back(std::move(statement));
  return statements;
}

static std::unique_ptr<FunctionNode> functionWith(
    const std::string& function, std::vector<NodePtr> params,
    std::vector<NodePtr> statements) {
  auto node = std::make_unique<FunctionNode>(token(function));
  node->id = Ident::decode(function);
  node->params = std::move(params);
  node->body = std::make_unique<BlockNode>(token("d"));
  auto* body = as<BlockNode>(node->body.get());
  for (auto& statement : statements) {
    body->stmts.push_back(std::move(statement));
  }
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

static programNode voidProgramWith(NodePtr statement) {
  programNode program;
  auto function = std::make_unique<FunctionNode>(token("F_void_main"));
  function->id = Ident::decode("F_void_main");
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

TEST_CASE("semantic analyser checks arithmetic and logical operand types") {
  programNode arithmetic =
      voidProgramWith(expressionStatement(binary("+", literal("1"),
                                                 literal("2", LitKind::FLOAT))));
  SemanticAnalyser arithmeticAnalyser;
  REQUIRE(arithmeticAnalyser.analyse(arithmetic));

  programNode invalidArithmetic = programWith(expressionStatement(
      binary("*", literal("True", LitKind::BOOL), literal("2"))));
  SemanticAnalyser invalidArithmeticAnalyser;
  REQUIRE(!invalidArithmeticAnalyser.analyse(invalidArithmetic));
  REQUIRE(invalidArithmeticAnalyser.getErrors()[0].find("arithmetic") !=
          std::string::npos);

  programNode invalidModulo = programWith(expressionStatement(
      binary("%", literal("1", LitKind::FLOAT), literal("2"))));
  SemanticAnalyser invalidModuloAnalyser;
  REQUIRE(!invalidModuloAnalyser.analyse(invalidModulo));

  programNode invalidLogical = programWith(expressionStatement(
      binary("&&", literal("1"), literal("2"))));
  SemanticAnalyser invalidLogicalAnalyser;
  REQUIRE(!invalidLogicalAnalyser.analyse(invalidLogical));
  REQUIRE(invalidLogicalAnalyser.getErrors()[0].find("boolean") !=
          std::string::npos);
}

TEST_CASE("semantic analyser checks unary operand types") {
  programNode valid = voidProgramWith(expressionStatement(
      unary("-", literal("2", LitKind::FLOAT))));
  SemanticAnalyser validAnalyser;
  REQUIRE(validAnalyser.analyse(valid));

  programNode invalid =
      programWith(expressionStatement(unary("!", literal("2"))));
  SemanticAnalyser invalidAnalyser;
  REQUIRE(!invalidAnalyser.analyse(invalid));
  REQUIRE(invalidAnalyser.getErrors()[0].find("boolean") !=
          std::string::npos);
}

TEST_CASE("semantic analyser enforces declaration order") {
  auto statements = std::vector<NodePtr>();
  statements.push_back(expressionStatement(
      assignment(name("V_int_value"), literal("1"))));
  statements.push_back(expressionStatement(
      assignment(name("V_int_value", true), literal("2"))));
  programNode program;
  program.items.push_back(
      functionWith("F_int_main", {}, std::move(statements)));

  SemanticAnalyser analyser;
  REQUIRE(!analyser.analyse(program));
  REQUIRE(analyser.getErrors()[0].find("Unknown or undeclared") !=
          std::string::npos);
}

TEST_CASE("semantic analyser validates function argument types and arity") {
  auto parameter = name("V_int_value", true);
  auto calleeReturn = std::make_unique<ReturnNode>(token("return"));
  calleeReturn->value = name("V_int_value");

  std::vector<NodePtr> wrongTypeArgs;
  wrongTypeArgs.push_back(literal("True", LitKind::BOOL));
  auto wrongTypeReturn = std::make_unique<ReturnNode>(token("return"));
  wrongTypeReturn->value = call("F_int_add", std::move(wrongTypeArgs));

  programNode wrongTypeProgram;
  wrongTypeProgram.items.push_back(functionWith(
      "F_int_add", one(std::move(parameter)), one(std::move(calleeReturn))));
  wrongTypeProgram.items.push_back(
      functionWith("F_int_main", {}, one(std::move(wrongTypeReturn))));
  SemanticAnalyser typeAnalyser;
  REQUIRE(!typeAnalyser.analyse(wrongTypeProgram));
  REQUIRE(typeAnalyser.getErrors()[0].find("type mismatch") !=
          std::string::npos);

  std::vector<NodePtr> wrongArityArgs;
  wrongArityArgs.push_back(literal("1"));
  auto wrongArityReturn = std::make_unique<ReturnNode>(token("return"));
  wrongArityReturn->value = call("F_int_add", std::move(wrongArityArgs));
  auto addReturn = std::make_unique<ReturnNode>(token("return"));
  addReturn->value = name("V_int_first");
  std::vector<NodePtr> twoParameters;
  twoParameters.push_back(name("V_int_first", true));
  twoParameters.push_back(name("V_int_second", true));

  programNode wrongArityProgram;
  wrongArityProgram.items.push_back(functionWith(
      "F_int_add", std::move(twoParameters), one(std::move(addReturn))));
  wrongArityProgram.items.push_back(
      functionWith("F_int_main", {}, one(std::move(wrongArityReturn))));
  SemanticAnalyser arityAnalyser;
  REQUIRE(!arityAnalyser.analyse(wrongArityProgram));
  REQUIRE(arityAnalyser.getErrors()[0].find("expects") != std::string::npos);
}

TEST_CASE("semantic analyser enforces heap allocation and donation rules") {
  auto heapDeclaration = expressionStatement(assignment(
      name("V_int_heap", true), unary("beg", literal("1"))));
  auto donateHeap = expressionStatement(unary("donate", name("V_int_heap")));
  auto useAfterDonate = expressionStatement(name("V_int_heap"));
  programNode invalidLifetime;
  invalidLifetime.items.push_back(functionWith(
      "F_int_main", {}, [&] {
        std::vector<NodePtr> statements;
        statements.push_back(std::move(heapDeclaration));
        statements.push_back(std::move(donateHeap));
        statements.push_back(std::move(useAfterDonate));
        return statements;
      }()));
  SemanticAnalyser lifetimeAnalyser;
  REQUIRE(!lifetimeAnalyser.analyse(invalidLifetime));
  REQUIRE(lifetimeAnalyser.getErrors()[0].find("Use after donate") !=
          std::string::npos);

  programNode invalidAllocation = programWith(expressionStatement(
      assignment(name("V_int_value", true),
                 unary("beg", literal("True", LitKind::BOOL)))));
  SemanticAnalyser allocationAnalyser;
  REQUIRE(!allocationAnalyser.analyse(invalidAllocation));
  REQUIRE(allocationAnalyser.getErrors()[0].find("incompatible") !=
          std::string::npos);
}

