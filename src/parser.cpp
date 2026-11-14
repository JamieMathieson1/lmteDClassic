#include "../header/parser.h"

#include "../header/Tree.h"

parser::parser(Tokenizer& t) {
  while (t.canPoll()) tokens.push_back(t.poll());
}

bool parser::isDone() { return position >= tokens.size(); }

Token& parser::cur() {
  if (isDone()) fail("unexpected end of input");
  return tokens[position];
}

Token& parser::futureVision(size_t forward) {
  if (position + forward >= tokens.size())
    fail("The tokens ran out (too far ahead).");
  return tokens[position + forward];
}

bool parser::is(TokenType type, string& text) {
  return !isDone() && tokens[position].getType() == type &&
         tokens[position].getContent() == text;
}

Token& parser::expect(TokenType type, string text) {
  if (!is(type, text)) fail("expected: " + text + "");
  return tokens[position++];
}

void parser::fail(string msg) {
  if (tokens.empty()) throw parseError(msg, 1, 1);
  if (isDone()) {
    const Token& last = tokens.back();
    throw parseError(msg, last.getLine(), last.getCharNum());
  }
  const Token& t = tokens[position];
  throw parseError(msg + " (found '" + t.getContent() + "')", t.getLine(),
                   t.getCharNum());
}

bool parser::declare(const string& fullName) {
  for (auto& s : scopes)
    if (s.count(fullName)) return false;
  scopes.back().insert(fullName);
  return true;
}

size_t parser::matchingBracket(size_t open) {
  int depth = 0;
  for (size_t i = open; i < tokens.size(); i++) {
    if (tokens[i].getType() != TokenType::PUNCTUATOR) continue;
    if (tokens[i].getContent() == "[")
      depth++;
    else if (tokens[i].getContent() == "]" && --depth == 0)
      return i;
  }
  throw parseError("unclosed '['", tokens[open].getLine(),
                   tokens[open].getCharNum());
}

// Yoinks the node and pushes.
NodePtr parser::parse() {
  auto prog = make_unique<programNode>();
  pushScope();  // global scope
  while (!isDone()) prog->items.push_back(parseItem());
  popScope();
  return prog;
}

// Helps parse class.
NodePtr parser::parseItem() {
  if (isK("lowkey") || isK("highkey"))
    fail("function modifiers are only valid on class member functions");
  if (cur().getType() == TokenType::IDENTIFIER) {
    if (isFunctionDef()) return parseFunction();
    // C_Name d ...   or   C_Name worships ...
    if (cur().getContent()[0] == 'C' && position + 1 < tokens.size() &&
        ((tokens[position + 1].getType() == TokenType::PUNCTUATOR &&
          tokens[position + 1].getContent() == "d") ||
         (tokens[position + 1].getType() == TokenType::KEYWORD &&
          tokens[position + 1].getContent() == "worships")))
      return parseClass();
  }
  return parseStatement();
}

// Helps to parsethe function arguments.
NodePtr parser::parseFunction(bool isVirtual, bool isAbstract) {
  auto fn = make_unique<FunctionNode>(cur());
  fn->id = Ident::decode(cur().getContent());
  fn->isVirtual = isVirtual;
  fn->isAbstract = isAbstract;
  position++;
  expect(TokenType::PUNCTUATOR, "[");

  pushScope();  // parameters live in the body's scope
  if (isK("void") && futureVision(1).getContent() == "]") {
    position++;
  } else if (!isP("]")) {
    while (true) {
      const Token& t = cur();
      if (t.getType() != TokenType::IDENTIFIER ||
          (t.getContent()[0] != 'V' && t.getContent()[0] != 'A'))
        fail("expected a parameter (V_ or A_ name)");
      auto p = make_unique<NameNode>(t);
      p->id = Ident::decode(t.getContent());
      p->isDecl = declare(p->id.full);
      if (!p->isDecl) fail("duplicate parameter");
      fn->params.push_back(std::move(p));
      position++;
      if (isP(",")) {
        position++;
        continue;
      }
      break;
    }
  }
  expect(TokenType::PUNCTUATOR, "]");
  if (isAbstract) {
    expect(TokenType::PUNCTUATOR, "/");
  } else {
    fn->body = parseBlock(false);  // same scope as the parameters
  }
  popScope();
  return fn;
}

