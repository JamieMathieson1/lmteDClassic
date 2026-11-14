#include "../header/CppGenerator.h"

#include "../header/Tree.h"
#include "../header/Utils.h"

void CppGenerator::generate(const programNode& p) {
  code = move(Utils::trim(visit(p)));
}

string CppGenerator::visit(const programNode& p) {
  string cpp;
  for (const NodePtr& next : p.items) {
    cpp.append(next.get()->callVisit(*this));
  }
  return cpp;
}

// top level

string CppGenerator::visit(const FunctionNode& n) {
  string header = indent() + n.id.type + " " + n.id.name + '(';
  for (const NodePtr& args : n.params) {
    header.append(args.get()->callVisit(*this) + ", ");
  }
  if (!n.params.empty()) {
    header.pop_back();
    header.pop_back();
  }  // remove extra ,
  header.push_back(')');
  string body = n.body.get()->callVisit(*this);
  return header + body;
}
// inheritance is always public
string CppGenerator::visit(const ClassNode& n) {
  string cls = indent() + "class " + n.id.name;
  if (n.bases.size() > 0) {
    cls.append(" : ");
    for (const Ident& i : n.bases) {
      cls.append(" public " + i.name + ", ");
    }
    cls.pop_back();
    cls.pop_back();  // remove the extra ,
  }
  cls.append(" {\n");
  // implicit block here
  level++;
  for (const NodePtr& mem : n.members) {
    cls.append(mem.get()->callVisit(*this));
  }
  level--;
  cls.append(indent() + "};\n");
  return cls;
}

// statements
// block has list of statements
string CppGenerator::visit(const BlockNode& n) {
  string block = " {\n";
  level++;
  for (const NodePtr& st : n.stmts) {
    block.append(st.get()->callVisit(*this));
  }
  level--;
  block.append(indent() + "}\n");
  return block;
}
string CppGenerator::visit(const ExprStmtNode& n) {
  return indent() + n.expr.get()->callVisit(*this) + ";\n";
}

string CppGenerator::visit(const ReturnNode& n) {
  return indent() + "return " + n.value.get()->callVisit(*this) + ";\n";
}

string CppGenerator::visit(const IfNode& n) {
  string res = indent() + "if (" + n.cond.get()->callVisit(*this) + ")";
  res.append(n.thenBlock.get()->callVisit(*this));
  for (const auto& [cond, blk] : n.elseifs) {
    res.append(indent() + "else if (" + cond.get()->callVisit(*this) + ")");
    res.append(blk.get()->callVisit(*this));
  }
  // else
  if (n.elseBlock) {
    res.append(indent() + "else" + n.elseBlock.get()->callVisit(*this));
  }
  return res;
}
string CppGenerator::visit(const ForNode& n) {
  string res = indent() + "for (";
  if (n.init) res.append(n.init.get()->callVisit(*this));
  res.append("; ");
  if (n.test) res.append(n.test.get()->callVisit(*this));
  res.append("; ");
  if (n.step) res.append(n.step.get()->callVisit(*this));
  res.append(")");
  res.append(n.body.get()->callVisit(*this));
  return res;
}
string CppGenerator::visit(const WhileNode& n) {
  return indent() + "while (" + n.cond.get()->callVisit(*this) + ")" +
         n.body.get()->callVisit(*this);
}
string CppGenerator::visit(const DoWhileNode& n) {
  return indent() + "do" + n.body.get()->callVisit(*this) + indent() +
         "while (" + n.cond.get()->callVisit(*this) + ");\n";
}

// expressions
string CppGenerator::visit(const AssignNode& n) {
  string saved = pendingType;
  if (n.target->kind == NodeKind::NAME && n.value->kind == NodeKind::UNARY) {
    auto* name = as<NameNode>(n.target.get());
    auto* un = as<UnaryNode>(n.value.get());
    if (name->isDecl && un->op == "beg") {
      heapVars.insert(name->id.full);
      pendingType = name->id.type;
    }
  }
  string targetStr = n.target.get()->callVisit(*this);
  string valueStr = n.value.get()->callVisit(*this);
  pendingType = saved;
  return targetStr + " " + n.op + " " + valueStr;
}

