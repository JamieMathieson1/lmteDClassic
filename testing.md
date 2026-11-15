# Testing guide

The project has two test tiers. Both are built with the C++20 settings in
[`makefile`](makefile), using the small local framework in
[`tests/unitTests/test_framework.h`](tests/unitTests/test_framework.h).

## Unit tests

Unit tests construct tokens or AST nodes directly and test one compiler
component without writing compiler fixtures. The files are:

| File | Focus |
| --- | --- |
| `tokenizer_unit_tests.cpp` | token kinds, names, comments, literals, and invalid input |
| `parser_unit_tests.cpp` | AST construction, precedence, blocks, and parse errors |
| `semantic_analyser_unit_tests.cpp` | declarations, types, scopes, conditions, and diagnostics |
| `optimizer_unit_tests.cpp` | constant folding, loop unrolling, and dead-code elimination |
| `codegen_unit_tests.cpp` | generated C++ for representative ASTs |

Run the complete unit tier:

```sh
make unit-tests
```

The executable is created at `/tmp/lmteDClassic-unit-tests`, and the runner
prints each registered test followed by a passed/failed summary. Tests are
registered with `TEST_CASE`; assertions are `REQUIRE` and `REQUIRE_EQ`.
There is no name filter, so a focused change is normally added to the
component's existing test file and the whole unit executable is run.

### Writing a unit test

For parser behavior, add a source string to
[`parser_unit_tests.cpp`](tests/unitTests/parser_unit_tests.cpp), tokenize it,
call `Parser::parse()`, and inspect the relevant `NodeKind` and fields. For
semantic or optimizer behavior, follow the small AST builders already present
in their respective files. Keep tests focused on observable behavior: a
valid case, the expected AST/type/result, or the expected diagnostic.

For example, a parser test for a new expression can use:

```cpp
NodePtr ast = parseSource("F_int_main[void]d return 1 + 2 * 3/b");
auto* function = as<FunctionNode>(as<programNode>(ast.get())->items[0].get());
auto* ret = as<ReturnNode>(as<BlockNode>(function->body.get())->stmts[0].get());
REQUIRE_EQ(as<BinaryNode>(ret->value.get())->op, std::string("+"));
```

## Compiler integration tests

Integration tests run the complete compiler pipeline and compare generated
C++ with checked-in expected output. Their layout is:

```text
tests/compilerIntegrationTests/
  input/*.dc       D Classic source fixtures
  output/*.cpp     expected generated C++
  src/*.cpp        fixture runner
```

Run all fixtures:

```sh
make compiler-tests
```

The runner discovers every `.dc` file in `input`, writes actual output under
`/tmp/lmteDClassic-integration-tests`, ignores trailing whitespace and blank
lines, and reports the first ten differing lines. The integration executable
also accepts fixture names after it has been built, for example:

```sh
/tmp/lmteDClassic-compiler-integration-tests basics.dc
```

## Adding a bespoke compiler test

1. Add a source program such as
   [`tests/compilerIntegrationTests/input/basics.dc`](tests/compilerIntegrationTests/input/basics.dc).
   Use the prefixed names, `/` statement terminators, and `d`/`b` blocks
   described in [`tokenization.md`](tokenization.md).
2. Generate its expected C++:

   ```sh
   make update-compiler-tests
   ```

   This updates every expected fixture, so review the diff and keep only the
   intended generated file changes.
3. Run the comparison:

   ```sh
   make compiler-tests
   ```

4. For a full pre-change check, run:

   ```sh
   make testall
   ```

The `--update` mode is a deliberate approval step, not a test assertion:
never use it to hide an unexpected compiler change. If a fixture should no
longer compile, add or update a unit test for the expected parser or semantic
diagnostic instead of recording generated output.

## Useful commands

```sh
make                 # build ./compiler
make test             # unit-tests (the default test target)
make testall          # unit-tests, then compiler-tests
make clean            # remove compiler and legacy test binaries
make clean-output    # remove legacy output*.txt files
```
