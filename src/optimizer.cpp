#include "../header/optimizer.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <algorithm>

using namespace std;


// GENERAL HELPERS

namespace {


// Create a token for a newly generated AST node.
// Uses the Token constructor used by the existing AST.
Token syntheticToken(const string& text, int line, int col) {
    return Token(text, TokenType::LITERAL, line, col);
}


// Create a replacement literal node.
NodePtr makeLiteral(
    LitKind kind,
    const string& text,
    int line,
    int col
) {
    Token token = syntheticToken(text, line, col);

    auto result = make_unique<LiteralNode>(token);

    result->litKind = kind;
    result->text = text;

    return result;
}


// Replace an existing node with a literal.
void replaceLiteral(
    NodePtr& node,
    LitKind kind,
    const string& text
) {
    int line = node->line;
    int col = node->col;

    node = makeLiteral(kind, text, line, col);
}


// Get a variable's full identifier.
string getName(const NameNode* node) {
    if (!node)
        return "";

    if (!node->id.full.empty())
        return node->id.full;

    return node->id.name;
}


// Parse an integer literal safely.
bool parseInteger(const LiteralNode* node, long long& value) {

    if (!node || node->litKind != LitKind::INT)
        return false;

    try {
        size_t position = 0;

        value = stoll(node->text, &position);

        return position == node->text.size();
    }
    catch (...) {
        return false;
    }
}


// Parse a floating-point literal safely.
bool parseFloat(const LiteralNode* node, long double& value) {

    if (!node || node->litKind != LitKind::FLOAT)
        return false;

    try {
        size_t position = 0;

        value = stold(node->text, &position);

        return position == node->text.size()
            && isfinite(value);
    }
    catch (...) {
        return false;
    }
}


// The generated C++ uses int for DDD integer values.
bool fitsInt(long long value) {

    return value >= numeric_limits<int>::min()
        && value <= numeric_limits<int>::max();
}


// Format a floating-point result.
string formatFloat(long double value) {

    ostringstream output;

    output << setprecision(
        numeric_limits<long double>::max_digits10
    ) << value;

    string result = output.str();

    if (result.find_first_of(".eE") == string::npos)
        result += ".0";

    return result;
}


// Identify comparison operators.
bool isComparison(const string& op) {

    return op == "==" ||
           op == "!=" ||
           op == "<"  ||
           op == "<=" ||
           op == ">"  ||
           op == ">=";
}


// ============================================================
// INTEGER POWER
// ============================================================

bool integerPower(
    long long base,
    long long exponent,
    long long& result
) {

    if (exponent < 0)
        return false;

    result = 1;

    long long factor = base;

    while (exponent > 0) {

        if (exponent & 1) {

            if (__builtin_mul_overflow(
                result, factor, &result))
                return false;
        }

        exponent >>= 1;

        if (exponent > 0) {

            if (__builtin_mul_overflow(
                factor, factor, &factor))
                return false;
        }
    }

    return fitsInt(result);
}


// ============================================================
// CONSTANT FOLDING HELPERS
// ============================================================

struct NumberValue {

    bool isInt = false;

    long long integer = 0;

