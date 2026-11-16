#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace test {
using Test = std::pair<std::string, std::function<void()>>;

inline std::vector<Test>& registry() {
  // Keep the registry alive so tests can register during static initialization.
  static std::vector<Test> tests;
  return tests;
}

struct Registrar {
  Registrar(const char* name, std::function<void()> fn) {
    // Static registrars make each TEST_CASE available to the runner.
    registry().emplace_back(name, std::move(fn));
  }
};

inline void require(bool condition, const char* expression, const char* file,
                    int line) {
  if (!condition) {
    // Include source location and expression so failures identify the assertion.
    std::ostringstream message;
    message << file << ':' << line << ": requirement failed: " << expression;
    throw std::runtime_error(message.str());
  }
}

template <typename A, typename B>
void requireEqual(const A& actual, const B& expected, const char* actualText,
                  const char* expectedText, const char* file, int line) {
  if (!(actual == expected)) {
    // Preserve the expressions because generic values may not support streaming.
    std::ostringstream message;
    message << file << ':' << line << ": expected " << expectedText
            << ", got " << actualText;
    throw std::runtime_error(message.str());
  }
}
}  // namespace test

#define TEST_CONCAT_IMPL(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT_IMPL(a, b)
// Define a uniquely named function and register it before main starts.
#define TEST_CASE(name)                                                        \
  static void TEST_CONCAT(test_, __LINE__)();                                 \
  static ::test::Registrar TEST_CONCAT(registrar_, __LINE__)(                 \
      name, TEST_CONCAT(test_, __LINE__));                                    \
  static void TEST_CONCAT(test_, __LINE__)()

#define REQUIRE(condition)                                                      \
  ::test::require((condition), #condition, __FILE__, __LINE__)

// Compare values while retaining both source expressions in failure output.
#define REQUIRE_EQ(actual, expected)                                            \
  ::test::requireEqual((actual), (expected), #actual, #expected, __FILE__,    \
                       __LINE__)

