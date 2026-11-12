#include "../../header/Tokenizer.h"
#include <cassert>
#include <stdexcept>

int main() {
  std::string source =
      "V_int_before = 1/ FAH ignored\n"
      "FFAH ignored\n"
      "still ignored HAFF V_int_after = 2/";
  Tokenizer tokenizer(source);

  assert(tokenizer.canPoll());
  assert(tokenizer.poll().getContent() == "V_int_before");
  assert(tokenizer.poll().getContent() == "=");
  assert(tokenizer.poll().getContent() == "1");
  assert(tokenizer.poll().getContent() == "/");
  const Token& after = tokenizer.poll();
  assert(after.getContent() == "V_int_after");
  assert(after.getLine() == 3);
  assert(tokenizer.poll().getContent() == "=");
  assert(tokenizer.poll().getContent() == "2");
  assert(tokenizer.poll().getContent() == "/");
  assert(!tokenizer.canPoll());

  bool threw = false;
  try {
    std::string unterminated = "FFAH never closed";
    Tokenizer invalid(unterminated);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  assert(threw);
}
