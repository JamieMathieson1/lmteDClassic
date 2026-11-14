# pragma once

# include "Tree.h"
# include <string>
# include <vector>
# include <unordered_map>

// This enumeration defines the multiple different data types that the semantic analyser can recognise.
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

// This struct is representing symbols within the table
struct Symbol {
    string name;
    SemanticType type;
};

// Class to perform the Semantic Analysis
class SemanticAnalyser {
    private:
        // Private Variables include:
        // 1. A Scope type as an unordered map that linkes variable names with their Symbol info
        // 2. A scope_list which holds all the currently active scopes
        // 3. A list of all the errors discovered during the analysis.
        // 4. A currentReturnType variable that keeps track of the return type of the function being analysed.
        using Scope = unordered_map<string, Symbol>;
        vector<Scope> scope_list;
        vector<string> errors;
        SemanticType currentReturnType = SemanticType::UNKNOWN;

        // Managing the creation and removal of Scopes
        void enterScope();
        void exitScope();

        // Managing Variable Declarations and Symbol Table lookup
        bool declare(const Symbol& symbol);
        Symbol* lookup(const string& name);

        // Record and Associates any errors
        void error(const Node* node, const string& message);

        // Getting Type Helper Function
        SemanticType getType(const string& type);

        // Do a specialised check for a binary expression
        SemanticType checkBinary(const string& op, SemanticType lhs, SemanticType rhs, const Node* node);
        
        // Do a specialised check for assignment expressions
        SemanticType checkAssignment(const AssignNode* node);

        // Traverse the AST
        void checkNode(const Node* node);

        // Analyse the Expression
        SemanticType checkExpression(const Node* node);
    public:
        // Functions for setting up, running the analyser and finding errors
        SemanticAnalyser();
        bool analyse(const programNode& program);
        bool hasErrors() const;
        const vector<string>& getErrors() const;
};