    long double floating = 0;
};


bool getNumber(
    const LiteralNode* node,
    NumberValue& number
) {

    if (node->litKind == LitKind::INT) {

        if (!parseInteger(node, number.integer))
            return false;

        number.isInt = true;
        number.floating = number.integer;

        return true;
    }

    if (node->litKind == LitKind::FLOAT) {

        if (!parseFloat(node, number.floating))
            return false;

        number.isInt = false;

        return true;
    }

    return false;
}


// ============================================================
// CONSTANT FOLDING: UNARY
// ============================================================

void foldUnary(NodePtr& node) {

    auto* unary = as<UnaryNode>(node.get());

    if (!unary || !unary->operand)
        return;

    auto* literal = as<LiteralNode>(unary->operand.get());

    if (!literal)
        return;


    // Negative integer.
    if (unary->op == "-" &&
        literal->litKind == LitKind::INT) {

        long long value;

        if (!parseInteger(literal, value))
            return;

        if (!fitsInt(value) ||
            value == numeric_limits<int>::min())
            return;

        replaceLiteral(
            node,
            LitKind::INT,
            to_string(-value)
        );

        return;
    }


    // Negative float.
    if (unary->op == "-" &&
        literal->litKind == LitKind::FLOAT) {

        long double value;

        if (!parseFloat(literal, value))
            return;

        value = -value;

        if (!isfinite(value))
            return;

        replaceLiteral(
            node,
            LitKind::FLOAT,
            formatFloat(value)
        );

        return;
    }


    // Unary positive.
    if (unary->op == "+" &&
        (literal->litKind == LitKind::INT ||
         literal->litKind == LitKind::FLOAT)) {

        replaceLiteral(
            node,
            literal->litKind,
            literal->text
        );

        return;
    }


    // Boolean NOT.
    if (unary->op == "!" &&
        literal->litKind == LitKind::BOOL) {

        bool value = literal->text == "True";

        replaceLiteral(
            node,
            LitKind::BOOL,
            value ? "False" : "True"
        );
    }
}


// ============================================================
// CONSTANT FOLDING: BINARY
// ============================================================

void foldBinary(NodePtr& node) {

    auto* binary = as<BinaryNode>(node.get());

    if (!binary || !binary->lhs || !binary->rhs)
        return;

    auto* left = as<LiteralNode>(binary->lhs.get());
    auto* right = as<LiteralNode>(binary->rhs.get());

    if (!left || !right)
        return;

    const string op = binary->op;


    // --------------------------------------------------------
    // Boolean comparisons.
    // --------------------------------------------------------

    if (left->litKind == LitKind::BOOL &&
        right->litKind == LitKind::BOOL) {

        bool a = left->text == "True";
        bool b = right->text == "True";

        if (op == "==") {
            replaceLiteral(node, LitKind::BOOL,
                a == b ? "True" : "False");
            return;
        }

        if (op == "!=") {
            replaceLiteral(node, LitKind::BOOL,
                a != b ? "True" : "False");
            return;
        }
    }


    // --------------------------------------------------------
    // String comparisons.
    // --------------------------------------------------------

    if (left->litKind == LitKind::STRING &&
        right->litKind == LitKind::STRING) {

        if (op == "==") {
            replaceLiteral(node, LitKind::BOOL,
                left->text == right->text ? "True" : "False");
            return;
        }

        if (op == "!=") {
            replaceLiteral(node, LitKind::BOOL,
                left->text != right->text ? "True" : "False");
            return;
        }
    }


    // --------------------------------------------------------
    // Get numeric operands.
    // --------------------------------------------------------

    NumberValue a, b;

    if (!getNumber(left, a) ||
        !getNumber(right, b))
        return;

    bool bothInt = a.isInt && b.isInt;


    // --------------------------------------------------------
    // Relational operators.
    // --------------------------------------------------------

    if (isComparison(op)) {

        bool result = false;

        if (bothInt) {

            if (op == "==") result = a.integer == b.integer;
            if (op == "!=") result = a.integer != b.integer;
            if (op == "<")  result = a.integer < b.integer;
            if (op == "<=") result = a.integer <= b.integer;
            if (op == ">")  result = a.integer > b.integer;
            if (op == ">=") result = a.integer >= b.integer;
        }
        else {

            if (op == "==") result = a.floating == b.floating;
            if (op == "!=") result = a.floating != b.floating;
            if (op == "<")  result = a.floating < b.floating;
            if (op == "<=") result = a.floating <= b.floating;
            if (op == ">")  result = a.floating > b.floating;
            if (op == ">=") result = a.floating >= b.floating;
        }

        replaceLiteral(
            node,
            LitKind::BOOL,
            result ? "True" : "False"
        );

        return;
    }


    // --------------------------------------------------------
    // Integer arithmetic.
    // --------------------------------------------------------

    if (bothInt && !(op == "**" && b.integer < 0)) {

        if (!fitsInt(a.integer) || !fitsInt(b.integer))
            return;

        long long result = 0;

        bool valid = true;

        if (op == "+") {

            valid = !__builtin_add_overflow(
                a.integer, b.integer, &result);
        }

        else if (op == "-") {

            valid = !__builtin_sub_overflow(
                a.integer, b.integer, &result);
        }

        else if (op == "*") {

            valid = !__builtin_mul_overflow(
                a.integer, b.integer, &result);
        }

        else if (op == "\\") {

            if (b.integer == 0 ||
                (a.integer == numeric_limits<long long>::min()
                 && b.integer == -1))
                return;

            result = a.integer / b.integer;
        }

        else if (op == "%") {

            if (b.integer == 0 ||
                (a.integer == numeric_limits<long long>::min()
                 && b.integer == -1))
                return;

            result = a.integer % b.integer;
        }

        else if (op == "**") {

            valid = integerPower(
                a.integer,
                b.integer,
                result
            );
        }

        else if (op == "&") {
            result = a.integer & b.integer;
        }

        else if (op == "|") {
            result = a.integer | b.integer;
        }

        else if (op == "^") {
            result = a.integer ^ b.integer;
        }

        else {
            return;
        }

        if (!valid || !fitsInt(result))
            return;

        replaceLiteral(
            node,
            LitKind::INT,
            to_string(result)
        );

        return;
    }


    // --------------------------------------------------------
    // Floating-point arithmetic.
    // --------------------------------------------------------

    long double result = 0;

    if (op == "+") {
        result = a.floating + b.floating;
    }

    else if (op == "-") {
        result = a.floating - b.floating;
    }

    else if (op == "*") {
        result = a.floating * b.floating;
    }

    else if (op == "\\") {

        if (b.floating == 0)
            return;

        result = a.floating / b.floating;
    }

    else if (op == "**") {
        result = pow(a.floating, b.floating);
    }

    else {
        return;
    }

    if (!isfinite(result))
        return;

    replaceLiteral(
        node,
        LitKind::FLOAT,
        formatFloat(result)
    );
}


// ============================================================
// AST DEEP COPY
// Required by loop unrolling.
// ============================================================

NodePtr cloneNode(const Node* source) {

    if (!source)
        return nullptr;

    Token token = syntheticToken(
        "",
        source->line,
        source->col
    );

    switch (source->kind) {

        case NodeKind::LITERAL: {

            auto* old = as<LiteralNode>(source);

            auto n = make_unique<LiteralNode>(token);

            n->litKind = old->litKind;
            n->text = old->text;

            return n;
        }

        case NodeKind::NAME: {

            auto* old = as<NameNode>(source);

            auto n = make_unique<NameNode>(token);

            n->id = old->id;
            n->isDecl = old->isDecl;

            return n;
        }

        case NodeKind::CALL: {

            auto* old = as<CallNode>(source);

            auto n = make_unique<CallNode>(token);

            n->callee = old->callee;

            for (const auto& arg : old->args)
                n->args.push_back(cloneNode(arg.get()));

            return n;
        }

        case NodeKind::ARRAY_LITERAL: {

            auto* old = as<ArrayLiteralNode>(source);

            auto n = make_unique<ArrayLiteralNode>(token);

            for (const auto& elem : old->elems)
                n->elems.push_back(cloneNode(elem.get()));

            return n;
        }

        case NodeKind::INDEX: {

            auto* old = as<IndexNode>(source);

            auto n = make_unique<IndexNode>(token);

            n->base = cloneNode(old->base.get());
            n->index = cloneNode(old->index.get());

            return n;
        }

        case NodeKind::MEMBER: {

            auto* old = as<MemberNode>(source);

            auto n = make_unique<MemberNode>(token);

            n->object = cloneNode(old->object.get());
            n->member = cloneNode(old->member.get());

            return n;
        }

        case NodeKind::UNARY: {

            auto* old = as<UnaryNode>(source);

            auto n = make_unique<UnaryNode>(token);

            n->op = old->op;
            n->operand = cloneNode(old->operand.get());

            return n;
        }

        case NodeKind::BINARY: {

            auto* old = as<BinaryNode>(source);

            auto n = make_unique<BinaryNode>(token);

            n->op = old->op;
            n->lhs = cloneNode(old->lhs.get());
            n->rhs = cloneNode(old->rhs.get());

            return n;
        }

        case NodeKind::ASSIGN: {

            auto* old = as<AssignNode>(source);

            auto n = make_unique<AssignNode>(token);

            n->op = old->op;
            n->target = cloneNode(old->target.get());
            n->value = cloneNode(old->value.get());

            return n;
        }

        case NodeKind::BLOCK: {

            auto* old = as<BlockNode>(source);

            auto n = make_unique<BlockNode>(token);

            for (const auto& stmt : old->stmts)
                n->stmts.push_back(cloneNode(stmt.get()));

            return n;
        }

        case NodeKind::EXPR_STMT: {

            auto* old = as<ExprStmtNode>(source);

            auto n = make_unique<ExprStmtNode>();
            
            n->line = old->line;
            n->col = old->col;
            n->expr = cloneNode(old->expr.get());

            return n;
        }

        case NodeKind::RETURN: {

            auto* old = as<ReturnNode>(source);

            auto n = make_unique<ReturnNode>(token);

            n->value = cloneNode(old->value.get());

            return n;
        }

        case NodeKind::IF: {

            auto* old = as<IfNode>(source);

            auto n = make_unique<IfNode>(token);

            n->cond = cloneNode(old->cond.get());
            n->thenBlock = cloneNode(old->thenBlock.get());

            for (const auto& branch : old->elseifs) {

                n->elseifs.emplace_back(
                    cloneNode(branch.first.get()),
                    cloneNode(branch.second.get())
                );
            }

            n->elseBlock = cloneNode(old->elseBlock.get());

            return n;
        }

        case NodeKind::FOR: {

            auto* old = as<ForNode>(source);

            auto n = make_unique<ForNode>(token);

            n->init = cloneNode(old->init.get());
            n->test = cloneNode(old->test.get());
            n->step = cloneNode(old->step.get());
            n->body = cloneNode(old->body.get());

            return n;
        }

        case NodeKind::WHILE: {

            auto* old = as<WhileNode>(source);

            auto n = make_unique<WhileNode>(token);

            n->cond = cloneNode(old->cond.get());
            n->body = cloneNode(old->body.get());

            return n;
        }

        case NodeKind::DO_WHILE: {

            auto* old = as<DoWhileNode>(source);

            auto n = make_unique<DoWhileNode>(token);

            n->body = cloneNode(old->body.get());
            n->cond = cloneNode(old->cond.get());

            return n;
        }

        case NodeKind::FUNCTION: {

            auto* old = as<FunctionNode>(source);

            auto n = make_unique<FunctionNode>(token);

            n->id = old->id;
            n->isVirtual = old->isVirtual;
            n->isAbstract = old->isAbstract;

            for (const auto& param : old->params)
                n->params.push_back(cloneNode(param.get()));

            n->body = cloneNode(old->body.get());

            return n;
        }

        case NodeKind::CLASS: {

            auto* old = as<ClassNode>(source);

            auto n = make_unique<ClassNode>(token);

            n->id = old->id;
            n->bases = old->bases;

            for (const auto& member : old->members)
                n->members.push_back(cloneNode(member.get()));

            return n;
        }

        case NodeKind::PROGRAM: {

            auto* old = as<programNode>(source);

            auto n = make_unique<programNode>();

            for (const auto& item : old->items)
                n->items.push_back(cloneNode(item.get()));

            return n;
        }
    }

    return nullptr;
}


// ============================================================
// LOOP UNROLLING HELPERS
// ============================================================

bool testLoopCondition(
    long long value,
    const string& op,
    long long limit
) {

    if (op == "<")  return value < limit;
    if (op == "<=") return value <= limit;
    if (op == ">")  return value > limit;
    if (op == ">=") return value >= limit;
    if (op == "==") return value == limit;
    if (op == "!=") return value != limit;

    return false;
}


// Check whether a node assigns to a particular variable.
bool writesVariable(
    const Node* node,
    const string& variable
) {

    if (!node)
        return false;

    if (node->kind == NodeKind::ASSIGN) {

        auto* assignment = as<AssignNode>(node);

        auto* target = as<NameNode>(assignment->target.get());

        if (target && getName(target) == variable)
            return true;
    }


    switch (node->kind) {

        case NodeKind::CALL: {

            auto* n = as<CallNode>(node);

            for (const auto& arg : n->args)
                if (writesVariable(arg.get(), variable))
                    return true;

            break;
        }

        case NodeKind::ARRAY_LITERAL: {

            auto* n = as<ArrayLiteralNode>(node);

            for (const auto& elem : n->elems)
                if (writesVariable(elem.get(), variable))
                    return true;

            break;
        }

        case NodeKind::INDEX: {

            auto* n = as<IndexNode>(node);

            return writesVariable(n->base.get(), variable) ||
                   writesVariable(n->index.get(), variable);
        }

        case NodeKind::MEMBER: {

            auto* n = as<MemberNode>(node);

            return writesVariable(n->object.get(), variable) ||
                   writesVariable(n->member.get(), variable);
        }

        case NodeKind::UNARY: {

            auto* n = as<UnaryNode>(node);

            return writesVariable(n->operand.get(), variable);
        }

        case NodeKind::BINARY: {

            auto* n = as<BinaryNode>(node);

            return writesVariable(n->lhs.get(), variable) ||
                   writesVariable(n->rhs.get(), variable);
        }

        case NodeKind::ASSIGN: {

            auto* n = as<AssignNode>(node);

            return writesVariable(n->value.get(), variable);
        }

        case NodeKind::BLOCK: {

            auto* n = as<BlockNode>(node);

            for (const auto& stmt : n->stmts)
                if (writesVariable(stmt.get(), variable))
                    return true;

            break;
        }

        case NodeKind::EXPR_STMT:
            return writesVariable(
                as<ExprStmtNode>(node)->expr.get(), variable);

        case NodeKind::RETURN:
            return writesVariable(
                as<ReturnNode>(node)->value.get(), variable);

        case NodeKind::IF: {

            auto* n = as<IfNode>(node);

            if (writesVariable(n->cond.get(), variable) ||
                writesVariable(n->thenBlock.get(), variable) ||
                writesVariable(n->elseBlock.get(), variable))
                return true;

            for (const auto& branch : n->elseifs)
                if (writesVariable(branch.first.get(), variable) ||
                    writesVariable(branch.second.get(), variable))
                    return true;

            break;
        }

        case NodeKind::FOR: {

            auto* n = as<ForNode>(node);

            return writesVariable(n->init.get(), variable) ||
                   writesVariable(n->test.get(), variable) ||
                   writesVariable(n->step.get(), variable) ||
                   writesVariable(n->body.get(), variable);
        }

        case NodeKind::WHILE: {

            auto* n = as<WhileNode>(node);

            return writesVariable(n->cond.get(), variable) ||
                   writesVariable(n->body.get(), variable);
        }

        case NodeKind::DO_WHILE: {

            auto* n = as<DoWhileNode>(node);

            return writesVariable(n->body.get(), variable) ||
                   writesVariable(n->cond.get(), variable);
        }

        case NodeKind::FUNCTION:
        case NodeKind::CLASS:
        case NodeKind::PROGRAM:
            // Avoid assuming nested declarations cannot modify
            // an outer variable.
            return true;

        default:
            break;
    }

    return false;
}


// Extract a simple integer loop step.
// Supported forms:
// i = i + 1
// i = i - 1
// i += 1
// i -= 1

bool getLoopStep(
    const AssignNode* step,
    const string& variable,
    long long& delta
) {

    if (!step)
        return false;

    auto* target = as<NameNode>(step->target.get());

    if (!target || getName(target) != variable)
        return false;


    // Compound assignment.
    if (step->op == "+=" || step->op == "-=") {

        auto* literal = as<LiteralNode>(step->value.get());

        long long amount;

        if (!parseInteger(literal, amount) ||
            !fitsInt(amount))
            return false;

        if (step->op == "-=") {

            if (amount == numeric_limits<long long>::min())
                return false;

            amount = -amount;
        }

        delta = amount;

        return delta != 0;
    }


    if (step->op != "=")
        return false;


    auto* expression = as<BinaryNode>(step->value.get());

    if (!expression)
        return false;

    auto* left = as<NameNode>(expression->lhs.get());
    auto* right = as<LiteralNode>(expression->rhs.get());

    if (!left || getName(left) != variable)
        return false;

    long long amount;

    if (!parseInteger(right, amount) ||
        !fitsInt(amount))
        return false;

    if (expression->op == "+") {
        delta = amount;
    }

    else if (expression->op == "-") {

        if (amount == numeric_limits<long long>::min())
            return false;

        delta = -amount;
    }

    else {
        return false;
    }

    return delta != 0;
}


// Check if a ForNode is safe to unroll.
bool getUnrollCount(
    const ForNode* loop,
    long long& initial,
    long long& delta,
    long long& iterations,
    string& variable
) {

    auto* init = as<AssignNode>(loop->init.get());
    auto* test = as<BinaryNode>(loop->test.get());
    auto* step = as<AssignNode>(loop->step.get());

    if (!init || !test || !step || !loop->body)
        return false;

    if (init->op != "=")
        return false;


    auto* initTarget = as<NameNode>(init->target.get());
    auto* initValue = as<LiteralNode>(init->value.get());

    if (!initTarget ||
        !parseInteger(initValue, initial) ||
        !fitsInt(initial))
        return false;

    variable = getName(initTarget);

    if (variable.empty())
        return false;


    // The condition must compare the loop variable with
    // a constant integer.
    auto* testVariable = as<NameNode>(test->lhs.get());
    auto* limitNode = as<LiteralNode>(test->rhs.get());

    long long limit;

    if (!testVariable ||
        getName(testVariable) != variable ||
        !parseInteger(limitNode, limit) ||
        !fitsInt(limit))
        return false;

    if (!testLoopCondition(initial, test->op, limit) &&
        test->op != "<" && test->op != "<=" &&
        test->op != ">" && test->op != ">=" &&
        test->op != "==" && test->op != "!=")
        return false;


    if (!getLoopStep(step, variable, delta))
        return false;


    // Do not unroll if the body changes the counter.
    if (writesVariable(loop->body.get(), variable))
        return false;


    // Simulate the loop to find its iteration count.
    // Limit prevents excessive code growth.
    constexpr long long MAX_UNROLL = 16;

    long long current = initial;

    iterations = 0;

    while (testLoopCondition(current, test->op, limit)) {

        if (iterations >= MAX_UNROLL)
            return false;

        iterations++;

        long long next;

        if (__builtin_add_overflow(current, delta, &next))
            return false;

        if (!fitsInt(next))
            return false;

        current = next;
    }

    return true;
}


// ============================================================
// DEAD CODE HELPERS
// ============================================================


// Recursively collect variables that are read by a node.
void collectReads(
    const Node* node,
    unordered_set<string>& live
) {

    if (!node)
        return;

    switch (node->kind) {

        case NodeKind::NAME: {

            auto* n = as<NameNode>(node);

            if (!n->isDecl)
                live.insert(getName(n));

            break;
        }

        case NodeKind::CALL: {

            auto* n = as<CallNode>(node);

            for (const auto& arg : n->args)
                collectReads(arg.get(), live);

            break;
        }

        case NodeKind::ARRAY_LITERAL: {

            auto* n = as<ArrayLiteralNode>(node);

            for (const auto& elem : n->elems)
                collectReads(elem.get(), live);

            break;
        }

        case NodeKind::INDEX: {

            auto* n = as<IndexNode>(node);

            collectReads(n->base.get(), live);
            collectReads(n->index.get(), live);

            break;
        }

        case NodeKind::MEMBER: {

            auto* n = as<MemberNode>(node);

            collectReads(n->object.get(), live);

            break;
        }

        case NodeKind::UNARY: {

            auto* n = as<UnaryNode>(node);

            collectReads(n->operand.get(), live);

            break;
        }

        case NodeKind::BINARY: {

            auto* n = as<BinaryNode>(node);

            collectReads(n->lhs.get(), live);
            collectReads(n->rhs.get(), live);

            break;
        }

        case NodeKind::ASSIGN: {

            auto* n = as<AssignNode>(node);

            collectReads(n->value.get(), live);

            // Compound assignments also read their target.
            if (n->op != "=")
                collectReads(n->target.get(), live);

            // Complex assignment targets may read an object
            // or array index.
            else if (!as<NameNode>(n->target.get()))
                collectReads(n->target.get(), live);

            break;
        }

        case NodeKind::BLOCK: {

            auto* n = as<BlockNode>(node);

            for (const auto& stmt : n->stmts)
                collectReads(stmt.get(), live);

            break;
        }

        case NodeKind::EXPR_STMT:
            collectReads(
                as<ExprStmtNode>(node)->expr.get(), live);
            break;

        case NodeKind::RETURN:
            collectReads(
                as<ReturnNode>(node)->value.get(), live);
            break;

        case NodeKind::IF: {

            auto* n = as<IfNode>(node);

            collectReads(n->cond.get(), live);
            collectReads(n->thenBlock.get(), live);

            for (const auto& branch : n->elseifs) {
                collectReads(branch.first.get(), live);
                collectReads(branch.second.get(), live);
            }

            collectReads(n->elseBlock.get(), live);

            break;
        }

        case NodeKind::FOR: {

            auto* n = as<ForNode>(node);

            collectReads(n->init.get(), live);
            collectReads(n->test.get(), live);
            collectReads(n->step.get(), live);
            collectReads(n->body.get(), live);

            break;
        }

        case NodeKind::WHILE: {

            auto* n = as<WhileNode>(node);

            collectReads(n->cond.get(), live);
            collectReads(n->body.get(), live);

            break;
        }

        case NodeKind::DO_WHILE: {

            auto* n = as<DoWhileNode>(node);

            collectReads(n->body.get(), live);
            collectReads(n->cond.get(), live);

            break;
        }

        case NodeKind::FUNCTION: {

            auto* n = as<FunctionNode>(node);

            for (const auto& param : n->params)
                collectReads(param.get(), live);

            collectReads(n->body.get(), live);

            break;
        }

        case NodeKind::CLASS: {

            auto* n = as<ClassNode>(node);

            for (const auto& member : n->members)
                collectReads(member.get(), live);

            break;
        }

        case NodeKind::PROGRAM: {

            auto* n = as<programNode>(node);

            for (const auto& item : n->items)
                collectReads(item.get(), live);

            break;
        }

        default:
            break;
    }
}


// Return true if evaluating a node may have side effects.
bool hasSideEffects(const Node* node) {

    if (!node)
        return false;

    switch (node->kind) {

        // Function calls may modify program state.
        case NodeKind::CALL:
            return true;

        case NodeKind::ASSIGN:
            return true;

        case NodeKind::UNARY:
            return hasSideEffects(
                as<UnaryNode>(node)->operand.get());

        case NodeKind::BINARY: {

            auto* n = as<BinaryNode>(node);

            return hasSideEffects(n->lhs.get()) ||
                   hasSideEffects(n->rhs.get());
        }

        case NodeKind::ARRAY_LITERAL: {

            auto* n = as<ArrayLiteralNode>(node);

            for (const auto& elem : n->elems)
                if (hasSideEffects(elem.get()))
                    return true;

            return false;
        }

        case NodeKind::INDEX: {

            auto* n = as<IndexNode>(node);

            return hasSideEffects(n->base.get()) ||
                   hasSideEffects(n->index.get());
        }

        case NodeKind::MEMBER:

            return hasSideEffects(
                as<MemberNode>(node)->object.get());

        default:
            return false;
    }
}


// Find a direct assignment inside an expression statement.
AssignNode* getStatementAssignment(Node* statement) {

    if (!statement)
        return nullptr;

    if (statement->kind == NodeKind::ASSIGN)
        return as<AssignNode>(statement);

    if (statement->kind == NodeKind::EXPR_STMT) {

        auto* expr = as<ExprStmtNode>(statement);

        return as<AssignNode>(expr->expr.get());
    }

    return nullptr;
}

// Convert an expression into a proper expression statement.
// This ensures the C++ generator adds the required semicolon.
NodePtr makeExpressionStatement(NodePtr expression) {

    if (!expression)
        return nullptr;

    // Already a statement: don't wrap it again.
    if (expression->kind == NodeKind::EXPR_STMT)
        return expression;

    auto statement = make_unique<ExprStmtNode>();

    statement->line = expression->line;
    statement->col = expression->col;
    statement->expr = move(expression);

    return statement;
}

} // namespace


