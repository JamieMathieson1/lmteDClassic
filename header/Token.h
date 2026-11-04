#pragma once 
#include <string>

using namespace std;

enum class TokenType {
    LITERAL, 
    IDENTIFIER, 
    OPERATOR,
    PUNCTUATOR, 
    KEYWORD
};



class Token {
    private: 
        string content;
        TokenType type;
        int line;
        int charNum; // the position of the token - line, charNum. 

    public: 
        Token() : content(""), type(TokenType::LITERAL), line(0), charNum(0) {}

        Token(const string& content, TokenType type, int line, int charNum)
            : content(content), type(type), line(line), charNum(charNum) {}

        const string& getContent() const { return content; }
        TokenType getType() const { return type; }
        int getLine() const { return line; }
        int getCharNum() const { return charNum; }
};