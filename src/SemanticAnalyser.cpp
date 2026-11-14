#include "../header/SemanticAnalyser.h"

#include <cctype>
#include <iostream>

// Creates an initial scope when initialised 
SemanticAnalyser::SemanticAnalyser() {
    enterScope();
}

// enterScope and exitScope are used for scope management and are called
// when the analyser enters and exits new blocks of code that use a seperate scope (such as fuctions) 
void SemanticAnalyser::enterScope() { scope_list.emplace_back(); }

void SemanticAnalyser::exitScope() {
  if (!scope_list.empty()) {
    scope_list.pop_back();
  }
}

// This function adds a new variable to the current scope if it doesn't already exist
bool SemanticAnalyser::declare(const Symbol& symbol) {
  if (scope_list.empty()) {
    enterScope();
  }
  auto& current = scope_list.back();
  if (current.count(symbol.name)) {
    return false;
  }
  current[symbol.name] = symbol;
  return true;
}

// This function searches for a variable starting from the innermost scope, and continues outwards.
// Returns a pointer to the variable's symbol if found, otherwise it returns nullptr.
Symbol* SemanticAnalyser::lookup(const string& name) {
  for (auto it = scope_list.rbegin(); it != scope_list.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      return &found->second;
    }
  }
  return nullptr;
}

// This records the semantic errors in the error list using the line and column information from the AST node.
void SemanticAnalyser::error(const Node* node, const string& message) {
  errors.push_back("Semantic error has occured at " + to_string(node->line) +
                   ":" + to_string(node->col) + ": " + message);
}



bool SemanticAnalyser::hasErrors() const { return !errors.empty(); }

const vector<string>& SemanticAnalyser::getErrors() const { return errors; }

SemanticType SemanticAnalyser::getType(const string& type) {
  if (type == "int") return SemanticType::INT;
  if (type == "float") return SemanticType::FLOAT;
  if (type == "bool") return SemanticType::BOOL;
  if (type == "char") return SemanticType::CHAR;
  if (type == "string") return SemanticType::STRING;
  if (type == "void") return SemanticType::VOID;
  if (!type.empty() && isupper(static_cast<unsigned char>(type[0])))
    return SemanticType::CLASS;
  return SemanticType::UNKNOWN;
}