// ============================================================
// PASS 1: CONSTANT FOLDING
// ============================================================

void Optimizer::foldExpressions(NodePtr& node) {

    if (!node)
        return;

    switch (node->kind) {

        case NodeKind::LITERAL:
        case NodeKind::NAME:
            break;


        case NodeKind::CALL: {

            auto* n = as<CallNode>(node.get());

            for (auto& arg : n->args)
                foldExpressions(arg);

            break;
        }


        case NodeKind::ARRAY_LITERAL: {

            auto* n = as<ArrayLiteralNode>(node.get());

            for (auto& elem : n->elems)
                foldExpressions(elem);

            break;
        }


        case NodeKind::INDEX: {

            auto* n = as<IndexNode>(node.get());

            foldExpressions(n->base);
            foldExpressions(n->index);

            break;
        }


        case NodeKind::MEMBER: {

            auto* n = as<MemberNode>(node.get());

            foldExpressions(n->object);
            foldExpressions(n->member);

            break;
        }


        case NodeKind::UNARY: {

            auto* n = as<UnaryNode>(node.get());

            foldExpressions(n->operand);

            foldUnary(node);

            break;
        }


        case NodeKind::BINARY: {

            auto* n = as<BinaryNode>(node.get());

            foldExpressions(n->lhs);
            foldExpressions(n->rhs);

            foldBinary(node);

            break;
        }


        case NodeKind::ASSIGN: {

            auto* n = as<AssignNode>(node.get());

            foldExpressions(n->target);
            foldExpressions(n->value);

            break;
        }


        case NodeKind::BLOCK: {

            auto* n = as<BlockNode>(node.get());

            for (auto& stmt : n->stmts)
                foldExpressions(stmt);

            break;
        }


        case NodeKind::EXPR_STMT:

            foldExpressions(
                as<ExprStmtNode>(node.get())->expr);

            break;


        case NodeKind::RETURN:

            foldExpressions(
                as<ReturnNode>(node.get())->value);

            break;


        case NodeKind::IF: {

            auto* n = as<IfNode>(node.get());

            foldExpressions(n->cond);
            foldExpressions(n->thenBlock);

            for (auto& branch : n->elseifs) {
                foldExpressions(branch.first);
                foldExpressions(branch.second);
            }

            foldExpressions(n->elseBlock);


            // Evaluate a constant primary condition.
            auto* condition = as<LiteralNode>(n->cond.get());

            if (!condition || condition->litKind != LitKind::BOOL)
                break;

            bool selected = condition->text == "True";

            if (selected) {

                node = move(n->thenBlock);

                if (!node)
                    node = make_unique<BlockNode>(
                        syntheticToken("", 0, 0));

                break;
            }


            // Primary branch was false.
            bool resolved = true;

            for (auto& branch : n->elseifs) {

                auto* elseCondition =
                    as<LiteralNode>(branch.first.get());

                if (!elseCondition ||
                    elseCondition->litKind != LitKind::BOOL) {

                    resolved = false;
                    break;
                }

                if (elseCondition->text == "True") {

                    node = move(branch.second);

                    if (!node)
                        node = make_unique<BlockNode>(
                            syntheticToken("", 0, 0));

                    break;
                }
            }

            if (resolved) {

                if (node->kind == NodeKind::IF) {

                    auto* remaining = as<IfNode>(node.get());

                    node = move(remaining->elseBlock);
                }

                if (!node)
                    node = make_unique<BlockNode>(
                        syntheticToken("", 0, 0));
            }

            break;
        }


        case NodeKind::FOR: {

            auto* n = as<ForNode>(node.get());

            foldExpressions(n->init);
            foldExpressions(n->test);
            foldExpressions(n->step);
            foldExpressions(n->body);

            break;
        }


        case NodeKind::WHILE: {

            auto* n = as<WhileNode>(node.get());

            foldExpressions(n->cond);
            foldExpressions(n->body);

            break;
        }


        case NodeKind::DO_WHILE: {

            auto* n = as<DoWhileNode>(node.get());

            foldExpressions(n->body);
            foldExpressions(n->cond);

            break;
        }


        case NodeKind::FUNCTION: {

            auto* n = as<FunctionNode>(node.get());

            for (auto& param : n->params)
                foldExpressions(param);

            foldExpressions(n->body);

            break;
        }


        case NodeKind::CLASS: {

            auto* n = as<ClassNode>(node.get());

            for (auto& member : n->members)
                foldExpressions(member);

            break;
        }


        case NodeKind::PROGRAM: {

            auto* n = as<programNode>(node.get());

            for (auto& item : n->items)
                foldExpressions(item);

            break;
        }
    }
}



