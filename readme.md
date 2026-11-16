# D Classic Compiler

D Classic source is compiled to C++ in five stages:

1. The tokenizer converts source text into tokens.
2. The parser builds an abstract syntax tree (AST).
3. The semantic analyser checks names, scopes, types, and control-flow rules.
4. The optimizer rewrites the AST.
5. The C++ generator emits the final source file.

The compiler entry point is `src/compiler.cpp`; the reusable pipeline is
`Compiler::run` in `src/Compiler.cpp`.

## Documentation

- [Project layout](layout.md) - source, headers, generated output, and fixtures.
- [Tokenization and source syntax](tokenization.md) - names, literals, operators,
  comments, and delimiters.
- [Compiler stages](compiler_pipeline.md) - parser, semantic analyser,
  optimizer, AST nodes, and language-feature implementation map.
- [Testing guide](testing.md) - unit tests, compiler integration tests, and how to add cases.

## Quick start

Build the compiler and compile a source file:

```sh
make
```

Then run compiled machine code: 

```sh
./compiler <input_file> <output_file>
Example: 
./compiler tests/compilerIntegrationTests/input/basics.dc output/generated.cpp
```

Run all tests:

```sh
make testall
```