SemanticType SemanticAnalyser::checkExpression(const Node* node) {
  if (!node) {
    return SemanticType::VOID;
  }
  switch (node->kind) {
    // For the case of a literal node, check for all literal kinds
    case NodeKind::LITERAL: {
      auto* n = as<LiteralNode>(node);
      switch (n->litKind) {
        case LitKind::INT:
          return SemanticType::INT;
        case LitKind::FLOAT:
          return SemanticType::FLOAT;
        case LitKind::BOOL:
          return SemanticType::BOOL;
        case LitKind::STRING:
          return SemanticType::STRING;
      }
      return SemanticType::ERROR;
    }
      // For the case of a variable node, check if the variable is declared
    case NodeKind::NAME: {
      auto* n = as<NameNode>(node);
      // Bare declaration with no initialiser, e.g.  V_int_age/
      if (n->isDecl) {
        SemanticType declared = getType(n->id.type);
        if (declared == SemanticType::UNKNOWN ||
            declared == SemanticType::VOID) {
          error(node, "Invalid variable type");
          return SemanticType::ERROR;
        }
        if (!declare({n->id.full, declared})) {
          error(node, "Duplicate declaration: " + n->id.full);
          return SemanticType::ERROR;
        }
        return declared;
      }
      Symbol* symbol = lookup(n->id.full);
      if (!symbol) {
        error(node, "Unknown or undeclared variable: " + n->id.full);
        return SemanticType::ERROR;
      }
      return symbol->type;
    }
    // For the case of a binary node
    case NodeKind::BINARY: {
      auto* n = as<BinaryNode>(node);
      SemanticType lhs = checkExpression(n->lhs.get());
      SemanticType rhs = checkExpression(n->rhs.get());

      return checkBinary(n->op, lhs, rhs, node);
    }
    // Checking for the case of unary nodes
    case NodeKind::UNARY: {
      auto* n = as<UnaryNode>(node);
      SemanticType operand = checkExpression(n->operand.get());
      if (operand == SemanticType::ERROR) {
        return SemanticType::ERROR;
      }
      if (n->op == "!") {
        if (operand == SemanticType::BOOL) {
          return SemanticType::BOOL;
        }
        error(node, "'!' requires a boolean operand");
        return SemanticType::ERROR;
      }
      if (n->op == "-") {
        if (operand == SemanticType::INT || operand == SemanticType::FLOAT) {
          return operand;
        }
        error(node, "Unary '-' requires a numeric operand");
        return SemanticType::ERROR;
      }

      // Memory operators need pointer types to be fully checked
      if (n->op == "beg" || n->op == "donate") {
        return SemanticType::UNKNOWN;
      }
      return SemanticType::UNKNOWN;
    }
    // Assignment Node Case, call checkAssignment helper function
    case NodeKind::ASSIGN: {
      auto* n = as<AssignNode>(node);
      return checkAssignment(n);
    }
    // Function Call Node Case
    case NodeKind::CALL: {
      auto* n = as<CallNode>(node);
      SemanticType result;
      if (n->callee.kind == 'F') {
        result = getType(n->callee.type);
      } else if (n->callee.kind == 'C') {
        result = SemanticType::CLASS;
      } else {
        error(node, "Invalid function or constructor call");
        return SemanticType::ERROR;
      }

      for (const auto& arg : n->args) {
        SemanticType argType = checkExpression(arg.get());
        if (argType == SemanticType::ERROR) {
          return SemanticType::ERROR;
        }
      }
      return result;
    }
    // Array Literal Nodes Case
    case NodeKind::ARRAY_LITERAL: {
      auto* n = as<ArrayLiteralNode>(node);
      if (n->elems.empty()) {
        return SemanticType::ARRAY;
      }
      SemanticType first = checkExpression(n->elems[0].get());
      for (size_t i = 1; i < n->elems.size(); i++) {
        SemanticType current = checkExpression(n->elems[i].get());
        if (current == SemanticType::ERROR) {
          return SemanticType::ERROR;
        }
        if (current != first) {
          error(node, "Array elements must have matching types.");
          return SemanticType::ERROR;
        }
      }
      return SemanticType::ERROR;
    }
    // Array Index Nodes Case
    case NodeKind::INDEX: {
      auto* n = as<IndexNode>(node);
      SemanticType base = checkExpression(n->base.get());
      SemanticType index = checkExpression(n->index.get());
      if (base == SemanticType::ERROR || index == SemanticType::ERROR) {
        return SemanticType::ERROR;
      }
      if (index != SemanticType::INT) {
        error(node, "Array index must be an integer.");
        return SemanticType::ERROR;
      }
      if (base != SemanticType::ARRAY) {
        error(node, "Cannot index a non-array value.");
        return SemanticType::ERROR;
      }

      // Element type resolution can be added when symbol stores array element
      // types
      return SemanticType::UNKNOWN;
    }
    // Member Access Case
    case NodeKind::MEMBER: {
      auto* n = as<MemberNode>(node);
      SemanticType object = checkExpression(n->object.get());
      if (object == SemanticType::ERROR) {
        return SemanticType::ERROR;
      }
      // **Requires a class member symbol table**
      return SemanticType::UNKNOWN;
    }
    default:
      error(node, "Invalid expression node");
      return SemanticType::UNKNOWN;
  }
}

SemanticType SemanticAnalyser::checkBinary(const string& op, SemanticType lhs,
                                           SemanticType rhs, const Node* node) {
  if (lhs == SemanticType::ERROR || rhs == SemanticType::ERROR) {
    return SemanticType::ERROR;
  }
  bool lhsNumeric = lhs == SemanticType::INT || lhs == SemanticType::FLOAT;
  bool rhsNumeric = rhs == SemanticType::INT || rhs == SemanticType::FLOAT;

  // Check the original arithmetic rules
  if (op == "+" || op == "-" || op == "*" || op == "\\" || op == "%" ||
      op == "**") {
    // Check for which type the resulting value should be
    if (!lhsNumeric || !rhsNumeric) {
      error(node, "Invalid arithmetic operands, must require numeric operands");
      return SemanticType::ERROR;
    }
    if (op == "%" && (lhs != SemanticType::INT || rhs != SemanticType::INT)) {
      error(node, "'%' requires integer operands");
      return SemanticType::ERROR;
    }
    if (lhs == SemanticType::FLOAT || rhs == SemanticType::FLOAT) {
      return SemanticType::FLOAT;
    }
    return SemanticType::INT;
  }

  // Comparison Operations Check
  if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" ||
      op == ">=") {
    if (lhsNumeric && rhsNumeric) {
      return SemanticType::BOOL;
    }
    if (lhs == rhs && (op == "==" || op == "!=")) {
      return SemanticType::BOOL;
    }
    error(node, "Incompatible comparison types used.");
    return SemanticType::ERROR;
  }

  // Bitwise Operations Check
  if (op == "&" || op == "|" || op == "^") {
    if (lhs == SemanticType::INT && rhs == SemanticType::INT) {
      return SemanticType::INT;
    }
    error(node, "Bitwise operators require integers to compute");
    return SemanticType::ERROR;
  }
  return SemanticType::UNKNOWN;
}

