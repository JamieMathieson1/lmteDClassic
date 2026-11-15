# Compiler stages

## End-to-end flow

`Compiler::run(input, output)` in [`src/Compiler.cpp`](src/Compiler.cpp)
performs the stages in this order:

```text
source file
  -> Tokenizer
  -> Parser
  -> programNode AST
  -> SemanticAnalyser
  -> Optimizer
  -> CppGenerator
  -> generated C++
```

The compiler stops with an exception if parsing or semantic analysis fails, if
an optimizer pass does not preserve the program root, or if code generation
produces no output. The command-line wrapper in
[`src/compiler.cpp`](src/compiler.cpp) prints the error and returns status 1.

## Parser

The parser is implemented by [`Parser`](header/Parser.h) and
[`src/parser.cpp`](src/parser.cpp). Its constructor drains the tokenizer into
a token vector. `parse()` creates a `programNode`, maintains a stack of
declaration scopes, and parses top-level functions, classes, or statements.
Errors are reported as `parseError` with a source line and column.

### AST shape

AST nodes are defined in [`header/Tree.h`](header/Tree.h). The root is a
`programNode` whose `items` contain declarations or statements. Expressions
use `LiteralNode`, `NameNode`, `CallNode`, `ArrayLiteralNode`, `IndexNode`,
`MemberNode`, `UnaryNode`, `BinaryNode`, and `AssignNode`. Statements use
`BlockNode`, `ExprStmtNode`, `ReturnNode`, `IfNode`, `ForNode`, `WhileNode`,
and `DoWhileNode`. Declarations use `FunctionNode` and `ClassNode`.

The parser's expression precedence, from loosest to tightest, is:

```text
assignment (right associative)
| ^ &
== !=
< <= > >=
 + -
 * \ %
unary: ! + - beg donate
** (right associative)
postfix: [index] and ;member
```

Parentheses group an expression. A comma-separated parenthesized expression
is an array literal. `/` terminates statements, and `d`/`b` delimit blocks.
The complete token and name rules are in
[`tokenization.md`](tokenization.md).

### Language feature map

| D Classic feature | Source form | AST representation | Main implementation |
| --- | --- | --- | --- |
| Variable declaration | `V_int_x/` or `V_int_x = 1/` | `NameNode`, optionally inside `AssignNode` | `parseCallOrName`, `SemanticAnalyser::checkAssignment` |
| Arrays and indexing | `A_int[3]_xs = (1, 2, 3)/`, `xs[0]` | `ArrayLiteralNode`, `IndexNode` | `parsePrimary`, `parsePostfix` |
| Functions and calls | `F_int_add[...] d ... b`, `F_int_add[...]` | `FunctionNode`, `CallNode` | `parseFunction`, `parseCallOrName` |
| Classes and inheritance | `C_Dog d ... b`, `C_Dog worships C_Animal d ... b` | `ClassNode` | `parseClass` |
| Member access | `V_Dog_r;V_int_age` | `MemberNode` | `parsePostfix` |
| Conditionals | `if[...] d ... b`, `elseif`, `catch` | `IfNode` | `parseIf` |
| Loops | `condition[init,test,step]`, `tested[...]`, `testing d ... b tested[...]` | `ForNode`, `WhileNode`, `DoWhileNode` | `parseFor`, `parseWhile`, `parseDoWhile` |
| Returns | `return expression/` or `return/` | `ReturnNode` | `parseReturn` |
| Heap operations | `beg` and `donate` | `UnaryNode` | `parseUnary`, semantic heap flag |
| Operators | arithmetic, comparison, logical, bitwise, assignment | `UnaryNode`, `BinaryNode`, `AssignNode` | `parseUnary`, `parseBinary`, `parseAssign` |
| Virtual/abstract members | `lowkey F_...`, `highkey F_.../` | `FunctionNode` flags | `parseClass`, `parseFunction` |

`Ident::decode` in [`header/Tree.h`](header/Tree.h) is the central mapping from
the language's prefixed names (`V_`, `A_`, `F_`, and `C_`) to the identifier
kind, type, name, and array dimensions.

## Semantic analyser

[`SemanticAnalyser`](header/SemanticAnalyser.h) traverses the AST in
[`src/SemanticAnalyser.cpp`](src/SemanticAnalyser.cpp). `analyse()` resets
state, creates the global scope, checks the program, and returns false when
any errors were recorded. Use `getErrors()` to inspect all diagnostics.

The analyser:

- maintains nested symbol-table scopes and resolves names from the innermost
  scope outward;
- recognises `int`, `float`, `bool`, `char`, `string`, `void`, arrays, and
  class types;
- checks declarations, duplicate names, assignments, compound assignments,
  calls, array element consistency, array indices, operators, and returns;
- requires `if`, `elseif`, `for`, `while`, and `do-while` conditions to be
  boolean when their type is known;
- checks function return types and reports non-void functions whose paths do
  not all return a value;
- tracks whether a declaration was initialized through `beg` using
  `Symbol::isHeap`.

Some AST types are intentionally conservative: indexing, member access, and
some memory expressions can resolve to `UNKNOWN`, allowing later analysis
until richer class/array symbol information is available. `UNKNOWN` is not
silently treated as an error; invalid and incompatible known types are.

## Optimizer

[`Optimizer::optimize`](header/Optimizer.h) in
[`src/optimizer.cpp`](src/optimizer.cpp) runs these passes:

1. **Constant folding** recursively evaluates numeric, boolean, unary, binary,
   and constant conditional expressions. It preserves source locations on
   replacement literals and avoids folding values that overflow or are not
   safely representable.
2. **Fixed-count loop unrolling** recognizes a `ForNode` with a statically
   understandable integer initializer, comparison, and step. It replaces the
   loop with a block containing the initializer, copied body statements, and
   each step. Loops with unsupported shapes or declaration hazards remain
   unchanged.
3. **Dead-code elimination** removes statements after an unconditional
   `return` and removes unused, side-effect-free variable declarations using
   backwards liveness information.

Constant folding runs again after loop unrolling so expressions introduced by
the unrolled loop can also be simplified. The optimizer mutates the same
`NodePtr` tree and must preserve a `programNode` root for the generator.
