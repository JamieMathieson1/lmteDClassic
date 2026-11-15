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
TypeInfo SemanticAnalyser::getTypeInfo(const Ident& id) {
  TypeInfo t;
  if (id.kind == 'A') {
    t.kind = SemanticType::ARRAY;
    t.elem = getType(id.type);
    return t;
  }
  t.kind = getType(id.type);
  if (t.kind == SemanticType::CLASS) {
    t.className = id.type;
  }
  return t;
}

bool SemanticAnalyser::validVarType(const TypeInfo& t) const {
  if (t.kind == SemanticType::UNKNOWN || t.kind == SemanticType::VOID)
    return false;
  if (t.kind == SemanticType::ARRAY &&
      (t.elem == SemanticType::UNKNOWN || t.elem == SemanticType::VOID))
    return false;
  return true;
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
TypeInfo SemanticAnalyser::checkExpression(const Node* node) {
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

      // Bare declaration with no initialiser, e.g. V_int_age/
      if (n->isDecl) {
        TypeInfo declared = getTypeInfo(n->id);

        if (!validVarType(declared)) {
          error(node, "Invalid variable type");
          return SemanticType::ERROR;
        }

        Symbol symbol;
        symbol.name = n->id.full;
        symbol.type = declared;

        if (!declare(symbol)) {
          error(node, "Duplicate declaration: " + n->id.full);
          return SemanticType::ERROR;
        }

        return declared;
      }
      if (n->heapRightAss) {  // constructor side of `beg`, not a variable use
        for (const auto& a : n->initArgs) checkExpression(a.get());
        return getTypeInfo(n->id);
      }
      Symbol* symbol = lookup(n->id.full);
      if (!symbol) {
        error(node, "Unknown or undeclared variable: " + n->id.full);
        return SemanticType::ERROR;
      }
      if (symbol->freed) {
        error(node, "Use after donate: " + n->id.full);
        return SemanticType::ERROR;
      }
      return symbol->type;
    }  // the case of a binary node, call the helper checkBinary with the lhs
       // and rhs of the expression
    case NodeKind::BINARY: {
      auto* n = as<BinaryNode>(node);
      TypeInfo lhs = checkExpression(n->lhs.get());
      TypeInfo rhs = checkExpression(n->rhs.get());

      return checkBinary(n->op, lhs, rhs, node);
    }
    // Checking for the case of unary nodes
    case NodeKind::UNARY: {
      auto* n = as<UnaryNode>(node);

      if (n->op == "donate") {
        const NameNode* v = n->operand && n->operand->kind == NodeKind::NAME
                                ? as<NameNode>(n->operand.get())
                                : nullptr;
        if (!v) {
          error(node, "'donate' operand must be a variable.");
          return SemanticType::ERROR;
        }
        Symbol* sym = lookup(v->id.full);
        if (!sym) {
          error(node, "Unknown variable in donate: " + v->id.full);
          return SemanticType::ERROR;
        }
        if (!sym->isHeap) {
          error(node, "Cannot donate non-heap variable: " + v->id.full);
          return SemanticType::ERROR;
        }
        if (sym->freed) {
          error(node, "Double donate: " + v->id.full);
          return SemanticType::ERROR;
        }
        sym->freed = true;
        return SemanticType::VOID;
      }

      TypeInfo operand = checkExpression(n->operand.get());
      if (operand == SemanticType::ERROR) return SemanticType::ERROR;

      if (n->op == "!") {
        if (operand == SemanticType::BOOL) return SemanticType::BOOL;
        error(node, "'!' requires a boolean operand");
        return SemanticType::ERROR;
      }
      if (n->op == "+" || n->op == "-") {
        if (operand == SemanticType::INT || operand == SemanticType::FLOAT)
          return operand;
        error(node, "Unary '" + n->op + "' requires a numeric operand");
        return SemanticType::ERROR;
      }
      if (n->op == "beg") return operand;
      error(node, "Unknown unary operator: " + n->op);
      return SemanticType::ERROR;
    }  // Assignment logic cases will be handled in the checkAssignment helper
    // function
    case NodeKind::ASSIGN: {
      auto* n = as<AssignNode>(node);
      return checkAssignment(n);
    }
      // The analyser checks if the call if for a function or a class with the
      // callee.kind value It then returns the respective type for the function
      // or the class type The analyser then loops through the arguments
      // supplied to the function call
    case NodeKind::CALL: {
      auto* n = as<CallNode>(node);
      if (n->callee.kind == 'F') {
        Symbol* f = lookup(n->callee.full);
        if (!f || !f->isFunction) {
          error(node, "Undefined function: " + n->callee.full);
          for (const auto& a : n->args) checkExpression(a.get());
          return SemanticType::ERROR;
        }
        if (!checkArgs(*f, n->args, node)) return SemanticType::ERROR;
        return f->type;
      }
      if (n->callee.kind == 'C') {
        for (const auto& a : n->args)
          if (checkExpression(a.get()) == SemanticType::ERROR)
            return SemanticType::ERROR;
        return TypeInfo(
            SemanticType::CLASS,
            n->callee.name);  // className is needed for V_Dog_d = C_Cat[]
      }
      error(node, "Invalid function or constructor call");
      return SemanticType::ERROR;
    }
      // If its an array literal case, first we need to check if its empty. If
      // it is, we return array semantic type Next we need to check the first
      // element type, as that is the expecting type for all other elements in
      // the array If any other element in the array is not the expected type
      // we return an error otherwise we return array type
    case NodeKind::ARRAY_LITERAL: {
      auto* n = as<ArrayLiteralNode>(node);
      if (n->elems.empty()) {
        TypeInfo arr(SemanticType::ARRAY);
        arr.elem = SemanticType::UNKNOWN;
        return arr;
      }
      TypeInfo first = checkExpression(n->elems[0].get());
      if (first.kind == SemanticType::ERROR) {
        return TypeInfo(SemanticType::ERROR);
      }
      for (size_t i = 1; i < n->elems.size(); i++) {
        TypeInfo current = checkExpression(n->elems[i].get());
        if (current.kind == SemanticType::ERROR) {
          return TypeInfo(SemanticType::ERROR);
        }
        if (current.kind != first.kind) {
          error(node, "Array elements must have matching types.");
          return TypeInfo(SemanticType::ERROR);
        }
      }
      TypeInfo arr(SemanticType::ARRAY);
      arr.elem = first.kind;
      return arr;
    }  // Array Index Nodes Case
    case NodeKind::INDEX: {
      auto* n = as<IndexNode>(node);
      TypeInfo base = checkExpression(n->base.get());
      TypeInfo index = checkExpression(n->index.get());
      if (base.kind == SemanticType::ERROR ||
          index.kind == SemanticType::ERROR) {
        return SemanticType::ERROR;
      }
      if (index != SemanticType::INT) {
        error(node, "Array index must be an integer.");
        return SemanticType::ERROR;
      }
      if (base.kind != SemanticType::ARRAY) {
        error(node, "Cannot index a non-array value.");
        return SemanticType::ERROR;
      }
      return base.elem;  // Correctly returns element type!
    }
      // Member Access Case
    case NodeKind::MEMBER: {
      auto* n = as<MemberNode>(node);
      TypeInfo object = checkExpression(n->object.get());
      if (object == SemanticType::ERROR) return SemanticType::ERROR;
      if (object != SemanticType::CLASS) {
        error(node, "Member access '.' requires a class object.");
        return SemanticType::ERROR;
      }
      const Node* m = n->member.get();
      if (m->kind == NodeKind::CALL) {
        auto* c = as<CallNode>(m);
        Symbol* f = findMember(object.className, c->callee.full);
        if (!f || !f->isFunction) {
          error(node, "Class " + object.className + " has no method " +
                          c->callee.full);
          for (const auto& a : c->args) checkExpression(a.get());
          return SemanticType::ERROR;
        }
        if (!checkArgs(*f, c->args, m)) return SemanticType::ERROR;
        return f->type;
      }
      if (m->kind == NodeKind::NAME) {
        auto* nm = as<NameNode>(m);
        Symbol* f = findMember(object.className, nm->id.full);
        if (!f) {
          error(node,
                "Class " + object.className + " has no member " + nm->id.full);
          return SemanticType::ERROR;
        }
        return f->type;
      }
      error(node, "Invalid member access");
      return SemanticType::ERROR;
    }
    default:
      error(node, "Invalid expression node");
      return SemanticType::UNKNOWN;
  }
}
Symbol* SemanticAnalyser::findMember(const string& cls, const string& member) {
  vector<string> stack{cls};
  unordered_set<string> seen;
  auto& global = scope_list.front();
  while (!stack.empty()) {
    string c = stack.back();
    stack.pop_back();
    if (!seen.insert(c).second) continue;
    auto it = global.find(c + "." + member);
    if (it != global.end()) return &it->second;
    auto b = classBases.find(c);
    if (b != classBases.end())
      for (const auto& x : b->second) stack.push_back(x);
  }
  return nullptr;
}
// This function checks the binary cases
TypeInfo SemanticAnalyser::checkBinary(const string& op, const TypeInfo& lhs,
                                       const TypeInfo& rhs, const Node* node) {
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
void SemanticAnalyser::requireBool(const Node* at, const TypeInfo& t,
                                   const string& what) {
  if (t.kind != SemanticType::BOOL && t.kind != SemanticType::ERROR) {
    error(at, what + " must be boolean.");
  }
}
static bool isBegNode(const Node* v) {
  return v && v->kind == NodeKind::UNARY && as<UnaryNode>(v)->op == "beg";
}

TypeInfo SemanticAnalyser::checkAssignment(const AssignNode* node) {
  if (!node->target || !node->value) return SemanticType::ERROR;
  const NameNode* target = node->target->kind == NodeKind::NAME
                               ? as<NameNode>(node->target.get())
                               : nullptr;

  TypeInfo rhs = checkExpression(node->value.get());
  if (rhs == SemanticType::ERROR) return SemanticType::ERROR;

  if (target && target->isDecl) {
    if (node->op != "=") {
      error(node, "Declarations must use '='");
      return SemanticType::ERROR;
    }
    TypeInfo declared = getTypeInfo(target->id);
    if (!validVarType(declared)) {
      error(node, "Invalid variable type");
      return SemanticType::ERROR;
    }
    if (!assignable(declared, rhs)) {
      error(node, "Cannot assign value of incompatible type.");
      return SemanticType::ERROR;
    }
    Symbol sym;
    sym.name = target->id.full;
    sym.type = declared;
    sym.isHeap = isBegNode(node->value.get());
    if (!declare(sym)) {
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

  TypeInfo lhs;
  Symbol* sym = nullptr;
  if (target) {  // direct lookup so re-assigning a donated variable is legal
    sym = lookup(target->id.full);
    if (!sym) {
      error(node, "Unknown or undeclared variable: " + target->id.full);
      return SemanticType::ERROR;
    }
    lhs = sym->type;
  } else {
    lhs = checkExpression(node->target.get());
    if (lhs == SemanticType::ERROR) return SemanticType::ERROR;
  }

  if (node->op == "=") {
    if (!assignable(lhs, rhs)) {
      error(node, "Assignment type mismatch");
      return SemanticType::ERROR;
    }
    if (sym) {
      sym->isHeap = isBegNode(node->value.get());
      sym->freed = false;
    }
    return lhs;
  }

  string binaryOp = node->op.substr(0, node->op.size() - 1);
  TypeInfo result = checkBinary(binaryOp, lhs, rhs, node);
  if (result == SemanticType::ERROR) return SemanticType::ERROR;
  if (result != SemanticType::UNKNOWN && result.kind != lhs.kind &&
      !(lhs.kind == SemanticType::FLOAT && result == SemanticType::INT)) {
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
      TypeInfo actual = SemanticType::VOID;
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
      if (!assignable(currentReturnType, actual)) {
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
    case NodeKind::FUNCTION: {
      auto* n = as<FunctionNode>(node);

      string funcName = currentClass + "::" + n->id.name;
      if (!declaredFunctions.insert(funcName).second) {
        error(node, "Duplicate function definition: " + n->id.name);
        break;
      }

      TypeInfo oldReturnType = currentReturnType;
      bool oldIn = inFunction;
      inFunction = true;
      currentReturnType = getTypeInfo(n->id);
      if (currentReturnType == SemanticType::UNKNOWN) {
        error(node, "Unknown function return type");
        currentReturnType = SemanticType::ERROR;
      }
      if (!currentClass.empty()) declare(makeFunctionSymbol(n));

      enterScope();
      for (const auto& param : n->params) {
        auto* p = as<NameNode>(param.get());
        if (!p) continue;
        TypeInfo paramType = getTypeInfo(p->id);
        if (!validVarType(paramType)) {
          error(param.get(), "Invalid parameter type.");
          continue;
        }
        Symbol symbol;
        symbol.name = p->id.full;
        symbol.type = paramType;
        if (!declare(symbol)) error(param.get(), "Duplicate parameter.");
      }

      auto* body = n->body ? as<BlockNode>(n->body.get()) : nullptr;
      if (body)
        for (const auto& stmt : body->stmts) checkNode(stmt.get());
      exitScope();

      if (!n->isAbstract && n->body &&
          currentReturnType != SemanticType::VOID &&
          currentReturnType != SemanticType::ERROR &&
          !alwaysReturns(n->body.get()))
        error(node, "Not all code paths return a value in " + n->id.full);

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
      for (const Ident& b : n->bases) {
        if (!declaredClasses.count(b.name))
          error(node,
                "Class " + n->id.name + " worships unknown class " + b.name);
        classBases[n->id.name].push_back(b.name);
      }
      string oldClass = currentClass;
      currentClass = n->id.name;
      enterScope();
      for (const auto& member : n->members) checkNode(member.get());
      // publish members as "Class.member" in the global scope for findMember
      for (const auto& [name, sym] : scope_list.back())
        scope_list.front()[n->id.name + "." + name] = sym;
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
  classBases.clear();
  currentClass.clear();
  inFunction = false;
  currentReturnType = TypeInfo();
  enterScope();
  for (const auto& item : program.items)  // pre-declare top-level functions
    if (item->kind == NodeKind::FUNCTION)
      declare(makeFunctionSymbol(as<FunctionNode>(item.get())));
  checkNode(&program);
  exitScope();
  return !hasErrors();
}

bool SemanticAnalyser::isSubclass(const string& derived,
                                  const string& base) const {
  vector<string> stack{derived};
  unordered_set<string> seen;
  while (!stack.empty()) {
    string c = stack.back();
    stack.pop_back();
    if (c == base) return true;
    if (!seen.insert(c).second) continue;
    auto it = classBases.find(c);
    if (it != classBases.end())
      for (const auto& b : it->second) stack.push_back(b);
  }
  return false;
}

bool SemanticAnalyser::assignable(const TypeInfo& to,
                                  const TypeInfo& from) const {
  if (to.kind == SemanticType::UNKNOWN || from.kind == SemanticType::UNKNOWN)
    return true;
  if (to.kind == SemanticType::FLOAT && from.kind == SemanticType::INT)
    return true;
  if (to.kind != from.kind) return false;
  if (to.kind == SemanticType::CLASS)
    return to.className == from.className ||
           isSubclass(from.className, to.className);
  if (to.kind == SemanticType::ARRAY)
    return to.elem == SemanticType::UNKNOWN ||
           from.elem == SemanticType::UNKNOWN || to.elem == from.elem;
  return true;
}

Symbol SemanticAnalyser::makeFunctionSymbol(const FunctionNode* f) {
  Symbol s;
  s.name = f->id.full;
  s.isFunction = true;
  s.type = getTypeInfo(f->id);
  for (const auto& p : f->params)
    if (p->kind == NodeKind::NAME)
      s.params.push_back(getTypeInfo(as<NameNode>(p.get())->id));
  return s;
}

bool SemanticAnalyser::checkArgs(const Symbol& f, const vector<NodePtr>& args,
                                 const Node* at) {
  vector<TypeInfo> types;
  bool ok = true;
  for (const auto& a : args) {
    types.push_back(checkExpression(a.get()));
    if (types.back() == SemanticType::ERROR) ok = false;
  }
  if (!ok) return false;
  if (types.size() != f.params.size()) {
    error(at, f.name + " expects " + to_string(f.params.size()) +
                  " argument(s), got " + to_string(types.size()));
    return false;
  }
  for (size_t i = 0; i < types.size(); i++)
    if (!assignable(f.params[i], types[i])) {
      error(args[i].get(), "Argument " + to_string(i + 1) +
                               " type mismatch in call to " + f.name);
      ok = false;
    }
  return ok;
}