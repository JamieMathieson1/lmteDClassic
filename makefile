CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -Iheader

CORE_SRCS = src/cppgenerator.cpp src/SemanticAnalyser.cpp src/tokenizer.cpp \
            src/parser.cpp src/Tree.cpp src/optimizer.cpp src/utils.cpp
UNIT_SRCS = tests/unitTests/test_runner.cpp \
            tests/unitTests/tokenizer_unit_tests.cpp \
            tests/unitTests/parser_unit_tests.cpp \
            tests/unitTests/optimizer_unit_tests.cpp \
            tests/unitTests/semantic_analyser_unit_tests.cpp \
            tests/unitTests/codegen_unit_tests.cpp

.PHONY: all compiler unit-tests compiler-tests test testall clean clean-output

all: compiler

compiler:
	$(CXX) $(CXXFLAGS) src/compiler.cpp $(CORE_SRCS) -o compiler

unit-tests:
	$(CXX) $(CXXFLAGS) $(UNIT_SRCS) $(CORE_SRCS) -o /tmp/lmteDClassic-unit-tests
	/tmp/lmteDClassic-unit-tests

compiler-tests:
	$(CXX) $(CXXFLAGS) tests/compilerIntegrationTests/unitTest.cpp $(CORE_SRCS) -o /tmp/lmteDClassic-pipeline-tests
	/tmp/lmteDClassic-pipeline-tests
	$(CXX) $(CXXFLAGS) tests/compilerIntegrationTests/tokenTester.cpp src/tokenizer.cpp src/utils.cpp -o /tmp/lmteDClassic-tokenizer-test
	/tmp/lmteDClassic-tokenizer-test /tmp/lmteDClassic-tokens.txt \
		tests/compilerIntegrationTests/simple.dc tests/compilerIntegrationTests/large.dc
	$(CXX) $(CXXFLAGS) tests/compilerIntegrationTests/optimizer/optimizer_test.cpp $(CORE_SRCS) -o /tmp/lmteDClassic-optimizer-test
	/tmp/lmteDClassic-optimizer-test
	$(CXX) $(CXXFLAGS) tests/compilerIntegrationTests/semanticAnalyser/semantic_analyser_test.cpp $(CORE_SRCS) -o /tmp/lmteDClassic-semantic-test
	/tmp/lmteDClassic-semantic-test

test: unit-tests

testall: unit-tests compiler-tests

clean:
	rm -f compiler runTests runTests.dbg

clean-output:
	rm -f tests/compilerIntegrationTests/output/output*.txt
