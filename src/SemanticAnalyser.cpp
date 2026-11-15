#include "../header/SemanticAnalyser.h"

#include <cctype>
#include <iostream>

// Creates an initial scope when initialised
SemanticAnalyser::SemanticAnalyser() { enterScope(); }

// enterScope and exitScope are used for scope management and are called
// when the analyser enters and exits new blocks of code that use a seperate
// scope (such as fuctions)
void SemanticAnalyser::enterScope() { scope_list.emplace_back(); }

void SemanticAnalyser::exitScope() {
  if (!scope_list.empty()) {
    scope_list.pop_back();
  }
}

// This function adds a new variable to the current scope if it doesn't already
// exist
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

// This function searches for a variable starting from the innermost scope, and
// continues outwards. Returns a pointer to the variable's symbol if found,
// otherwise it returns nullptr.
Symbol* SemanticAnalyser::lookup(const string& name) {
  for (auto it = scope_list.rbegin(); it != scope_list.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      return &found->second;
    }
  }
  return nullptr;
}

// This records the semantic errors in the error list using the line and column
// information from the AST node.
void SemanticAnalyser::error(const Node* node, const string& message) {
  errors.push_back("Semantic error has occured at " + to_string(node->line) +
                   ":" + to_string(node->col) + ": " + message);
}

// This checks whether the error vector has any errors within it
bool SemanticAnalyser::hasErrors() const { return !errors.empty(); }

// This returns the vector containing the semantic errors.
const vector<string>& SemanticAnalyser::getErrors() const { return errors; }

// This function converts type names that are strings into their SemanticType
// enum values
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

