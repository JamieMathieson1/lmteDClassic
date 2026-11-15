#include "test_framework.h"

int main() {
  int failures = 0;
  for (const auto& [name, test] : test::registry()) {
    try {
      test();
      std::cout << "PASS " << name << '\n';
    } catch (const std::exception& error) {
      ++failures;
      std::cout << "FAIL " << name << ": " << error.what() << '\n';
    }
  }
  std::cout << "-----\n"
            << test::registry().size() - failures << " passed, " << failures
            << " failed\n";
  return failures == 0 ? 0 : 1;
}

