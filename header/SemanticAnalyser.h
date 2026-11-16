#pragma once

#include <bits/stdc++.h>

#include "Tree.h"
struct Node;
struct FunctionNode;
struct Ident;
using NodePtr =
    std::unique_ptr<Node>;  // This enumeration defines the multiple different
                            // data types that the semantic
// analyser can recognise.
enum class SemanticType {
  INT,
  FLOAT,
  BOOL,
  CHAR,
  STRING,
  VOID,
  ARRAY,
  CLASS,
  UNKNOWN,
  ERROR
};
struct Ident;
struct TypeInfo {
  // Arrays use elem, and classes use className; primitive types use only kind.
  SemanticType kind = SemanticType::UNKNOWN;
  string className;
  SemanticType elem = SemanticType::UNKNOWN;

  TypeInfo() = default;

  TypeInfo(SemanticType k) : kind(k) {}

  TypeInfo(SemanticType k, string cls) : kind(k), className(std::move(cls)) {}

  bool operator==(SemanticType k) const { return kind == k; }

  bool operator!=(SemanticType k) const { return kind != k; }

  bool operator==(const TypeInfo& other) const {
    return kind == other.kind && className == other.className &&
           elem == other.elem;
  }

  bool operator!=(const TypeInfo& other) const { return !(*this == other); }
};
struct Symbol {
  // A symbol records both its declared type and state used by later checks.
  bool variadic = false;

  string name;
  TypeInfo type;
  bool isHeap = false;
  bool freed = false;
  bool isFunction = false;
  vector<TypeInfo> params;
};

struct Node;
struct programNode;
struct AssignNode;
struct FunctionSignature {
  SemanticType returnType;
  std::vector<SemanticType> parameterTypes;
};
class SemanticAnalyser {
 public:
  // Expose scopes to code generation after semantic analysis has completed.
  const std::vector<std::unordered_map<std::string, Symbol>>& getScopeList()
      const {
    return scope_list;
  }

 private:
  // Private Variables include:
  // 1. A Scope type as an unordered map that linkes variable names with their
  // Symbol info
  // 2. A scope_list which holds all the currently active scopes
  // 3. A list of all the errors discovered during the analysis.
  // 4. A currentReturnType variable that keeps track of the return type of
  // the function being analysed.
  using Scope = unordered_map<string, Symbol>;
  vector<Scope> scope_list;
  vector<string> errors;

  // Managing the creation and removal of Scopes
  void enterScope();
  void exitScope();

  // Managing Variable Declarations and Symbol Table lookup
  bool declare(const Symbol& symbol);
  Symbol* lookup(const string& name);

  // Record and Associates any errors
  void error(const Node* node, const string& message);

  // Getting Type Helper Function
  SemanticType getType(const string& type);

  // Do a specialised check for a binary expression

  TypeInfo checkBinary(const string& op, const TypeInfo& lhs,
                       const TypeInfo& rhs, const Node* node);
  // Do a specialised check for assignment expressions
  TypeInfo checkAssignment(const AssignNode* node);

  // Traverse the AST
  void checkNode(const Node* node);
  TypeInfo currentReturnType;
  unordered_map<string, vector<string>> classBases;
  // Analyse the Expression
  TypeInfo checkExpression(const Node* node);

 public:
  bool assignable(const TypeInfo& to, const TypeInfo& from) const;
  bool isSubclass(const string& derived, const string& base) const;
  bool checkArgs(const Symbol& f, const vector<NodePtr>& args, const Node* at);
  Symbol makeFunctionSymbol(const FunctionNode* f);
  string currentClass;
  bool inFunction = false;
  unordered_set<string> declaredFunctions;
  unordered_set<string> declaredClasses;

  bool alwaysReturns(const Node* node) const;
  SemanticType resolveType(const string& t);
  const Symbol* symbolOf(const Node* node);
  SemanticAnalyser();
  bool analyse(const programNode& program);
  bool hasErrors() const;
  TypeInfo getTypeInfo(const Ident& id);
  bool validVarType(const TypeInfo& t) const;
  const vector<string>& getErrors() const;
  Symbol* findMember(const string& cls, const string& memberName);
  void requireBool(const Node* at, const TypeInfo& t, const string& what);
  void addBuiltin(const string& name, SemanticType ret, bool variadic = true);
};