// ============================================================
// PASS 2: LOOP UNROLLING
// ============================================================

void Optimizer::unrollLoops(NodePtr& node) {

    if (!node)
        return;

    // --------------------------------------------------------
    // First optimise nested loops.
    // --------------------------------------------------------

    switch (node->kind) {

        case NodeKind::BLOCK: {

            auto* n = as<BlockNode>(node.get());

            for (auto& stmt : n->stmts)
                unrollLoops(stmt);

            break;
        }

        case NodeKind::IF: {

            auto* n = as<IfNode>(node.get());

            unrollLoops(n->thenBlock);

            for (auto& branch : n->elseifs)
                unrollLoops(branch.second);

            unrollLoops(n->elseBlock);

            break;
        }

        case NodeKind::FOR: {

            auto* n = as<ForNode>(node.get());

            unrollLoops(n->body);

            break;
        }

        case NodeKind::WHILE:

            unrollLoops(as<WhileNode>(node.get())->body);
            break;

        case NodeKind::DO_WHILE:

            unrollLoops(as<DoWhileNode>(node.get())->body);
            break;

        case NodeKind::FUNCTION:

            unrollLoops(as<FunctionNode>(node.get())->body);
            break;

        case NodeKind::CLASS: {

            auto* n = as<ClassNode>(node.get());

            for (auto& member : n->members)
                unrollLoops(member);

            break;
        }

        case NodeKind::PROGRAM: {

            auto* n = as<programNode>(node.get());

            for (auto& item : n->items)
                unrollLoops(item);

            break;
        }

        default:
            break;
    }


    // --------------------------------------------------------
    // Only unroll FOR loops.
    // --------------------------------------------------------

    if (node->kind != NodeKind::FOR)
        return;

    auto* loop = as<ForNode>(node.get());

    long long initial = 0;
    long long delta = 0;
    long long iterations = 0;

    string variable;


    // Check whether this is a safe, recognisable loop.
    if (!getUnrollCount(
        loop,
        initial,
        delta,
        iterations,
        variable
    )) {
        return;
    }


    // --------------------------------------------------------
    // Get the loop body.
    // --------------------------------------------------------

    auto* body = as<BlockNode>(loop->body.get());

    if (!body)
        return;


    // Do not flatten a loop body that declares variables
    // directly in its outermost block. Each original iteration
    // has its own scope, so flattening would change behaviour.

    for (const auto& statement : body->stmts) {

        AssignNode* assignment =
            getStatementAssignment(statement.get());

        if (!assignment)
            continue;

        auto* target =
            as<NameNode>(assignment->target.get());

        if (target && target->isDecl)
            return;
    }


    // --------------------------------------------------------
    // Create ONE replacement block.
    // --------------------------------------------------------

    auto replacement = make_unique<BlockNode>(
        syntheticToken("", node->line, node->col)
    );


    // --------------------------------------------------------
    // Preserve the initial assignment.
    // Wrap it in ExprStmtNode to generate a semicolon.
    // --------------------------------------------------------

    NodePtr initCopy = cloneNode(loop->init.get());

    replacement->stmts.push_back(
        makeExpressionStatement(move(initCopy))
    );


    // --------------------------------------------------------
    // Unroll the loop.
    //
    // IMPORTANT:
    // Copy the individual body statements instead of copying
    // the entire BlockNode. This prevents extra braces.
    // --------------------------------------------------------

    for (long long i = 0; i < iterations; i++) {

        for (const auto& statement : body->stmts) {

            NodePtr statementCopy =
                cloneNode(statement.get());

            if (!statementCopy)
                return;

            // Wrap raw expressions/assignments in statements.
            switch (statementCopy->kind) {

                case NodeKind::ASSIGN:
                case NodeKind::CALL:
                case NodeKind::BINARY:
                case NodeKind::UNARY:
                case NodeKind::INDEX:
                case NodeKind::MEMBER:
                case NodeKind::ARRAY_LITERAL:
                case NodeKind::NAME:
                case NodeKind::LITERAL:

                    statementCopy =
                        makeExpressionStatement(
                            move(statementCopy)
                        );

                    break;

                default:
                    break;
            }

            replacement->stmts.push_back(
                move(statementCopy)
            );
        }


        // ----------------------------------------------------
        // Apply the loop increment.
        //
        // Also wrap it in ExprStmtNode for the semicolon.
        // The increment is retained after the final iteration
        // to preserve normal for-loop behaviour.
        // ----------------------------------------------------

        NodePtr stepCopy =
            cloneNode(loop->step.get());

        replacement->stmts.push_back(
            makeExpressionStatement(move(stepCopy))
        );
    }


    // --------------------------------------------------------
    // Replace the original FOR node.
    // --------------------------------------------------------

    node = move(replacement);
}