// This function examines an expression node and determines its SemanticType
// It then checks whether the expression follows the type rules
// It returns a SemanticType representing the resulting type of the expression
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
        case LitKind::CHAR:
          return SemanticType::CHAR;
      }
      return SemanticType::ERROR;
    }
    // For the case of a variable node, check if the variable is declared
    // If the variable does not exist, return unknown type and error
    // If it does, we return the semantic type of the variable
    case NodeKind::NAME: {
      auto* n = as<NameNode>(node);
      // Bare declaration with no initialiser, e.g.  V_int_age/
      if (n->isDecl) {
        SemanticType declared =
            n->id.kind == 'A' ? SemanticType::ARRAY : getType(n->id.type);
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
    // For the case of a binary node, call the helper checkBinary with the lhs
    // and rhs of the expression
    case NodeKind::BINARY: {
      auto* n = as<BinaryNode>(node);
      SemanticType lhs = checkExpression(n->lhs.get());
      SemanticType rhs = checkExpression(n->rhs.get());

      return checkBinary(n->op, lhs, rhs, node);
    }
    // Checking for the case of unary nodes
    case NodeKind::UNARY: {
      auto* n = as<UnaryNode>(node);
      if (n->op == "beg" && n->operand) {
          if (const auto* name = as<NameNode>(n->operand.get())) {
              return getType(name->id.type);
          }
          if (const auto* call = as<CallNode>(n->operand.get())) {
              return getType(call->callee.type);
          }
      }
      SemanticType operand = checkExpression(n->operand.get());
      if (operand == SemanticType::ERROR) {
        return SemanticType::ERROR;
      }
      // Checks for the "logical NOT operator and return a bool value if the
      // operand is bool"
      if (n->op == "!") {
        if (operand == SemanticType::BOOL) {
          return SemanticType::BOOL;
        }
        error(node, "'!' requires a boolean operand");
        return SemanticType::ERROR;
      }
      // Check for the unary minus, which is for negative values.
      // Is only allowed for INT and FLOAT operand types
      if (n->op == "+" || n->op == "-") {
        if (operand == SemanticType::INT || operand == SemanticType::FLOAT) {
          return operand;
        }
        error(node, "Unary '" + n->op + "' requires a numeric operand");
        return SemanticType::ERROR;
      }

      // Memory operators need pointer types to be fully checked
      if (n->op == "beg" || n->op == "donate") {
        return SemanticType::UNKNOWN;
      }
      return SemanticType::UNKNOWN;
    }
    // Assignment logic cases will be handled in the checkAssignment helper
    // function
    case NodeKind::ASSIGN: {
      auto* n = as<AssignNode>(node);
      return checkAssignment(n);
    }
    // The analyser checks if the call if for a function or a class with the
    // callee.kind value It then returns the respective type for the function or
    // the class type The analyser then loops through the arguments supplied to
    // the function call
    case NodeKind::CALL: {
      auto* n = as<CallNode>(node);
      SemanticType result;
      if (n->callee.kind == 'F' || n->callee.kind == 'V') {
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
    // If its an array literal case, first we need to check if its empty. If it
    // is, we return array semantic type Next we need to check the first element
    // type, as that is the expecting type for all other elements in the array
    // If any other element in the array is not the expected type we return an
    // error otherwise we return array type
    case NodeKind::ARRAY_LITERAL: {
      auto* n = as<ArrayLiteralNode>(node);
      if (n->elems.empty()) {
        return SemanticType::ARRAY;
      }
      SemanticType first = checkExpression(n->elems[0].get());
      if (first == SemanticType::ERROR) {
        return SemanticType::ERROR;
      }
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
      return SemanticType::ARRAY;
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

// This function checks the binary cases
SemanticType SemanticAnalyser::checkBinary(const string& op, SemanticType lhs,
                                           SemanticType rhs, const Node* node) {
  if (lhs == SemanticType::ERROR || rhs == SemanticType::ERROR) {
    return SemanticType::ERROR;
  }
  if (lhs == SemanticType::UNKNOWN || rhs == SemanticType::UNKNOWN) {
    bool isCmp = (op == "==" || op == "!=" || op == "<" || op == ">" ||
                  op == "<=" || op == ">=");
    bool isLogic = (op == "&&" || op == "||");
    return (isCmp || isLogic) ? SemanticType::BOOL : SemanticType::UNKNOWN;
  }
  if (op == "&&" || op == "||") {
    if (lhs == SemanticType::BOOL && rhs == SemanticType::BOOL) {
      return SemanticType::BOOL;
    }
    error(node, "Logical operators '&&' and '||' require boolean operands");
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
void SemanticAnalyser::requireBool(const Node* at, SemanticType t,
                                   const string& what) {
  // TODO: drop UNKNOWN once INDEX/MEMBER/beg return real types
  if (t != SemanticType::BOOL && t != SemanticType::ERROR &&
      t != SemanticType::UNKNOWN)
    error(at, what + " must be boolean.");
}
SemanticType SemanticAnalyser::checkAssignment(const AssignNode* node) {
  if (!node->target || !node->value) {
    return SemanticType::ERROR;
  }
  const NameNode* target = node->target->kind == NodeKind::NAME
                               ? as<NameNode>(node->target.get())
                               : nullptr;
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
    SemanticType declared =
        target->id.kind == 'A' ? SemanticType::ARRAY : getType(target->id.type);
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
  if (node->target->kind != NodeKind::NAME &&
      node->target->kind != NodeKind::INDEX &&
      node->target->kind != NodeKind::MEMBER) {
    error(node, "Left side of assignment is not assignable.");
    return SemanticType::ERROR;
  }
  // Normal Assignment
  SemanticType lhs = checkExpression(node->target.get());
  if (lhs == SemanticType::ERROR) {
    return SemanticType::ERROR;
  }
  if (node->op == "=") {
    if (lhs != SemanticType::UNKNOWN && lhs != rhs &&
        !(lhs == SemanticType::FLOAT && rhs == SemanticType::INT) &&
        rhs != SemanticType::UNKNOWN) {
      error(node, "Assignment type mismatch");
      return SemanticType::ERROR;
    }
    return lhs;
  }
  // Compound Assignment
  if (lhs == SemanticType::UNKNOWN) {
    return lhs;
  }
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
        actual = checkExpression(
            n->value.get());  // must stay: evaluates the returned expression
      }
      if (!inFunction) {
        error(node, "Return statement outside of a function.");
        break;
      }
      if (currentReturnType == SemanticType::ERROR ||
          actual == SemanticType::ERROR) {
        break;
      }
      if (actual != SemanticType::UNKNOWN && actual != currentReturnType &&
          !(currentReturnType == SemanticType::FLOAT &&
            actual == SemanticType::INT)) {
        error(node, "Return type does not match function return type.");
      }
      break;
    }
    // If Statement
    case NodeKind::IF: {
      auto* n = as<IfNode>(node);
      requireBool(n->cond.get(), checkExpression(n->cond.get()),
                  "If condition");
      checkNode(n->thenBlock.get());
      for (const auto& [condition, block] : n->elseifs) {
        requireBool(condition.get(), checkExpression(condition.get()),
                    "Elseif condition");
        checkNode(block.get());
      }
      if (n->elseBlock) checkNode(n->elseBlock.get());
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
        requireBool(n->test.get(), checkExpression(n->test.get()),
                    "For loop condition");
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
      requireBool(n->cond.get(), checkExpression(n->cond.get()),
                  "While condition");
      checkNode(n->body.get());
      break;
    }
    // Do While Loop
    case NodeKind::DO_WHILE: {
      auto* n = as<DoWhileNode>(node);
      checkNode(n->body.get());
      requireBool(n->cond.get(), checkExpression(n->cond.get()),
                  "Do-while condition");
      break;
    }
      // Function Condition
    // Function Condition
    case NodeKind::FUNCTION: {
      auto* n = as<FunctionNode>(node);

      // Duplicate check (item 5)
      string funcName = currentClass + "::" + n->id.full;
      if (!declaredFunctions.insert(funcName).second) {
        error(node, "Duplicate function definition: " + n->id.full);
        break;
      }

      // Save old state, then set up the new function context (item 6)
      SemanticType oldReturnType = currentReturnType;
      bool oldIn = inFunction;
      inFunction = true;
      currentReturnType = getType(n->id.type);
      if (currentReturnType == SemanticType::UNKNOWN) {
        error(node, "Unknown function return type");
        currentReturnType =
            SemanticType::ERROR;  // reported once; returns skip comparison
      }

      // Parameters and the function body share the same scope
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

      // Don't call checkNode() on the body: that would open an extra scope
      auto* body = as<BlockNode>(n->body.get());
      if (body) {
        for (const auto& stmt : body->stmts) {
          checkNode(stmt.get());
        }
      }
      exitScope();

      // Missing-return check (item 6), after the body has been checked
      if (currentReturnType != SemanticType::VOID &&
          currentReturnType != SemanticType::ERROR &&
          !alwaysReturns(n->body.get())) {
        error(node, "Not all code paths return a value in " + n->id.full);
      }

      // Restore the outer context
      inFunction = oldIn;
      currentReturnType = oldReturnType;
      break;
    }
    // Class Section
    case NodeKind::CLASS: {
      auto* n = as<ClassNode>(node);
      if (!declaredClasses.insert(n->id.name).second) {
        error(node, "Duplicate class: " + n->id.name);
        break;
      }
      string oldClass = currentClass;
      currentClass = n->id.name;
      enterScope();
      for (const auto& member : n->members) {
        checkNode(member.get());
      }
      exitScope();
      currentClass = oldClass;
      break;
    }
    // Other Expressions
    default: {
      checkExpression(node);
      break;
    }
  }
}
bool SemanticAnalyser::alwaysReturns(const Node* node) const {
  if (!node) return false;
  switch (node->kind) {
    case NodeKind::RETURN:
      return true;
    case NodeKind::BLOCK:
      for (const auto& s : as<BlockNode>(node)->stmts)
        if (alwaysReturns(s.get())) return true;
      return false;
    case NodeKind::IF: {
      auto* n = as<IfNode>(node);
      if (!n->elseBlock || !alwaysReturns(n->thenBlock.get())) return false;
      for (const auto& [c, b] : n->elseifs)
        if (!alwaysReturns(b.get())) return false;
      return alwaysReturns(n->elseBlock.get());
    }
    default:
      return false;
  }
}
// To do the full analysis, clear the vectors and variables and then run
// checkNode on program and return the errors if there are any.

bool SemanticAnalyser::analyse(const programNode& program) {
  errors.clear();
  scope_list.clear();
  declaredFunctions.clear();
  declaredClasses.clear();
  currentClass.clear();
  inFunction = false;
  currentReturnType = SemanticType::UNKNOWN;
  enterScope();
  checkNode(&program);
  exitScope();
  return !hasErrors();
}