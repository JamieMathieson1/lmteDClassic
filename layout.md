# Project Layout

The D Classic Compiler is organised into separate directories for source code, header files, generated output, and testing. The compiler implementation is contained within `src/`, while `header/` contains the corresponding class and data structure declarations. Testing is divided into unit tests and compiler integration tests.

## Directory Structure

```text
lmteDClassic/
│
├── header/
│   ├── Compiler.h
│   ├── CppGenerator.h
│   ├── Optimizer.h
│   ├── Parser.h
│   ├── SemanticAnalyser.h
│   ├── Token.h
│   ├── Tokenizer.h
│   ├── Tree.h
│   └── Utils.h
│
├── src/
│   ├── Compiler.cpp
│   ├── SemanticAnalyser.cpp
│   ├── Tree.cpp
│   ├── compiler.cpp
│   ├── cppgenerator.cpp
│   ├── optimizer.cpp
│   ├── parser.cpp
│   ├── tokenizer.cpp
│   └── utils.cpp
│
├── output/
│   └── generated.cpp
│
├── tests/...
│
├── compiler_pipeline.md
├── testing.md
├── layout.md
├── readme.md
└── makefile
```

## Main Directories

### `src/` – Source Code

Contains the implementation of the compiler and its five main stages.

| File | Description |
|---|---|
| `compiler.cpp` | Command-line entry point. Receives the input and output file paths and calls the compiler pipeline. |
| `Compiler.cpp` | Implements `Compiler::run()`, coordinating the complete compilation process. |
| `tokenizer.cpp` | Implements lexical analysis, converting D Classic source code into tokens. |
| `parser.cpp` | Implements syntax analysis, constructing the abstract syntax tree (AST). |
| `SemanticAnalyser.cpp` | Checks variable declarations, scopes, types, function calls, and control-flow rules. |
| `optimizer.cpp` | Optimizes the AST through constant folding, loop unrolling, and dead-code elimination. |
| `cppgenerator.cpp` | Traverses the optimized AST and generates equivalent C++ source code. |
| `Tree.cpp` | Implements AST-related functionality. |
| `utils.cpp` | Implements shared utility functions used throughout the compiler. |

### `header/` – Header Files

Contains the declarations and interfaces for the compiler components. These headers allow different implementation files to access the required classes, structures, and functions.

| File | Description |
|---|---|
| `Compiler.h` | Declares the main `Compiler` class and its `run()` function. |
| `Tokenizer.h` | Declares the tokenizer and defines the language's operators, keywords, types, and punctuators. |
| `Token.h` | Defines the `Token` class and `TokenType` enumeration. |
| `Parser.h` | Declares the parser, parsing functions, and syntax error handling. |
| `Tree.h` | Defines the AST node structures, node types, and identifier representation. |
| `SemanticAnalyser.h` | Declares semantic analysis functionality, symbol tables, and type information. |
| `Optimizer.h` | Declares the AST optimization passes. |
| `CppGenerator.h` | Declares the C++ code generator and its AST traversal functions. |
| `Utils.h` | Declares shared utility functions, including string trimming and token type conversion. |

### `output/` – Generated Code

Contains generated C++ output from the compiler.

- `generated.cpp` is the default output file used when compiling a D Classic source program.
- The output path can also be specified through the command-line arguments.

Generated code can be compiled separately using a C++ compiler.

## Testing Structure

The project uses two levels of testing to verify individual compiler components and the complete compilation pipeline.

### `tests/unitTests/`

Contains tests for individual compiler components. These tests construct tokens or AST nodes directly to check functionality without requiring a complete source-to-C++ compilation.

| File | Testing Focus |
|---|---|
| `tokenizer_unit_tests.cpp` | Token recognition, identifiers, literals, comments, and invalid input. |
| `parser_unit_tests.cpp` | AST construction, expression precedence, blocks, and parsing errors. |
| `semantic_analyser_unit_tests.cpp` | Type checking, declarations, scopes, and semantic errors. |
| `optimizer_unit_tests.cpp` | Constant folding, loop unrolling, and dead-code elimination. |
| `codegen_unit_tests.cpp` | Correct generation of C++ from AST structures. |
| `test_framework.h` | Lightweight testing framework containing test registration and assertion functions. |
| `test_runner.cpp` | Registers and executes the unit tests. |

The `output/` subdirectory contains additional expected output files used by some unit tests.

### `tests/compilerIntegrationTests/`

Tests the complete compiler pipeline using D Classic source files and their expected generated C++ output.

| Directory/File | Description |
|---|---|
| `input/` | Contains `.dc` source programs used as compiler test fixtures. |
| `output/` | Contains the expected `.cpp` output corresponding to each input fixture. |
| `src/compiler_integration_tests.cpp` | Implements the integration test runner, compiling fixtures and comparing generated output against expected results. |

Each input fixture has a corresponding expected C++ file. This allows the integration tests to verify that all compiler stages work together correctly.

## Supporting Files

| File | Description |
|---|---|
| `makefile` | Defines the build process and commands for running unit tests, integration tests, and the complete test suite. |
| `readme.md` | Provides the project overview, documentation links, and instructions for building and running the compiler. |
| `compiler_pipeline.md` | Documents the compiler stages, AST structure, language features, and implementation details. |
| `testing.md` | Explains the testing structure, available commands, and how to add new tests. |