// C_Dog d ... b        C_Dog worships C_Animal, C_Pet d ... b
NodePtr parser::parseClass() {
  auto cls = make_unique<ClassNode>(cur());
  cls->id = Ident::decode(cur().getContent());
  position++;
  if (isK("worships")) {
    position++;
    while (true) {
      if (cur().getType() != TokenType::IDENTIFIER ||
          cur().getContent()[0] != 'C')
        fail("expected a class name (C_...) after 'worships'");
      cls->bases.push_back(Ident::decode(cur().getContent()));
      position++;
      if (isP(",")) {
        position++;
        continue;
      }
      break;
    }
  }
  expect(TokenType::PUNCTUATOR, "d");
  pushScope();  // members get their own scope
  while (!isP("b")) {
    if (isDone()) fail("unterminated class, expected 'b'");
    if (isK("lowkey") || isK("highkey")) {
      bool isVirtual = isK("lowkey");
      position++;
      if (isK("lowkey") || isK("highkey"))
        fail("function modifiers cannot be combined");
      if (isDone() || cur().getType() != TokenType::IDENTIFIER ||
          cur().getContent()[0] != 'F')
        fail("expected a function after '" +
             string(isVirtual ? "lowkey" : "highkey") + "'");
      cls->members.push_back(parseFunction(isVirtual, !isVirtual));
    } else {
      cls->members.push_back(parseItem());
    }
  }
  popScope();
  expect(TokenType::PUNCTUATOR, "b");
  return cls;
}

// Statements
NodePtr parser::parseBlock(bool newScope) {
  auto blk = make_unique<BlockNode>(cur());
  expect(TokenType::PUNCTUATOR, "d");
  if (newScope) pushScope();
  while (!isP("b")) {
    if (isDone()) fail("unterminated block, expected 'b'");
    blk->stmts.push_back(parseItem());
  }
  if (newScope) popScope();
  expect(TokenType::PUNCTUATOR, "b");
  return blk;
}

NodePtr parser::parseStatement() {
  if (isK("if")) return parseIf();
  if (isK("condition")) return parseFor();
  if (isK("tested")) return parseWhile();
  if (isK("testing")) return parseDoWhile();
  if (isK("return")) return parseReturn();
  if (isK("elseif") || isK("catch"))
    fail("'" + cur().getContent() + "' without a matching 'if'");
  if (isP("d") || isP("b")) fail("unexpected '" + cur().getContent() + "'");

  auto st = make_unique<ExprStmtNode>(cur());
  st->expr = parseExpr();
  expect(TokenType::PUNCTUATOR, "/");
  return st;
}

// if[cond] d..b  (elseif[cond] d..b)*  (catch d..b)?
NodePtr parser::parseIf() {
  auto node = make_unique<IfNode>(cur());
  position++;
  expect(TokenType::PUNCTUATOR, "[");
  node->cond = parseExpr();
  expect(TokenType::PUNCTUATOR, "]");
  node->thenBlock = parseBlock();
  while (isK("elseif")) {
    position++;
    expect(TokenType::PUNCTUATOR, "[");
    NodePtr c = parseExpr();
    expect(TokenType::PUNCTUATOR, "]");
    node->elseifs.emplace_back(std::move(c), parseBlock());
  }
  if (isK("catch")) {
    position++;
    node->elseBlock = parseBlock();
  }
  return node;
}

// for loop tester, let's there be empty stuff.
NodePtr parser::parseFor() {
  auto node = make_unique<ForNode>(cur());
  position++;
  expect(TokenType::PUNCTUATOR, "[");
  pushScope();
  // checks for internal dec.
  if (!isP(",")) node->init = parseExpr();
  expect(TokenType::PUNCTUATOR, ",");
  if (!isP(",")) node->test = parseExpr();
  expect(TokenType::PUNCTUATOR, ",");
  if (!isP("]")) node->step = parseExpr();
  expect(TokenType::PUNCTUATOR, "]");
  node->body = parseBlock();
  popScope();
  return node;
}

