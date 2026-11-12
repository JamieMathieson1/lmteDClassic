// test_codegen.cpp
#include <bits/stdc++.h>
#include "../../header/Tree.h"
#include "../../header/CppGenerator.h"
#include "../../header/Utils.h"
using namespace std;

Token tok(const string& text) {
  return Token(text, TokenType::IDENTIFIER, 1, 1);
}
// ---- builders ----
template <class... A> vector<NodePtr> vec(A&&... a) {
  vector<NodePtr> v; (v.push_back(move(a)), ...); return v;
}
NodePtr name(const string& s, bool decl = false) {
  auto n = make_unique<NameNode>(tok(s)); n->id = Ident::decode(s); n->isDecl = decl; return n;
}
NodePtr lit(const string& s, LitKind k = LitKind::INT) {
  auto n = make_unique<LiteralNode>(tok(s)); n->text = s; n->litKind = k; return n;
}
NodePtr bin(const string& op, NodePtr l, NodePtr r) {
  auto n = make_unique<BinaryNode>(tok(op)); n->lhs = move(l); n->rhs = move(r); return n;
}
NodePtr un(const string& op, NodePtr e) {
  auto n = make_unique<UnaryNode>(tok(op)); n->operand = move(e); return n;
}
NodePtr assign(const string& op, NodePtr t, NodePtr v) {
  auto n = make_unique<AssignNode>(tok(op)); n->target = move(t); n->value = move(v); return n;
}
NodePtr call(const string& f, vector<NodePtr> args) {
  auto n = make_unique<CallNode>(tok(f)); n->callee = Ident::decode(f); n->args = move(args); return n;
}
NodePtr arr(vector<NodePtr> e) {
  auto n = make_unique<ArrayLiteralNode>(tok("(")); n->elems = move(e); return n;
}
NodePtr idx(NodePtr b, NodePtr i) {
  auto n = make_unique<IndexNode>(tok("[")); n->base = move(b); n->index = move(i); return n;
}
NodePtr member(NodePtr o, NodePtr m) {
  auto n = make_unique<MemberNode>(tok(";")); n->object = move(o); n->member = move(m); return n;
}
NodePtr stmt(NodePtr e) {
  auto n = make_unique<ExprStmtNode>(tok("/")); n->expr = move(e); return n;
}
NodePtr ret(NodePtr e) {
  auto n = make_unique<ReturnNode>(tok("return")); n->value = move(e); return n;
}
NodePtr block(vector<NodePtr> s) {
  auto n = make_unique<BlockNode>(tok("d")); n->stmts = move(s); return n;
}
unique_ptr<FunctionNode> func(const string& f, vector<NodePtr> params, NodePtr body) {
  auto n = make_unique<FunctionNode>(tok(f)); n->id = Ident::decode(f);
  n->params = move(params); n->body = move(body); return n;
}

// ---- check helper ----
int fails = 0;
void expect(const string& out, const string& expected, const string& test) {
  cout << "=== " << test << " ===\n";
  if (out == expected) {
    cout << "  PASS\n\n";
    return;
  }
  fails++;
  cout << "  FAIL\n--- expected ---\n" << expected
       << "\n--- got ---\n" << out << "\n\n";
}

// ================= SMALL =================
// F_int_top[void] d  V_int_x = 5/  return V_int_x + 1/  b
void testSmall() {
  CppGenerator g;
  programNode prog;
  prog.items.push_back(func("F_int_top", {},
    block(vec(
      stmt(assign("=", name("V_int_x", true), lit("5"))),
      ret(bin("+", name("V_int_x"), lit("1")))
    ))));
  g.generate(prog);
  string expected = R"(int top() {
    int x = 5;
    return x + 1;
}
)";
  expect(Utils::trim(g.getCode()), Utils::trim(expected), "small");
}

