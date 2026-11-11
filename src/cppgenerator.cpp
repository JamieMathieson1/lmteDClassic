#include "../header/CppGenerator.h"
#include "../header/Tree.h"

void CppGenerator::generate(programNode& p) {
    for (const NodePtr& next : p.items) {
        code.append(next.get()->callVisit(*this));
    }
}

// top level

string CppGenerator::visit(const FunctionNode& n) { 
    string header = n.id.type + " " + n.id.name + '(';
    for (const NodePtr& args : n.params) {
        header.append(args.get()->callVisit(*this) + ',');
    }
    header.pop_back(); // remove extra , 
    header.push_back(')');
    string body = n.body.get()->callVisit(*this);
    return header + body; 
}
// inheritance is always public
string CppGenerator::visit(const ClassNode& n) { 
    string cls = "class " + n.id.name;
    if (n.bases.size() > 0) {
        cls.append(": ");
        for (const Ident& i : n.bases) {
            cls.append(" public " + i.name + ", ");
        }
        cls.pop_back(); cls.pop_back(); // remove the extra , 
    }
    cls.append("{\n");
    for (const NodePtr& mem : n.members) {
        cls.append(mem.get()->callVisit(*this));
    }
    cls.append("\n};");
 }

// statements
// block has list of statements 
string CppGenerator::visit(const BlockNode& n) { 
    string block = "{\n";
    for (const NodePtr& st : n.stmts) {
        block.append(st.get()->callVisit(*this));
    }
    block.append("\n} ");
    return block;
}
string CppGenerator::visit(const ExprStmtNode& n) { 
    return n.expr.get()->callVisit(*this) + ';';
 }

string CppGenerator::visit(const ReturnNode& n) { 
    return "return " + n.value.get()->callVisit(*this);
 }

string CppGenerator::visit(const IfNode& n) { 
    string res = "if (" + n.cond.get()->callVisit(*this) + ")";
    res.append(n.thenBlock.get()->callVisit(*this));
    for (const auto& [cond, blk] : n.elseifs) {
        res.append("else if (" + cond.get()->callVisit(*this) + ")");
        res.append(blk.get()->callVisit(*this));
    }
    // else 
    if (n.elseBlock) {
        res.append("else " + n.elseBlock.get()->callVisit(*this));
    }
    return res;
}
string CppGenerator::visit(const ForNode& n) { 
    string res = "for (";
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
    return "while (" + n.cond.get()->callVisit(*this) + ")" +
           n.body.get()->callVisit(*this);
 }
string CppGenerator::visit(const DoWhileNode& n) { 
    return "do " + n.body.get()->callVisit(*this) +
           "while (" + n.cond.get()->callVisit(*this) + ");";
 }

// expressions
string CppGenerator::visit(const AssignNode& n) { 
    return n.target.get()->callVisit(*this) + " " + n.op + " " + n.value.get()->callVisit(*this);
}
string CppGenerator::visit(const BinaryNode& n) { 
    return n.lhs.get()->callVisit(*this) + " " + n.op + " " + n.rhs.get()->callVisit(*this);
}
string CppGenerator::visit(const UnaryNode& n) {
    string o = n.op; 
    if (n.op == "beg") o = "new ";
    else if (n.op == "donate") o = "delete ";
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
    return n.base.get()->callVisit(*this) + '[' + n.index.get()->callVisit(*this) + ']';
}
string CppGenerator::visit(const MemberNode& n) { 
    return n.object.get()->callVisit(*this) + '.' + n.member.get()->callVisit(*this);
 }
string CppGenerator::visit(const NameNode& n) { 
    if (n.isDecl) return n.id.type + " " + n.id.name;
    return n.id.name;  
 }
string CppGenerator::visit(const LiteralNode& n) { 
    if (n.litKind == LitKind::STRING) {
        return n.text;
    }
    string t = n.text;
    for_each(t.begin(), t.end(), [](char& x) {x = tolower(x);});
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