#include "../header/Tokenizer.h"

#include <cctype>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "../header/Utils.h"
bool Tokenizer::isValidType(const string& t) {
  if (TYPES.count(t)) return true;
  // class type: starts with an uppercase letter, alphanumeric only
  return !t.empty() && isupper(static_cast<unsigned char>(t[0])) &&
         all_of(t.begin(), t.end(), ::isalnum);
}
bool Tokenizer::isValidLiteral(string& content) {
  if (content.empty()) return false;
  // either a string:
  if (content.size() >= 2 && content[0] == '{' && content.back() == '}') {
    return true;
  }
  // or a numeric literal : all digits + only one appearance of '.'
  bool dotSeen = false;
  for (char a : content) {
    if (isalpha(a)) return false;
    if (a == '.') {
      if (dotSeen) return false;
      dotSeen = true;
    }
  }
  return true;
}

bool Tokenizer::isValidName(string& name) {
  if (name.empty()) return false;
  return all_of(name.begin(), name.end(), ::isalnum);
}

bool Tokenizer::arrayIdentifier(string& arr) {
  // either arr is a primitive type with square brackets eg. int[] or it starts
  // with A_<call function here>_validName first call will never be a base case
  // arr is only the type part (name already stripped by isValidIdentifier)
  // array type := "A_" + elem + "[size]", where elem is a primitive type or
  // another array type eg. A_int[5], A_A_int[3][4], A_A_A_char[2][3][4]
  if (!arr.starts_with("A_") || arr.back() != ']') return false;

  // this level's size is the last [digits]
  size_t left = arr.rfind('[');
  if (left == string::npos || left < 2) return false;

  string num = arr.substr(left + 1, arr.size() - left - 2);
  if (num.empty()) return false;
  for (char c : num)
    if (!isdigit(static_cast<unsigned char>(c))) return false;

  // element type sits between "A_" and "[size]"
  string elem = arr.substr(2, left - 2);
  if (isValidType(elem)) return true;
  return arrayIdentifier(elem);
}

bool Tokenizer::isValidIdentifier(string& content) {
  // starting can only be v or a
  // regex of 'v_<type>_<name without operators / punctuators>
  // type can only be a valid type, name can have only letters and numbers.
  // or 'a_type_name[length]' where type can be recursively defined as an array
  // recursively or a type
  if (content.starts_with("V_") || content.starts_with("F_")) {
    istringstream iss(content);
    string part;
    iss.ignore(content.size(), '_');
    getline(iss, part, '_');

    // Primitive type, or (variables only) a class name starting with a
    // capital letter, e.g. V_Dog_rex
    bool classType = content[0] == 'V' && !part.empty() &&
                     isupper(static_cast<unsigned char>(part[0])) &&
                     isValidName(part);
    if (!TYPES.count(part) && !classType) return false;

    getline(iss, part, '\0');

    return isValidName(part);
  } else if (content.starts_with("A_")) {
    // A_<array type>_name -> name is after the last '_', the rest is the array
    // type
    size_t sep = content.rfind('_');
    if (sep == string::npos || sep == content.size() - 1) return false;

    string name = content.substr(sep + 1);
    if (!isValidName(name)) return false;

    string type = content.substr(0, sep);
    return arrayIdentifier(type);
  } else if (content.starts_with("C_")) {
    string name = content.substr(2);
    return isValidName(name);
  }
  return false;
}