// ================= MEDIUM =================
void testMedium() {
  CppGenerator g;
  programNode prog;

  auto cls = make_unique<ClassNode>(tok("C_Dog"));
  cls->id = Ident::decode("C_Dog");
  cls->members.push_back(stmt(name("V_int_age", true)));
  cls->members.push_back(func("F_int_add",
    vec(name("V_int_a", true), name("V_int_b", true)),
    block(vec(ret(bin("+", name("V_int_a"),
      bin("*", name("V_int_b"), bin("-", name("V_int_a"), lit("2")))))))));
  prog.items.push_back(move(cls));

  auto ifn = make_unique<IfNode>(tok("if"));
  ifn->cond = bin("<", name("V_int_i"), lit("3"));
  ifn->thenBlock = block(vec(stmt(assign("+=", name("V_int_i"), lit("1")))));
  ifn->elseifs.emplace_back(bin("==", name("V_int_i"), lit("3")),
                            block(vec(stmt(assign("=", name("V_int_i"), lit("0"))))));
  ifn->elseBlock = block(vec(stmt(assign("=", name("V_int_i"), un("-", lit("1"))))));

  auto fr = make_unique<ForNode>(tok("condition"));
  fr->init = assign("=", name("V_int_j", true), lit("1"));
  fr->test = bin("<", name("V_int_j"), lit("3"));
  fr->step = assign("=", name("V_int_j"), bin("+", name("V_int_j"), lit("1")));
  fr->body = block(vec(stmt(call("F_void_print", vec(lit("Abc", LitKind::STRING))))));

  auto wh = make_unique<WhileNode>(tok("tested"));
  wh->cond = bin("==", name("V_bool_loop"), lit("True", LitKind::BOOL));
  wh->body = block(vec(stmt(assign("=", name("V_bool_loop"), lit("False", LitKind::BOOL)))));

  auto dw = make_unique<DoWhileNode>(tok("testing"));
  dw->body = block(vec(stmt(assign("=", name("V_bool_loop"), lit("True", LitKind::BOOL)))));
  dw->cond = bin("==", name("V_bool_loop"), lit("True", LitKind::BOOL));

  prog.items.push_back(func("F_int_top", {}, block(vec(
    stmt(assign("=", name("V_int_i", true), lit("0"))),
    stmt(assign("=", name("V_bool_loop", true), lit("True", LitKind::BOOL))),
    stmt(assign("=", name("V_float_f", true), lit("123.45", LitKind::FLOAT))),
    stmt(assign("=", name("A_int[3]_arr", true), arr(vec(lit("1"), lit("2"), lit("3"))))),
    stmt(assign("=", name("V_int_k", true), idx(name("A_int[3]_arr"), lit("0")))),
    stmt(assign("=", name("V_bool_n", true), un("!", name("V_bool_loop")))),
    stmt(assign("=", name("V_int_r", true),
      member(call("C_Dog", vec()), call("F_int_add", vec(lit("3"), lit("4")))))),
    move(ifn), move(fr), move(wh), move(dw),
    ret(lit("0"))
  ))));

  g.generate(prog);
  string expected = R"(class Dog {
    int age;
    int add(int a, int b) {
        return a + b * (a - 2);
    }
};
int top() {
    int i = 0;
    bool loop = true;
    float f = 123.45;
    int arr[3] = {1, 2, 3};
    int k = arr[0];
    bool n = !loop;
    int r = Dog().add(3, 4);
    if (i < 3) {
        i += 1;
    }
    else if (i == 3) {
        i = 0;
    }
    else {
        i = -1;
    }
    for (int j = 1; j < 3; j = j + 1) {
        print("Abc");
    }
    while (loop == true) {
        loop = false;
    }
    do {
        loop = true;
    }
    while (loop == true);
    return 0;
}
)";
  expect(Utils::trim(g.getCode()), Utils::trim(expected), "medium");
}

int main() {
  testSmall();
  testMedium();
  cout << (fails ? "FAILED: " + to_string(fails) : string("ALL PASSED")) << "\n";
  return fails != 0;
}