CPP = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -Iheader
SOURCES = src/compiler.cpp src/tokenizer.cpp src/parser.cpp src/Tree.cpp \
          src/cppgenerator.cpp src/SemanticAnalyser.cpp src/optimizer.cpp \
          src/utils.cpp

.PHONY: compiler clean

compiler:
	$(CPP) $(CXXFLAGS) $(SOURCES) -o compiler

clean:
	rm -f compiler

test:
	g++ -std=c++20 src/main.cpp src/cppgenerator.cpp src/SemanticAnalyser.cpp src/tokenizer.cpp src/parser.cpp src/Tree.cpp src/optimizer.cpp src/utils.cpp -o runCode
	./runCode
	g++ output.cpp -o runTest
	./runTest
