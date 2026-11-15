#pragma once
#include <string>

enum class TokenType;

using namespace std;

// Shared helper functions, all static
class Utils {
    public:
        // readable name for a token type, eg. TokenType::KEYWORD -> "KEYWORD"
        static const char* toString(TokenType t);

        // copy of s without leading/trailing whitespace
        static string trim(const string& s);
};
