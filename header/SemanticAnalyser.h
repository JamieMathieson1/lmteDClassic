# pragma once

# include "Tree.h"
# include <string>
# include <vector>
# include <unordered_map>

enum class SemanticType {
    INT,
    FLOAT,
    BOOL,
    CHAR,
    STRING,
    VOID,
    ARRAY,
    CLASS,
    UNKNOWN,
    ERROR
};

struct Symbol {
    string name;
    SemanticType type;
};

class SemanticAnalyser {
    private:
        using Scope = unordered_map<string, Symbol>;
        vector<Scope> scope_list;
        vector<string> errors;
        SemanticType currentReturnType = SemanticType::UNKNOWN;

        // Managing Scope
        void enterScope();
        void exitScope();

        // Managing Symbols
        bool declare(const Symbol& symbol);
        Symbol* lookup(const string& name);

        // Handle Errors
        void error(const Node* node, const string& message);

        // Getting Type Helper Function
        SemanticType getType(const string& type);

        // Check if a expression is correct when using binary nodes
        SemanticType checkBinary(const string& op, SemanticType lhs, SemanticType rhs, const Node* node);
        
        // Check if a expression is correct when doing assignment and with assignment nodes
        SemanticType checkAssignment(const AssignNode* node);

        // Traverse the AST
        void checkNode(const Node* node);

        // Analyse the Expression
        SemanticType checkExpression(const Node* node);
    public:
        SemanticAnalyser();
        bool analyse(const programNode& program);
        bool hasErrors() const;
        const vector<string>& getErrors() const;
};