void Tokenizer::finalizeToken(string& content, int i) {
  content = Utils::trim(content);
  if (content.empty()) {
    opM = false;
    strM = false;
    arrM = false;
    return;
  }
  TokenType type;
  if (OPERATORS.count(content))
    type = TokenType::OPERATOR;
  else if (TYPES.count(content))
    type = TokenType::KEYWORD;
  else if (KEYWORDS.count(content))
    type = TokenType::KEYWORD;
  else if (content.size() == 1 && PUNCTUATORS.count(content[0]))
    type = TokenType::PUNCTUATOR;
  else if (BOOL_LITERALS.count(content) || isValidLiteral(content))
    type = TokenType::LITERAL;
  else if (isValidIdentifier(content))
    type = TokenType::IDENTIFIER;
  else {
    // invalid token, throw error
    cout << "THROWING FOR: " << content << endl;
    throw invalid_argument("Incorrect token");
  }
  // i is one past the token's last char, so i - size is where it starts; +1 for
  // 1-based columns
  tokens.emplace_back(content, type, line,
                      i - lineStart - (int)content.size() + 1);
  content.clear();
  opM = false;
  strM = false;
  arrM = false;
}

void Tokenizer::build(string& code) {
  // line number => increment on each '/'
  // keep last line start, curr char number = last line start - curr position
  // operator mode: seen an operator, expecting either an operator to continue
  // token or anything else is a new token non operator mode: all letters
  // accepted and underscores, anything else belongs to the next token. not
  // accounting: array literals, classes, functions, [] string mode: accept
  // anything until a '}'.
  int len(code.size());
  string content;
  bool lineComment = false;
  bool blockComment = false;

  for (int i = 0; i < len; i++) {
    if (lineComment) {
      if (code[i] == '\n' || code[i] == '\r') {
        lineComment = false;
        line++;
        lineStart = i + 1;
      }
      continue;
    }

    if (blockComment) {
      if (code.compare(i, 4, "HAFF") == 0) {
        blockComment = false;
        i += 3;
      } else if (code[i] == '\n' || code[i] == '\r') {
        line++;
        lineStart = i + 1;
      }
      continue;
    }

    if (content.empty() && code.compare(i, 4, "FFAH") == 0) {
      blockComment = true;
      i += 3;
      continue;
    }
    if (content.empty() && code.compare(i, 3, "FAH") == 0) {
      lineComment = true;
      i += 2;
      continue;
    }

    // inside string
    if (strM) {
      content.push_back(code[i]);
      if (code[i] == '}') {
        finalizeToken(content, i + 1);
      }
    } else if (code[i] == '\n' || code[i] == '\r') {
      finalizeToken(content, i);
      line++;
      lineStart = i + 1;
    } else if (code[i] == ' ') {
      // finalize token
      finalizeToken(content, i);
    }

    else if (opM) {
      if (OPERATORS.count(string(1, code[i]))) {
        content.push_back(code[i]);
        i++;
        // exception: **= is the only 3-char operator
        if (content == "**" && i < len && code[i] == '=') {
          content.push_back(code[i]);
          i++;
        }
      }
      finalizeToken(content, i);
      i--;  // current one redos if curr is not an operator.
    }

    else {
      if (content.empty()) {
        if (code[i] == 'A')
          arrM = true;
        else if (code[i] == '{') {
          strM = true;
          content.push_back(
              code[i]);  // keep '{' so the size (and start column) is right
          continue;
        }
      }
      // non operator mode
      if (content.empty() && OPERATORS.count(string(1, code[i]))) {
        content.push_back(code[i]);
        opM = true;
      } else if (isdigit(code[i]) || isalpha(code[i]) || code[i] == '_' ||
                 code[i] == '.' ||
                 (string("[]").find(code[i]) != string::npos && arrM)) {
        content.push_back(code[i]);
      } else if (PUNCTUATORS.count(code[i])) {
        if (!content.empty()) finalizeToken(content, i);
        content.push_back(code[i]);
        finalizeToken(content, i + 1);
      }  // if number or letter or underscore, keep taking as token
      else {
        // invalid char
        cout << "THROWING OUT WITH: " << code[i] << " at : " << line << endl;
        throw invalid_argument("Incorrect token");
      }
    }
  }
  if (blockComment) throw invalid_argument("Unterminated multiline comment");
  finalizeToken(content, len);
}