// ============================================================
// PASS 3: DEAD CODE ELIMINATION
// ============================================================

void Optimizer::eliminateDeadCode(NodePtr& node) {

    if (!node)
        return;


    // --------------------------------------------------------
    // Process individual node children.
    // --------------------------------------------------------

    switch (node->kind) {

        case NodeKind::BLOCK: {

            auto* n = as<BlockNode>(node.get());

            for (auto& stmt : n->stmts)
                eliminateDeadCode(stmt);

            // Remove statements after an unconditional return.
            size_t returnIndex = n->stmts.size();

            for (size_t i = 0; i < n->stmts.size(); i++) {

                if (n->stmts[i] &&
                    n->stmts[i]->kind == NodeKind::RETURN) {

                    returnIndex = i + 1;
                    break;
                }
            }

            if (returnIndex < n->stmts.size()) {

                n->stmts.erase(
                    n->stmts.begin() + returnIndex,
                    n->stmts.end()
                );
            }


            // ------------------------------------------------
            // Remove unused variable declarations.
            // Backwards liveness analysis within this block.
            // ------------------------------------------------

            unordered_set<string> live;

            vector<bool> remove(n->stmts.size(), false);

            for (int i = static_cast<int>(n->stmts.size()) - 1;
                 i >= 0;
                 i--) {

                Node* statement = n->stmts[i].get();

                AssignNode* assignment =
                    getStatementAssignment(statement);

                if (!assignment) {

                    collectReads(statement, live);
                    continue;
                }


                auto* target =
                    as<NameNode>(assignment->target.get());


                // Only remove direct variable declarations.
                if (target &&
                    target->isDecl &&
                    assignment->op == "=") {

                    string name = getName(target);

                    bool variableIsUsed = live.count(name) > 0;

                    bool pureValue =
                        !hasSideEffects(assignment->value.get());


                    if (!variableIsUsed && pureValue) {

                        remove[i] = true;
                        continue;
                    }

                    // The assignment defines the variable.
                    live.erase(name);

                    collectReads(assignment->value.get(), live);

                    continue;
                }

                collectReads(statement, live);
            }


            // Rebuild the block without removed statements.
            vector<NodePtr> kept;

            for (size_t i = 0; i < n->stmts.size(); i++) {

                if (!remove[i])
                    kept.push_back(move(n->stmts[i]));
            }

            n->stmts = move(kept);

            break;
        }


        case NodeKind::IF: {

            auto* n = as<IfNode>(node.get());

            eliminateDeadCode(n->thenBlock);

            for (auto& branch : n->elseifs)
                eliminateDeadCode(branch.second);

            eliminateDeadCode(n->elseBlock);

            break;
        }


        case NodeKind::FOR:

            eliminateDeadCode(
                as<ForNode>(node.get())->body);

            break;


        case NodeKind::WHILE:

            eliminateDeadCode(
                as<WhileNode>(node.get())->body);

            break;


        case NodeKind::DO_WHILE:

            eliminateDeadCode(
                as<DoWhileNode>(node.get())->body);

            break;


        case NodeKind::FUNCTION:

            eliminateDeadCode(
                as<FunctionNode>(node.get())->body);

            break;


        case NodeKind::CLASS: {

            auto* n = as<ClassNode>(node.get());

            for (auto& member : n->members)
                eliminateDeadCode(member);

            break;
        }


        case NodeKind::PROGRAM: {

            auto* n = as<programNode>(node.get());

            for (auto& item : n->items)
                eliminateDeadCode(item);

            break;
        }


        default:
            break;
    }
}


// ============================================================
// MAIN OPTIMIZER
// ============================================================

void Optimizer::optimize(NodePtr& root) {

    if (!root)
        return;


    // First simplify all constant expressions.
    foldExpressions(root);


    // Next unroll loops with known iteration counts.
    unrollLoops(root);


    // Fold expressions in the newly expanded loop bodies.
    foldExpressions(root);


    // Finally remove unnecessary code.
    eliminateDeadCode(root);
}
