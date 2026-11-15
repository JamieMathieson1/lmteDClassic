#include "test_framework.h"

#include "../../header/Tokenizer.h"

TEST_CASE("tokenizer classifies identifiers, operators, and literals") {
  std::string source = "V_int_count = 42/";
  Tokenizer tokenizer(source);
  REQUIRE_EQ(tokenizer.poll().getType(), TokenType::IDENTIFIER);
  REQUIRE_EQ(tokenizer.poll().getContent(), "=");
  const Token& number = tokenizer.poll();
  REQUIRE_EQ(number.getType(), TokenType::LITERAL);
  REQUIRE_EQ(number.getContent(), "42");
  REQUIRE_EQ(tokenizer.poll().getContent(), "/");
  REQUIRE(!tokenizer.canPoll());
}

TEST_CASE("tokenizer ignores line and block comments while preserving positions") {
  std::string source =
      "V_int_before = 1/ FAH ignored\n"
      "FFAH ignored\n"
      "still ignored HAFF V_int_after = 2/";
  Tokenizer tokenizer(source);
  REQUIRE_EQ(tokenizer.poll().getContent(), "V_int_before");
  while (tokenizer.poll().getContent() != "V_int_after") {
  }
  const Token& after = tokenizer.peek();
  REQUIRE_EQ(after.getLine(), 3);
  REQUIRE_EQ(tokenizer.poll().getContent(), "=");
  REQUIRE_EQ(tokenizer.poll().getContent(), "2");
  REQUIRE_EQ(tokenizer.poll().getContent(), "/");
  REQUIRE(!tokenizer.canPoll());
}

TEST_CASE("tokenizer rejects an unterminated block comment") {
  bool threw = false;
  try {
    std::string source = "FFAH never closed";
    Tokenizer tokenizer(source);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  REQUIRE(threw);
}