int findLevel(const string& op) {
  int l = 0;
  for (auto& vec : LEVELS) {
    if (find(vec.begin(), vec.end(), op) != vec.end()) {
      return l;
    }
    l++;
  }
  return -1;
}

string CppGenerator::wrap(const NodeKind& kind, const string& printed,
                          int tightestP, int currLevel) {
  if (kind == NodeKind::UNARY) return printed;
  if (tightestP > currLevel) {
    // parent was tighter, so add brackets
    return "(" + printed + ")";
  } else
    return printed;
}

string CppGenerator::visit(const BinaryNode& n) {
  int currLevel = findLevel(n.op);
  int oldTight = tightestP;
  tightestP = max(tightestP, currLevel);

  string lhsStr = n.lhs.get()->callVisit(*this);
  string rhsStr = n.rhs.get()->callVisit(*this);

  string node = lhsStr + " " + n.op + " " + rhsStr;
  node = move(wrap(n.kind, node, oldTight, currLevel));
  tightestP = oldTight;
  return node;
}
string CppGenerator::visit(const UnaryNode& n) {
  if (n.op == "beg") {
    string t = pendingType.empty() ? "int" : pendingType;
    return "std::make_shared<" + t + ">(" + n.operand.get()->callVisit(*this) +
           ")";
  }
  if (n.op == "donate") {
    // use the raw name, not the dereferenced one
    if (n.operand->kind == NodeKind::NAME)
      return as<NameNode>(n.operand.get())->id.name + ".reset()";
    return n.operand.get()->callVisit(*this) + ".reset()";
  }
  return n.op + n.operand.get()->callVisit(*this);
}
string CppGenerator::visit(const CallNode& n) {
  string res = n.callee.name + "(";
  for (size_t i = 0; i < n.args.size(); ++i) {
    if (i > 0) res.append(", ");
    res.append(n.args[i].get()->callVisit(*this));
  }
  res.append(")");
  return res;
}
string CppGenerator::visit(const IndexNode& n) {
  return n.base.get()->callVisit(*this) + '[' +
         n.index.get()->callVisit(*this) + ']';
}
const Symbol* CppGenerator::lookupSymbol(const string& name) {
  if (!scope_list) return nullptr;
  for (auto it = scope_list->rbegin(); it != scope_list->rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      return &found->second;
    }
  }
  return nullptr;
}

string CppGenerator::visit(const MemberNode& n) {
  string objectStr = n.object.get()->callVisit(*this);
  string memberStr = n.member.get()->callVisit(*this);

  bool isHeapVar = false;
  if (n.object->kind == NodeKind::NAME) {
    auto* nameNode = as<NameNode>(n.object.get());
    const Symbol* sym = lookupSymbol(nameNode->id.full);
    if (sym && sym->isHeap) {
      isHeapVar = true;
    }
  }

  string accessor = isHeapVar ? "->" : ".";
  return objectStr + accessor + memberStr;
}

bool CppGenerator::isHeapName(const NameNode& n) {
  if (heapVars.count(n.id.full)) return true;
  const Symbol* sym = lookupSymbol(n.id.full);
  return sym && sym->isHeap;
}

string CppGenerator::visit(const NameNode& n) {
  bool heap = isHeapName(n);
  if (n.isDecl) {
    string t = heap ? "std::shared_ptr<" + n.id.type + ">" : n.id.type;
    return t + " " + n.id.name + n.id.length;
  }
  return heap ? "*" + n.id.name : n.id.name;
}
string CppGenerator::visit(const LiteralNode& n) {
  if (n.litKind == LitKind::STRING) {
    return "\"" + n.text + "\"";
  }
  string t = n.text;
  for_each(t.begin(), t.end(), [](char& x) { x = tolower(x); });
  return t;
}

string CppGenerator::visit(const ArrayLiteralNode& n) {
  string res = "{";
  for (size_t i = 0; i < n.elems.size(); ++i) {
    if (i > 0) res.append(", ");
    res.append(n.elems[i].get()->callVisit(*this));
  }
  res.append("}");
  return res;
}