SemanticType SemanticAnalyser::checkAssignment(const AssignNode* node) {
  if (!node->target || !node->value) {
    return SemanticType::ERROR;
  }
  auto* target = as<NameNode>(node->target.get());
  // Evaluate the RHS first
  SemanticType rhs = checkExpression(node->value.get());
  if (rhs == SemanticType::ERROR) {
    return SemanticType::ERROR;
  }

  // Variable Declaration
  if (target && target->isDecl) {
    if (node->op != "=") {
      error(node, "Declarations must use '='");
      return SemanticType::ERROR;
    }
    SemanticType declared = getType(target->id.type);
    if (declared == SemanticType::UNKNOWN || declared == SemanticType::VOID) {
      error(node, "Invalid variable type");
      return SemanticType::ERROR;
    }
    if (rhs != SemanticType::UNKNOWN && rhs != declared) {
      // Allow integer to float conversion
      if (!(declared == SemanticType::FLOAT && rhs == SemanticType::INT)) {
        error(node, "Cannot assign value of incompatible type.");
        return SemanticType::ERROR;
      }
    }

    // Check if the RHS is a heap allocation using "beg"
    bool isHeapAlloc = false;
    if (node->value && node->value->kind == NodeKind::UNARY) {
      auto* unaryOp = as<UnaryNode>(node->value.get());
      if (unaryOp && unaryOp->op == "beg") {
        isHeapAlloc = true;
      }
    }

    // Declare the symbol with the correct isHeap flag
    if (!declare({target->id.full, declared, isHeapAlloc})) {
      error(node, "Duplicate declaration: " + target->id.full);
      return SemanticType::ERROR;
    }
    return declared;
  }
  // Normal Assignment
  SemanticType lhs = checkExpression(node->target.get());
  if (lhs == SemanticType::ERROR) {
    return SemanticType::ERROR;
  }
  if (node->op == "=") {
    if (lhs != rhs &&
        !(lhs == SemanticType::FLOAT && rhs == SemanticType::INT) &&
        rhs != SemanticType::UNKNOWN) {
      error(node, "Assignment type mismatch");
      return SemanticType::ERROR;
    }
    return lhs;
  }
  // Compound Assignment
  string binaryOp = node->op.substr(0, node->op.size() - 1);
  SemanticType result = checkBinary(binaryOp, lhs, rhs, node);
  if (result == SemanticType::ERROR) {
    return SemanticType::ERROR;
  }
  if (result != lhs &&
      !(lhs == SemanticType::FLOAT && result == SemanticType::INT)) {
    error(node, "Compound assignment type mismatch.");
    return SemanticType::ERROR;
  }
  return lhs;
}

