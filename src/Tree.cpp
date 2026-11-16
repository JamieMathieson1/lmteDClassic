#include "../header/Tree.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// every node's visit functions implemented 
inline string LiteralNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string NameNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string CallNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string ArrayLiteralNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string IndexNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string MemberNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string UnaryNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string BinaryNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string AssignNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string BlockNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string ExprStmtNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string ReturnNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string IfNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string ForNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string WhileNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string DoWhileNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string FunctionNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string ClassNode::callVisit(CppGenerator& g) const { return g.visit(*this); }
inline string programNode::callVisit(CppGenerator& g) const { return g.visit(*this); }