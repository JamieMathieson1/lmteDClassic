#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../../header/CppGenerator.h"
#include "../../header/Parser.h"
#include "../../header/SemanticAnalyser.h"
#include "../../header/Tokenizer.h"
#include "../../header/Tree.h"

namespace fs = std::filesystem;

static const fs::path IN_DIR = "tests/unitTests/testCase";
static const fs::path OUT_DIR = "tests/unitTests/output";

static std::string readFile(const fs::path& p) {
  std::ifstream f(p, std::ios::binary);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static void writeFile(const fs::path& p, const std::string& text) {
  std::ofstream f(p, std::ios::binary);
  f << text;
}

// Split into lines, ignoring \r, trailing spaces, and trailing blank lines.
static std::vector<std::string> splitLines(const std::string& s) {
  std::vector<std::string> lines;
  std::string cur;
  auto flush = [&] {
    while (!cur.empty() && (cur.back() == ' ' || cur.back() == '\t'))
      cur.pop_back();
    lines.push_back(cur);
    cur.clear();
  };
  for (char c : s) {
    if (c == '\r') continue;
    if (c == '\n')
      flush();
    else
      cur += c;
  }
  if (!cur.empty()) flush();
  while (!lines.empty() && lines.back().empty()) lines.pop_back();
  return lines;
}

// Runs the full pipeline. Errors are returned as text so error cases can
// also be compared against an Eoutput file.
static std::string compile(std::string source) {
  try {
    Tokenizer tokenizer(source);
    Parser p(tokenizer);
    NodePtr ast = p.parse();

    auto* program = as<programNode>(ast.get());
    if (!program) return "Error: Invalid program AST\n";

    SemanticAnalyser semantic;
    if (!semantic.analyse(*program)) {
      std::string out = "--- SEMANTIC ERRORS ---\n";
      for (const std::string& err : semantic.getErrors()) out += err + '\n';
      return out;
    }

    CppGenerator generator;
    generator.scope_list = &semantic.getScopeList();
    generator.generate(*program);
    return generator.getCode();
  } catch (const std::exception& e) {
    return std::string("Exception: ") + e.what() + "\n";
  }
}

static bool runTest(const std::string& name, bool update) {
  fs::path in = IN_DIR / ("input" + name + ".txt");
  fs::path out = OUT_DIR / ("output" + name + ".txt");
  fs::path exp = OUT_DIR / ("Eoutput" + name + ".txt");

  if (!fs::exists(in)) {
    std::cout << "FAIL  " << name << "  (missing " << in.string() << ")\n";
    return false;
  }

  std::string actual = compile(readFile(in));
  writeFile(out, actual);

  if (update) {
    writeFile(exp, actual);
    std::cout << "UPDATED  " << name << "  -> " << exp.string() << "\n";
    return true;
  }

  if (!fs::exists(exp)) {
    std::cout << "FAIL  " << name << "  (missing " << exp.string() << ")\n";
    return false;
  }

  auto a = splitLines(actual);
  auto e = splitLines(readFile(exp));
  if (a == e) {
    std::cout << "PASS  " << name << "\n";
    return true;
  }

  std::cout << "FAIL  " << name << "  (" << out.string() << " differs from "
            << exp.string() << ")\n";
  int shown = 0;
  for (size_t i = 0; i < std::max(a.size(), e.size()) && shown < 10; i++) {
    std::string el = i < e.size() ? e[i] : "<missing>";
    std::string al = i < a.size() ? a[i] : "<missing>";
    if (el == al) continue;
    std::cout << "      line " << i + 1 << ":\n"
              << "        expected: " << el << "\n"
              << "        actual:   " << al << "\n";
    shown++;
  }
  return false;
}

static std::vector<std::string> discoverTests() {
  std::vector<std::string> names;
  if (!fs::exists(IN_DIR)) return names;
  for (const auto& entry : fs::directory_iterator(IN_DIR)) {
    std::string file = entry.path().filename().string();
    if (entry.path().extension() == ".txt" && file.rfind("input", 0) == 0)
      names.push_back(entry.path().stem().string().substr(5));
  }
  std::sort(names.begin(), names.end());
  return names;
}

int main(int argc, char* argv[]) {
  bool update = false;
  std::vector<std::string> names;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--update")
      update = true;
    else if (!arg.empty())
      names.push_back(arg);
  }

  fs::create_directories(OUT_DIR);
  if (names.empty()) names = discoverTests();
  if (names.empty()) {
    std::cout << "No tests found in " << IN_DIR.string() << "/\n";
    return 1;
  }

  int pass = 0, fail = 0;
  for (const auto& n : names) (runTest(n, update) ? pass : fail)++;

  std::cout << "-----\n" << pass << " passed, " << fail << " failed\n";
  return fail == 0 ? 0 : 1;
}