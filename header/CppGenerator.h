#pragma once
#include <memory>
#include <string>
struct Symbol;
#include "SemanticAnalyser.h"
using namespace std;
struct Node;
using NodePtr = unique_ptr<Node>;
enum class NodeKind;
struct programNode;
struct FunctionNode;
struct ClassNode;
struct BlockNode;
struct ExprStmtNode;
struct ReturnNode;
struct IfNode;
struct ForNode;
struct WhileNode;
struct DoWhileNode;
struct AssignNode;
struct BinaryNode;
struct UnaryNode;
struct CallNode;
struct IndexNode;
struct MemberNode;
struct NameNode;
struct LiteralNode;
struct ArrayLiteralNode;

class CppGenerator {
  // Generate C++ code from the abstract syntax tree returned by the optimizer
 private:
  string code;
  int level = 0;
  string wrap(const NodeKind& kind, const string& printed, int tightestP,
              int currLevel);
  int tightestP = -1;

 public:
  const vector<unordered_map<string, Symbol>>* scope_list = nullptr;
  string& getCode() { return code; }
  // top level
  string visit(const programNode& n);
  string visit(const FunctionNode& n);
  string visit(const ClassNode& n);

  // statements
  string visit(const BlockNode& n);
  string visit(const ExprStmtNode& n);
  string visit(const ReturnNode& n);
  string visit(const IfNode& n);
  string visit(const ForNode& n);
  string visit(const WhileNode& n);
  string visit(const DoWhileNode& n);

  // expressions
  string visit(const AssignNode& n);
  string visit(const BinaryNode& n);
  string visit(const UnaryNode& n);
  string visit(const CallNode& n);
  string visit(const IndexNode& n);
  string visit(const MemberNode& n);
  string visit(const NameNode& n);
  string visit(const LiteralNode& n);
  string visit(const ArrayLiteralNode& n);

  // takes in the AST's program node (top node)
  CppGenerator() {};

  string indent() const { return string(level * 4, ' '); }

  void generate(const programNode& p);
  /// symbol manip;
  const Symbol* lookupSymbol(const string& name);
};