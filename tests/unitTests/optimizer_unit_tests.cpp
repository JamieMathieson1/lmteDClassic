#include "test_framework.h"

#include "../../header/Optimizer.h"

static Token token(const std::string& text) {
  return Token(text, TokenType::LITERAL, 1, 1);
}

static NodePtr literal(const std::string& text, LitKind kind = LitKind::INT) {
  auto node = std::make_unique<LiteralNode>(token(text));
  node->text = text;
  node->litKind = kind;
  return node;
}

static NodePtr binary(const std::string& op, NodePtr lhs, NodePtr rhs) {
  auto node = std::make_unique<BinaryNode>(token(op));
  node->lhs = std::move(lhs);
  node->rhs = std::move(rhs);
  return node;
}

static NodePtr expression(NodePtr value) {
  auto node = std::make_unique<ExprStmtNode>(token("/"));
  node->expr = std::move(value);
  return node;
}

TEST_CASE("optimizer folds nested integer arithmetic") {
  NodePtr root = binary("+", literal("2"), binary("*", literal("3"), literal("4")));
  Optimizer optimizer;
  optimizer.optimize(root);
  auto* result = as<LiteralNode>(root.get());
  REQUIRE(result != nullptr);
  REQUIRE_EQ(result->text, std::string("14"));
}

TEST_CASE("optimizer folds unary and boolean expressions") {
  NodePtr root = binary("==", literal("False", LitKind::BOOL),
                        literal("False", LitKind::BOOL));
  Optimizer optimizer;
  optimizer.optimize(root);
  auto* result = as<LiteralNode>(root.get());
  REQUIRE(result != nullptr);
  REQUIRE_EQ(result->litKind, LitKind::BOOL);
  REQUIRE_EQ(result->text, std::string("True"));
}

TEST_CASE("optimizer removes statements after a return") {
  auto function = std::make_unique<FunctionNode>(token("F_int_main"));
  function->id = Ident::decode("F_int_main");
  auto body = std::make_unique<BlockNode>(token("d"));
  auto ret = std::make_unique<ReturnNode>(token("return"));
  ret->value = literal("1");
  body->stmts.push_back(std::move(ret));
  body->stmts.push_back(expression(literal("2")));
  function->body = std::move(body);
  NodePtr root = std::move(function);
  Optimizer optimizer;
  optimizer.optimize(root);
  auto* optimizedBody =
      as<BlockNode>(as<FunctionNode>(root.get())->body.get());
  REQUIRE_EQ(optimizedBody->stmts.size(), size_t{1});
}

TEST_CASE("optimizer unrolls a fixed-count loop") {
  auto loop = std::make_unique<ForNode>(token("condition"));
  auto init = std::make_unique<AssignNode>(token("="));
  auto variable = std::make_unique<NameNode>(token("V_int_i"));
  variable->id = Ident::decode("V_int_i");
  variable->isDecl = true;
  init->target = std::move(variable);
  init->value = literal("0");
  loop->init = std::move(init);
  loop->test = binary("<", [&] {
    auto n = std::make_unique<NameNode>(token("V_int_i"));
    n->id = Ident::decode("V_int_i");
    return n;
  }(), literal("2"));
  auto step = std::make_unique<AssignNode>(token("="));
  auto stepTarget = std::make_unique<NameNode>(token("V_int_i"));
  stepTarget->id = Ident::decode("V_int_i");
  step->target = std::move(stepTarget);
  step->value = binary("+", [&] {
    auto n = std::make_unique<NameNode>(token("V_int_i"));
    n->id = Ident::decode("V_int_i");
    return n;
  }(), literal("1"));
  loop->step = std::move(step);
  loop->body = std::make_unique<BlockNode>(token("d"));
  NodePtr root = std::move(loop);
  Optimizer optimizer;
  optimizer.optimize(root);
  REQUIRE(as<BlockNode>(root.get()) != nullptr);
  REQUIRE(as<BlockNode>(root.get())->stmts.size() >= size_t{3});
}

