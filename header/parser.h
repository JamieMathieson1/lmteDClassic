#pragma once
#include <bits/stdc++.h>

#include "Tokenizer.h"
#include "Tree.h"

using NodePtr = unique_ptr<Node>;
/*
Reads in from a token stucture, goes down using a tree to generate the code.
This is how it parses the expression. Recursive consutrctioN of the code.

Reads in a token by calling .next onto the tokenizer class.
*/
// This is for error spitting out jazz yayayayayaya.
class parseError : public runtime_error {
 public:
  parseError(const string msg, int line, int col)
      : runtime_error("Parse error at " + to_string(line) + ":" +
                      to_string(col) + ": " + msg) {}
};
using namespace std;


class parser {
 private:
  // vars.
  std::vector<Token> tokens;
  size_t position = 0;
  vector<unordered_set<string>> scopes;
  bool suppressDecl = false;
  // Finders and ifno getters.
  Token& cur();

  bool isDone();
  // checker
  Token& futureVision(size_t ahead);
  bool is(TokenType type, string& text);
  Token& expect(TokenType type, string text);
  void fail(string msg);
  bool isOp(string op);

  // The tree.
  NodePtr parseItem();
  NodePtr parseFunction(bool isVirtual = false, bool isAbstract = false);
  NodePtr parseClass();
  NodePtr parseStatement();
  NodePtr parseIf();

  // scopes
  void pushScope() { scopes.emplace_back(); }
  void popScope() { scopes.pop_back(); }
  bool declare(const string& fullName);
  size_t matchingBracket(size_t open);
  // parsers.
   NodePtr parseBlock(bool newScope = true);
    NodePtr parseDoWhile();
  NodePtr parseReturn();
  NodePtr parseWhile();
  NodePtr parseFor();
  NodePtr parseExpr();
  // expression rules.
  NodePtr parseAssign();
  NodePtr parseBinary(int level);
  NodePtr parseUnary();
  NodePtr parsePostfix();
  NodePtr parsePrimary();
  NodePtr parsePower();
  NodePtr parseCallOrName();
  vector<NodePtr> parseArgs();
  bool isFunctionDef();
  bool isK(string keyWord);
  bool isP(string punctuation);

 public:
  parser(Tokenizer& tokens);
  ~parser();
  NodePtr parse();
};