// tested[cond] d..b
NodePtr parser::parseWhile() {
  auto node = make_unique<WhileNode>(cur());
  position++;
  expect(TokenType::PUNCTUATOR, "[");
  node->cond = parseExpr();
  expect(TokenType::PUNCTUATOR, "]");
  node->body = parseBlock();
  return node;
}

// testing d..b tested[cond]/
NodePtr parser::parseDoWhile() {
  auto node = make_unique<DoWhileNode>(cur());
  position++;
  node->body = parseBlock();
  expect(TokenType::KEYWORD, "tested");
  expect(TokenType::PUNCTUATOR, "[");
  node->cond = parseExpr();
  expect(TokenType::PUNCTUATOR, "]");
  expect(TokenType::PUNCTUATOR, "/");
  return node;
}

// return/    or    return expr/
NodePtr parser::parseReturn() {
  auto node = make_unique<ReturnNode>(cur());
  position++;
  if (!isP("/")) node->value = parseExpr();
  expect(TokenType::PUNCTUATOR, "/");
  return node;
}

// ===========================================================================
// expressions. Precedence, loosest -> tightest:
//   = += -= *= **= \= %= &= |= ^=     (right assoc)
//   |   ^   &   (== !=)   (< <= > >=)   (+ -)   (* \ %)
//   prefix: ! + - beg donate
//   **                                 (right assoc;  -a ** b  ==  -(a ** b))
//   postfix: [index]   ;member
// ===========================================================================

NodePtr parser::parseExpr() { 
  return parseAssign();
 }

static bool isAssignOp(const string& s) {
  return s == "=" || s == "+=" || s == "-=" || s == "*=" || s == "**=" ||
         s == "\\=" || s == "%=" || s == "&=" || s == "|=" || s == "^=";
}

NodePtr parser::parseAssign() {
  NodePtr lhs = parseBinary(0);
  if (!isDone() && cur().getType() == TokenType::OPERATOR &&
      isAssignOp(cur().getContent())) {
    auto node = make_unique<AssignNode>(cur());
    position++;
    node->target = std::move(lhs);
    node->value = parseAssign();  // right associative
    return node;
  }
  return lhs;
}

NodePtr parser::parseBinary(int level) {
  if (level >= (int)LEVELS.size()) return parseUnary();
  NodePtr lhs = parseBinary(level + 1);
  while (!isDone() && cur().getType() == TokenType::OPERATOR) {
    const string& op = cur().getContent();
    bool match = false;
    for (const string& o : LEVELS[level])
      if (o == op) match = true;
    if (!match) break;
    auto node = make_unique<BinaryNode>(cur());
    position++;
    node->lhs = std::move(lhs);
    node->rhs = parseBinary(level + 1);  // left associative
    lhs = std::move(node);
  }
  return lhs;
}

NodePtr parser::parseUnary() {
  bool prefixOp = !isDone() && cur().getType() == TokenType::OPERATOR &&
                  (cur().getContent() == "!" ||
                   cur().getContent() == "+" ||
                   cur().getContent() == "-");
  if (prefixOp || isK("beg") || isK("donate")) {
    auto node = make_unique<UnaryNode>(cur());
    position++;
    node->operand = parseUnary();
    return node;
  }
  return parsePower();
}

NodePtr parser::parsePower() {
  NodePtr base = parsePostfix();
  if (isOp("**")) {
    auto node = make_unique<BinaryNode>(cur());
    position++;
    node->lhs = std::move(base);
    node->rhs = parseUnary();  // right associative; allows  a ** -b
    return node;
  }
  return base;
}

NodePtr parser::parsePostfix() {
  NodePtr e = parsePrimary();
  while (true) {
    if (isP("[") && !as<CallNode>(e.get())) {
      // indexing:  A_int[3]_nums [ i ]
      auto node = make_unique<IndexNode>(cur());
      position++;
      node->base = std::move(e);
      node->index = parseExpr();
      expect(TokenType::PUNCTUATOR, "]");
      e = std::move(node);
    } else if (isP(";")) {
      // member access:  obj ; F_int_get[]   or   obj ; V_int_age
      auto node = make_unique<MemberNode>(cur());
      position++;
      if (isDone() || cur().getType() != TokenType::IDENTIFIER)
        fail("expected a member name after ';'");
      node->object = std::move(e);
      bool saved = suppressDecl;
      suppressDecl = true;  // member names are not new declarations
      node->member = parseCallOrName();
      suppressDecl = saved;
      e = std::move(node);
    } else {
      break;
    }
  }
  return e;
}