void SemanticAnalyser::checkNode(const Node* node) {
  if (!node) {
    return;
  }
  switch (node->kind) {
    // Program Node
    case NodeKind::PROGRAM: {
      auto* n = as<programNode>(node);
      for (const auto& item : n->items) {
        checkNode(item.get());
      }
      break;
    }
    // Block Node
    case NodeKind::BLOCK: {
      auto* n = as<BlockNode>(node);
      enterScope();
      for (const auto& stmt : n->stmts) {
        checkNode(stmt.get());
      }
      exitScope();
      break;
    }
    // Expression Statement
    case NodeKind::EXPR_STMT: {
      auto* n = as<ExprStmtNode>(node);
      checkExpression(n->expr.get());
      break;
    }
    // Return
    case NodeKind::RETURN: {
      auto* n = as<ReturnNode>(node);
      SemanticType actual = SemanticType::VOID;
      if (n->value) {
        actual = checkExpression(n->value.get());
      }
      if (currentReturnType == SemanticType::UNKNOWN) {
        error(node, "Return statement outside of a function.");
        break;
      }
      if (actual != SemanticType::ERROR && actual != currentReturnType &&
          !(currentReturnType == SemanticType::FLOAT &&
            actual == SemanticType::INT)) {
        error(node, "Return type does not match function return type.");
      }
      break;
    }
    // If Statement
    case NodeKind::IF: {
      auto* n = as<IfNode>(node);
      SemanticType cond = checkExpression(n->cond.get());
      if (cond != SemanticType::BOOL && cond != SemanticType::ERROR &&
          cond != SemanticType::UNKNOWN) {
        error(node, "If condition must be boolean.");
      }
      checkNode(n->thenBlock.get());
      for (const auto& [condition, block] : n->elseifs) {
        SemanticType elseifType = checkExpression(condition.get());
        if (elseifType != SemanticType::BOOL &&
            elseifType != SemanticType::ERROR &&
            elseifType != SemanticType::UNKNOWN) {
          error(condition.get(), "Elseif condition must be boolean.");
        }
        checkNode(block.get());
      }
      if (n->elseBlock) {
        checkNode(n->elseBlock.get());
      }
      break;
    }
    // For Loop Statement
    case NodeKind::FOR: {
      auto* n = as<ForNode>(node);
      enterScope();
      if (n->init) {
        checkExpression(n->init.get());
      }
      if (n->test) {
        SemanticType test = checkExpression(n->test.get());
        if (test != SemanticType::BOOL && test != SemanticType::ERROR &&
            test != SemanticType::UNKNOWN) {
          error(node, "For loop condition must be boolean.");
        }
      }
      if (n->step) {
        checkExpression(n->step.get());
      }
      checkNode(n->body.get());
      exitScope();
      break;
    }
    // While Loop Checking
    case NodeKind::WHILE: {
      auto* n = as<WhileNode>(node);
      SemanticType cond = checkExpression(n->cond.get());
      if (cond != SemanticType::BOOL && cond != SemanticType::ERROR &&
          cond != SemanticType::UNKNOWN) {
        error(node, "While condition must be boolean");
      }
      checkNode(n->body.get());
      break;
    }
    // Do While Loop
    case NodeKind::DO_WHILE: {
      auto* n = as<DoWhileNode>(node);
      checkNode(n->body.get());
      SemanticType cond = checkExpression(n->cond.get());
      if (cond != SemanticType::BOOL && cond != SemanticType::ERROR &&
          cond != SemanticType::UNKNOWN) {
        error(node, "Do-while condition must be boolean");
      }
      break;
    }
    // Function Condition
    case NodeKind::FUNCTION: {
      auto* n = as<FunctionNode>(node);
      SemanticType oldReturnType = currentReturnType;
      currentReturnType = getType(n->id.type);
      if (currentReturnType == SemanticType::UNKNOWN) {
        error(node, "Unknown function return type");
      }
      // Parameters and the function body share the same scope in the Parser.
      enterScope();
      for (const auto& param : n->params) {
        auto* p = as<NameNode>(param.get());
        if (!p) {
          continue;
        }
        SemanticType paramType = getType(p->id.type);
        if (paramType == SemanticType::UNKNOWN ||
            paramType == SemanticType::VOID) {
          error(param.get(), "Invalid parameter type.");
          continue;
        }
        if (!declare({p->id.full, paramType})) {
          error(param.get(), "Duplicate parameter.");
        }
      }
      // We do not call the checkNode() in the function body here as that would
      // create an additional scope for its block.
      auto* body = as<BlockNode>(n->body.get());
      if (body) {
        for (const auto& stmt : body->stmts) {
          checkNode(stmt.get());
        }
      }
      exitScope();
      currentReturnType = oldReturnType;
      break;
    }
    // Class Section
    case NodeKind::CLASS: {
      auto* n = as<ClassNode>(node);
      enterScope();
      for (const auto& member : n->members) {
        checkNode(member.get());
      }
      exitScope();
      break;
    }
    // Other Expressions
    default: {
      checkExpression(node);
      break;
    }
  }
}

bool SemanticAnalyser::analyse(const programNode& program) {
  errors.clear();
  scope_list.clear();
  currentReturnType = SemanticType::UNKNOWN;
  enterScope();
  checkNode(&program);
  exitScope();
  return !hasErrors();
}