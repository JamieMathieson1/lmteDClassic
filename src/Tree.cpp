#pragma once
#include "../header/Tree.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// MY NODESSSSSSS FINALLYL.
// ENUM FOR DIFFERENTIATION, CLASS TO AVOID SILLY BUSIENSSSS>

// Seperate identifiers. kind is the first letter, full is the original text.

// The debugger.
// Prints an indented tree for checking the parser's output.

inline void dumpTree(const Node* n, ostream& o = cout, int depth = 0) {
  auto line = [&](const string& s) {
    o << string(depth * 2, ' ') << s << "\n";
  };
  auto sub = [&](const char* label, const Node* c) {
    o << string((depth + 1) * 2, ' ') << label << ":\n";
    dumpTree(c, o, depth + 2);
  };
  if (!n) {
    line("(none)");
    return;
  }

  switch (n->kind) {
    case NodeKind::LITERAL: {
      auto* x = as<LiteralNode>(n);
      static const char* names[] = {"Int", "Float", "Bool", "String"};
      line(string("Literal<") + names[(int)x->litKind] + "> " + x->text);
      break;
    }
    case NodeKind::NAME: {
      auto* x = as<NameNode>(n);
      line((x->isDecl ? "Declare " : "Name ") + x->id.full);
      break;
    }
    case NodeKind::CALL: {
      auto* x = as<CallNode>(n);
      line("Call " + x->callee.full);
      for (auto& a : x->args) dumpTree(a.get(), o, depth + 1);
      break;
    }
    case NodeKind::ARRAY_LITERAL: {
      line("ArrayLiteral");
      for (auto& e : as<ArrayLiteralNode>(n)->elems)
        dumpTree(e.get(), o, depth + 1);
      break;
    }
    case NodeKind::INDEX: {
      auto* x = as<IndexNode>(n);
      line("Index");
      dumpTree(x->base.get(), o, depth + 1);
      dumpTree(x->index.get(), o, depth + 1);
      break;
    }
    case NodeKind::MEMBER: {
      auto* x = as<MemberNode>(n);
      line("Member");
      dumpTree(x->object.get(), o, depth + 1);
      dumpTree(x->member.get(), o, depth + 1);
      break;
    }
    case NodeKind::UNARY: {
      auto* x = as<UnaryNode>(n);
      line("Unary " + x->op);
      dumpTree(x->operand.get(), o, depth + 1);
      break;
    }
    case NodeKind::BINARY: {
      auto* x = as<BinaryNode>(n);
      line("Binary " + x->op);
      dumpTree(x->lhs.get(), o, depth + 1);
      dumpTree(x->rhs.get(), o, depth + 1);
      break;
    }
    case NodeKind::ASSIGN: {
      auto* x = as<AssignNode>(n);
      line("Assign " + x->op);
      dumpTree(x->target.get(), o, depth + 1);
      dumpTree(x->value.get(), o, depth + 1);
      break;
    }
    case NodeKind::BLOCK: {
      line("Block");
      for (auto& s : as<BlockNode>(n)->stmts) dumpTree(s.get(), o, depth + 1);
      break;
    }
    case NodeKind::EXPR_STMT: {
      line("ExprStmt");
      dumpTree(as<ExprStmtNode>(n)->expr.get(), o, depth + 1);
      break;
    }
    case NodeKind::RETURN: {
      line("Return");
      if (as<ReturnNode>(n)->value)
        dumpTree(as<ReturnNode>(n)->value.get(), o, depth + 1);
      break;
    }
    case NodeKind::IF: {
      auto* x = as<IfNode>(n);
      line("If");
      sub("cond", x->cond.get());
      sub("then", x->thenBlock.get());
      for (auto& e : x->elseifs) {
        sub("elseif cond", e.first.get());
        sub("elseif body", e.second.get());
      }
      if (x->elseBlock) sub("catch", x->elseBlock.get());
      break;
    }
    case NodeKind::FOR: {
      auto* x = as<ForNode>(n);
      line("Condition(for)");
      sub("init", x->init.get());
      sub("test", x->test.get());
      sub("step", x->step.get());
      dumpTree(x->body.get(), o, depth + 1);
      break;
    }
    case NodeKind::WHILE: {
      auto* x = as<WhileNode>(n);
      line("Tested(while)");
      sub("cond", x->cond.get());
      dumpTree(x->body.get(), o, depth + 1);
      break;
    }
    case NodeKind::DO_WHILE: {
      auto* x = as<DoWhileNode>(n);
      line("Testing(do-while)");
      dumpTree(x->body.get(), o, depth + 1);
      sub("cond", x->cond.get());
      break;
    }
    case NodeKind::FUNCTION: {
      auto* x = as<FunctionNode>(n);
      line("Function " + x->id.full);
      for (auto& p : x->params) dumpTree(p.get(), o, depth + 1);
      dumpTree(x->body.get(), o, depth + 1);
      break;
    }
    case NodeKind::CLASS: {
      auto* x = as<ClassNode>(n);
      string s = "Class " + x->id.full;
      for (size_t i = 0; i < x->bases.size(); i++)
        s += (i ? ", " : " worships ") + x->bases[i].full;
      line(s);
      for (auto& m : x->members) dumpTree(m.get(), o, depth + 1);
      break;
    }
    case NodeKind::PROGRAM: {
      line("Program");
      for (auto& i : as<programNode>(n)->items) dumpTree(i.get(), o, depth + 1);
      break;
    }
  }
}