#include "test_framework.h"

int main() {
  int failures = 0;
  // Run every registered test so one failure does not hide the rest.
  for (const auto& [name, test] : test::registry()) {
    try {
      test();
      std::cout << "PASS " << name << '\n';
    } catch (const std::exception& error) {
      // Catch per-test failures so the remaining registered tests still run.
      ++failures;
      std::cout << "FAIL " << name << ": " << error.what() << '\n';
    }
  }
  std::cout << "-----\n"
            << test::registry().size() - failures << " passed, " << failures
            << " failed\n";
  // Return a nonzero status when any test fails for use in automation.
  return failures == 0 ? 0 : 1;
}