// F_x_y[args]  |  C_X[args] (constructor)  |  plain V_/A_/C_ name
NodePtr parser::parseCallOrName() {
  const Token& t = cur();
  Ident id = Ident::decode(t.getContent());
  bool bracketNext = position + 1 < tokens.size() &&
                     tokens[position + 1].getType() == TokenType::PUNCTUATOR &&
                     tokens[position + 1].getContent() == "[";

  if (id.kind == 'F' || (id.kind == 'C' && bracketNext)) {
    if (!bracketNext) fail("expected '[' after function name");
    auto call = make_unique<CallNode>(t);
    call->callee = id;
    position++;
    call->args = parseArgs();
    return call;
  }

  auto name = make_unique<NameNode>(t);
  name->id = id;
  if (!suppressDecl && (id.kind == 'V' || id.kind == 'A'))
    name->isDecl = declare(id.full);
  position++;
  return name;
}

vector<NodePtr> parser::parseArgs() {
  vector<NodePtr> args;
  expect(TokenType::PUNCTUATOR, "[");
  if (!isP("]")) {
    while (true) {
      args.push_back(parseExpr());
      if (isP(",")) {
        position++;
        continue;
      }
      break;
    }
  }
  expect(TokenType::PUNCTUATOR, "]");
  return args;
}

NodePtr parser::parsePrimary() {
  const Token& t = cur();

  if (t.getType() == TokenType::LITERAL) {
    auto lit = make_unique<LiteralNode>(t);
    const string& c = t.getContent();
    if (c == "True" || c == "False") {
      lit->litKind = LitKind::BOOL;
      lit->text = c;
    } else if (c[0] == '{') {
      lit->litKind = LitKind::STRING;
      lit->text = c.substr(1, c.size() - 2);
    } else if (c.find('.') != string::npos) {
      lit->litKind = LitKind::FLOAT;
      lit->text = c;
    } else {
      lit->litKind = LitKind::INT;
      lit->text = c;
    }
    position++;
    return lit;
  }

  if (t.getType() == TokenType::IDENTIFIER) return parseCallOrName();

  if (isP("(")) {
    // for arrays.
    auto arr = make_unique<ArrayLiteralNode>(t);
    position++;
    if (isP(")")) {
      position++;
      return arr;
    }
    NodePtr first = parseExpr();
    if (isP(",")) {
      arr->elems.push_back(std::move(first));
      while (isP(",")) {
        position++;
        arr->elems.push_back(parseExpr());
      }
      expect(TokenType::PUNCTUATOR, ")");
      return arr;
    }
    expect(TokenType::PUNCTUATOR, ")");
    return first;
  }

  fail("expected an expression");
  return nullptr;  // fail() always throws, this just silences the warning
}

bool parser::isK(string keyWord) {
  return !isDone() && tokens[position].getType() == TokenType::KEYWORD &&
         tokens[position].getContent() == keyWord;
}

bool parser::isP(string punctuation) {
  return !isDone() && tokens[position].getType() == TokenType::PUNCTUATOR &&
         tokens[position].getContent() == punctuation;
}

bool parser::isOp(string op) {
  return !isDone() && tokens[position].getType() == TokenType::OPERATOR &&
         tokens[position].getContent() == op;
}

// Same check as isFunction(): F_... [ ... ] d
// F_... [ ... ] d
bool parser::isFunctionDef() {
  if (isDone() || tokens[position].getType() != TokenType::IDENTIFIER)
    return false;
  if (tokens[position].getContent()[0] != 'F') return false;
  if (position + 1 >= tokens.size() || tokens[position + 1].getContent() != "[")
    return false;
  size_t close = matchingBracket(position + 1);
  return close + 1 < tokens.size() &&
         tokens[close + 1].getType() == TokenType::PUNCTUATOR &&
         tokens[close + 1].getContent() == "d";
}

parser::~parser() {}