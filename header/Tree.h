#pragma once
#include <bits/stdc++.h>
using namespace std;
#include "CppGenerator.h"
#include "Tokenizer.h"
class CppGenerator;
struct Node;
/// Give the unique pointer an alias cuz I'm lazy.
using NodePtr = unique_ptr<Node>;

// one row per precedence level, loosest first
static const vector<vector<string>> LEVELS = {
    {"|"},      {"^"},           {"&"}, {"==", "!="}, {"<", "<=", ">", ">="},
    {"+", "-"}, {"*", "\\", "%"}};

enum class NodeKind {
  // expressions
  LITERAL,
  NAME,
  CALL,
  ARRAY_LITERAL,
  INDEX,
  MEMBER,
  UNARY,
  BINARY,
  ASSIGN,
  // statements
  BLOCK,
  EXPR_STMT,
  RETURN,
  IF,
  FOR,
  WHILE,
  DO_WHILE,
  // declarations
  FUNCTION,
  CLASS,
  // root
  PROGRAM
};

struct Ident {
  char kind = '?';
  string type;
  string name;
  string full;
  string length;  // arrays only, e.g. "[3]" or "[2][3]"

  static Ident decode(const string& text) {
    Ident id;
    id.full = text;
    id.kind = text[0];
    if (id.kind == 'C') {
      id.name = text.substr(2);
    } else if (id.kind == 'A') {
      size_t sep = text.rfind('_');
      id.name = text.substr(sep + 1);
      string t = text.substr(2, sep - 2);  // "int[3]" or "int"
      size_t br = t.find('[');
      if (br == string::npos) {
        id.type = t;
      } else {
        id.type = t.substr(0, br);
        id.length = t.substr(br);
      }
    } else {  // V_type_name, F_type_name
      size_t sep = text.find('_', 2);
      id.type = text.substr(2, sep - 2);
      id.name = text.substr(sep + 1);
    }
    return id;
  }
};

// the nodes struct.
struct Node {
  NodeKind kind;
  int line = 0;
  int col = 0;
  // Each subclass implements this by calling g.visit(*this).
  virtual string callVisit(CppGenerator& g) const = 0;
  Node(NodeKind k, int line = 0, int col = 0) : kind(k), line(line), col(col) {}
  Node(NodeKind k, const Token& t)
      : kind(k), line(t.getLine()), col(t.getCharNum()) {}
};

// Template return the node of type.
template <class T>
T* as(Node* n) {
  return (n && n->kind == T::KIND) ? static_cast<T*>(n) : nullptr;
}
// Same as before.
template <class T>
const T* as(const Node* n) {
  return (n && n->kind == T::KIND) ? static_cast<const T*>(n) : nullptr;
}

// Literals types.
enum class LitKind { INT, FLOAT, BOOL, STRING };

// The Literals.,
struct LiteralNode : Node {
  static constexpr NodeKind KIND = NodeKind::LITERAL;
  LitKind litKind = LitKind::INT;
  string text;
  LiteralNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// checks the validity of the name.
struct NameNode : Node {
  static constexpr NodeKind KIND = NodeKind::NAME;
  Ident id;
  bool isDecl = false;
  NameNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// functions
struct CallNode : Node {
  static constexpr NodeKind KIND = NodeKind::CALL;
  Ident callee;
  vector<NodePtr> args;
  CallNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// array
struct ArrayLiteralNode : Node {
  static constexpr NodeKind KIND = NodeKind::ARRAY_LITERAL;
  vector<NodePtr> elems;
  ArrayLiteralNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// indexing
struct IndexNode : Node {
  static constexpr NodeKind KIND = NodeKind::INDEX;
  NodePtr base, index;
  IndexNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// The member node.
struct MemberNode : Node {
  static constexpr NodeKind KIND = NodeKind::MEMBER;
  NodePtr object, member;
  MemberNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// unary operators
struct UnaryNode : Node {
  static constexpr NodeKind KIND = NodeKind::UNARY;
  string op;
  NodePtr operand;
  UnaryNode(const Token& t) : Node(KIND, t), op(t.getContent()) {}
  string callVisit(CppGenerator& g) const override;
};

// expression operators.
struct BinaryNode : Node {
  static constexpr NodeKind KIND = NodeKind::BINARY;
  string op;
  NodePtr lhs, rhs;
  BinaryNode(const Token& t) : Node(KIND, t), op(t.getContent()) {}
  string callVisit(CppGenerator& g) const override;
};

// assignment operators.
struct AssignNode : Node {
  static constexpr NodeKind KIND = NodeKind::ASSIGN;
  string op;
  NodePtr target, value;
  AssignNode(const Token& t) : Node(KIND, t), op(t.getContent()) {}
  string callVisit(CppGenerator& g) const override;
};

// statements
struct BlockNode : Node {
  static constexpr NodeKind KIND = NodeKind::BLOCK;
  vector<NodePtr> stmts;
  BlockNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// expr
struct ExprStmtNode : Node {
  static constexpr NodeKind KIND = NodeKind::EXPR_STMT;
  NodePtr expr;
  ExprStmtNode(const Token& t) : Node(KIND, t) {}
  ExprStmtNode() : Node(KIND) {}
  string callVisit(CppGenerator& g) const override;
};

// return expr
struct ReturnNode : Node {
  static constexpr NodeKind KIND = NodeKind::RETURN;
  NodePtr value;
  ReturnNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// if statement.
struct IfNode : Node {
  static constexpr NodeKind KIND = NodeKind::IF;
  NodePtr cond, thenBlock;
  vector<pair<NodePtr, NodePtr>> elseifs;
  NodePtr elseBlock;
  IfNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// For loop.
struct ForNode : Node {
  static constexpr NodeKind KIND = NodeKind::FOR;
  NodePtr init, test, step, body;
  ForNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// while do
struct WhileNode : Node {
  static constexpr NodeKind KIND = NodeKind::WHILE;
  NodePtr cond, body;
  WhileNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// Do while
struct DoWhileNode : Node {
  static constexpr NodeKind KIND = NodeKind::DO_WHILE;
  NodePtr body, cond;
  DoWhileNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// declaring

// functions.
struct FunctionNode : Node {
  static constexpr NodeKind KIND = NodeKind::FUNCTION;
  Ident id;
  vector<NodePtr> params;
  NodePtr body;
  FunctionNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// class origination.
struct ClassNode : Node {
  static constexpr NodeKind KIND = NodeKind::CLASS;
  Ident id;
  vector<Ident> bases;
  vector<NodePtr> members;
  ClassNode(const Token& t) : Node(KIND, t) {}
  string callVisit(CppGenerator& g) const override;
};

// Root of my nodes.

struct programNode : Node {
  static constexpr NodeKind KIND = NodeKind::PROGRAM;
  vector<NodePtr> items;
  programNode() : Node(KIND, 1, 1) {}
  string callVisit(CppGenerator& g) const override;
};
