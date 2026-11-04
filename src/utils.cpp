#include "../header/Utils.h"

const char* Utils::toString(TokenType t) {
    switch (t) {
        case TokenType::KEYWORD:    return "KEYWORD";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::OPERATOR:   return "OPERATOR";
        case TokenType::PUNCTUATOR: return "PUNCTUATOR";
        case TokenType::LITERAL:    return "LITERAL";
    }
    return "UNKNOWN";
}

string Utils::trim(const string& s) {
    const char* ws = " \t\n\r";
    auto start = s.find_first_not_of(ws);
    if (start == string::npos) return "";  // all whitespace
    auto end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}
