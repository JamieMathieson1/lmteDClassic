#pragma once
#include <vector>
#include "Token.h"
#include <string>
#include <unordered_set>

using namespace std;

const unordered_set<string> OPERATORS = {
    // arithmetic
    "+", "-", "*", "**", "\\", "%",
    // relational
    "<", "<=", "==", ">", ">=", "!=",
    // assignment
    "=", "+=", "-=", "*=", "**=", "\\=", "%=",
    // bitwise
    "&", "|", "^", "&=", "|=", "^=", "!", "!="
};

const unordered_set<string> KEYWORDS = {
    // control flow
    "if", "elseif", "catch", "condition", "tested", "testing", "return",
    // OOP / memory
    "worships", "beg", "donate"
};

const unordered_set<string> TYPES = {
    "int", "bool", "char", "string", "float", "void"
};

const unordered_set<string> BOOL_LITERALS = { "True", "False" };

const unordered_set<char> PUNCTUATORS = {
    '[', ']',   // conditions, params, array length
    '(', ')',   // grouping, array literals
    ',',        // separators
    ';',        // member access
    '/',
    'd',
    'b'         // end of 
};

// Wrapper for tokens, next() creates
class Tokenizer {
    private:
        vector<Token> tokens;
        int curr = 0;

        // helper functions
        bool isValidLiteral(string& content); 
        bool isValidIdentifier(string& content);
        bool arrayIdentifier(string& arr);
        bool isValidName(string& name);
        void finalizeToken(string& content, int i);
        // populates the tokens array, called once at start of main
        void build(string& code);

        // helper vars
        bool strM = false;
        bool arrM = false;
        bool opM = false;
        int line = 1;
        int lineStart = 0;

    public:

        Tokenizer(string& code) { build(code); };

        bool canPoll() {
            return curr < tokens.size();
        }

        Token& peek() {
            return tokens[curr];
        }

        Token& poll() {
            curr++; 
            return tokens[curr - 1];
        }

        int getCurrIdx() { return curr; }
        
};