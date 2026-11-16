#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../../header/Compiler.h"

namespace fs = std::filesystem;

static const fs::path IN_DIR = "tests/compilerIntegrationTests/input";
static const fs::path OUT_DIR = "tests/compilerIntegrationTests/output";
static const fs::path ACTUAL_DIR = "/tmp/lmteDClassic-integration-tests";

static std::string readFile(const fs::path& p) {
  // Read generated and expected files identically for exact comparisons.
  std::ifstream f(p, std::ios::binary);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static void writeFile(const fs::path& path, const std::string& contents) {
  std::ofstream output(path, std::ios::binary);
  if (!output) {
    throw std::runtime_error("Could not write " + path.string());
  }
  output << contents;
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

static bool runTest(const fs::path& input, bool update) {
  // Compile one fixture, then compare or refresh its expected output.
  const std::string name = input.stem().string();
  fs::path actual = ACTUAL_DIR / (name + ".cpp");
  fs::path expected = OUT_DIR / (name + ".cpp");

  if (!update && !fs::exists(expected)) {
    std::cout << "FAIL  " << name << "  (missing " << expected.string()
              << ")\n";
    return false;
  }

  fs::create_directories(ACTUAL_DIR);
  try {
    if (Compiler::run(input.string(), actual.string()) != 0) {
      std::cout << "FAIL  " << name << "  (compiler returned an error)\n";
      return false;
    }
  } catch (const std::exception& error) {
    std::cout << "FAIL  " << name << "  (" << error.what() << ")\n";
    return false;
  }
  const std::string generated = readFile(actual);

  if (update) {
    writeFile(expected, generated);
    std::cout << "UPDATED  " << name << "  -> " << expected.string() << "\n";
    return true;
  }

  auto a = splitLines(generated);
  auto e = splitLines(readFile(expected));
  if (a == e) {
    std::cout << "PASS  " << name << "\n";
    return true;
  }

  std::cout << "FAIL  " << name << "  (" << actual.string() << " differs from "
            << expected.string() << ")\n";
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

static std::vector<fs::path> discoverTests() {
  // Run fixtures in sorted order so output is deterministic.
  std::vector<fs::path> inputs;
  if (!fs::exists(IN_DIR)) return inputs;
  for (const auto& entry : fs::directory_iterator(IN_DIR)) {
    if (entry.path().extension() == ".dc") inputs.push_back(entry.path());
  }
  std::sort(inputs.begin(), inputs.end());
  return inputs;
}

int main(int argc, char* argv[]) {
  // Explicit file names select tests; with no names, discover every fixture.
  bool update = false;
  std::vector<fs::path> inputs;
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--update")
      update = true;
    else if (!arg.empty())
      inputs.push_back(IN_DIR / arg);
  }

  fs::create_directories(OUT_DIR);
  if (inputs.empty()) inputs = discoverTests();
  if (inputs.empty()) {
    std::cout << "No tests found in " << IN_DIR.string() << "/\n";
    return 1;
  }

  int pass = 0, fail = 0;
  for (const auto& input : inputs) (runTest(input, update) ? pass : fail)++;

  std::cout << "-----\n" << pass << " passed, " << fail << " failed\n";
  return fail == 0 ? 0 : 1;
}