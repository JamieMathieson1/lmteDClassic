# DDD Parser Notes

A plain-language guide to how `parser.cpp` works, with the examples we went through.

## 1. What the parser does

The parser is a **recursive descent parser**. Each grammar rule has its own `parseX()` function, and each one does three things:

1. **Check** tokens are in the expected order (`expect`, `isP`, `isK`). If not, `fail()`.
2. **Decide** which rule applies by looking at the next token (e.g. `isK("if")` means call `parseIf`).
3. **Build** a node and attach the child nodes returned by the functions it calls.

Example, `parseReturn`:

```cpp
auto node = make_unique<ReturnNode>(cur());  // build a node
position++;                                  // consume 'return'
if (!isP("/")) node->value = parseExpr();    // parse the pieces, attach them
expect(TokenType::PUNCTUATOR, "/");          // validate the end
```

The output is an **AST** (abstract syntax tree). Brackets, `d`/`b` and `/` are consumed by `expect(...)` and never stored. There is no separate parse tree.

## 2. Parser vs semantic analysis

| Stage | Question it answers | Examples |
| --- | --- | --- |
| Parser | Is this shaped correctly? | Missing `]`, unexpected `b`, duplicate name |
| Semantic analysis | Does this make sense? | Use before declaration, string + int, `return` type vs `F_int_` |

The parser never looks at types. Semantic analysis is a second pass over the finished tree.

## 3. The call chain

From big structures down to small ones:

```
parse()                 whole program
└── parseItem()         function? class? or statement?
    ├── parseFunction() F_int_add[...] d ... b
    ├── parseClass()    C_Dog d ... b
    └── parseStatement()
        ├── parseIf / parseFor / parseWhile / parseDoWhile / parseReturn
        └── parseExpr() → parseAssign()
            └── parseBinary()   (| ^ & == < + - * \ %)
                └── parseUnary() → parsePower() → parsePostfix()
                    └── parsePrimary()   literals, names, ( )
```

`parseBlock` calls `parseItem`, which can hit another `if` or function, which calls `parseBlock` again. That is the recursion.

## 4. Expressions and precedence

`parseAssign` is the "left side, `=`, right side" rule:

```cpp
NodePtr lhs = parseBinary(0);          // left side
if (cur is an assignment operator) {   // the =
    node->value = parseAssign();       // right side (right associative)
}
```

`parseBinary(level)` has one level per precedence row, loosest first:

```
|    ^    &    (== !=)    (< <= > >=)    (+ -)    (* \ %)
```

It parses the next-tighter level, then loops while the next token is an operator at this level. This is the `while`-loop version of the textbook "expression prime" (E → T E') approach. Because `*` is tighter than `+`, it ends up lower in the tree.

Below the binary levels: prefix operators (`!`, `+`, `-`, `beg`, `donate`), then `**` (right associative), then postfix (`[index]`, `;member`), then primary values.

## 5. What the tree looks like

**Function**: `F_int_add[V_int_a, V_int_b] d return V_int_a + V_int_b/ b`

```
FunctionNode (F_int_add)
├── params: NameNode V_int_a, NameNode V_int_b
└── body: BlockNode
    └── ReturnNode
        └── BinaryNode +
            ├── NameNode V_int_a
            └── NameNode V_int_b

Inside a class, `lowkey` before a function marks it virtual:
`lowkey F_void_run[void] d ... b`. `highkey` marks an abstract function and
uses a signature-only form: `highkey F_void_run[void]/`, generated as
`virtual void run() = 0;`. These modifiers are class-member-only and cannot be
combined.
```

The function node holds its decoded name (`id`), its `params`, and its `body` (a `BlockNode` of statements).

**Class**: `C_Dog worships C_Animal d ... b`

```
ClassNode (C_Dog)
├── bases: [C_Animal]
└── members: [FunctionNode, ExprStmtNode, ...]
```

**If**: `if[x] d..b elseif[y] d..b catch d..b`

```
IfNode
├── cond: x
├── thenBlock: BlockNode
├── elseifs: [(y, BlockNode)]
└── elseBlock: BlockNode
```

**Loops**

```
ForNode      (condition[...]):           init, test, step, body
WhileNode    (tested[...]):              cond, body
DoWhileNode  (testing d..b tested[...]): body, cond
```

**Plain statement**: `V_int_x = 3/`

```
ExprStmtNode
└── AssignNode
    ├── target: NameNode V_int_x (isDecl = true)
    └── value: LiteralNode 3 (INT)
```

**Larger example from the test cases**: `return V_int_a + V_int_b * (V_int_a - 2)/`

```
Return
└── Add (+)
    ├── Var: V_int_a
    └── Multiply (*)
        ├── Var: V_int_b
        └── Subtract (-)
            ├── Var: V_int_a
            └── Literal: 2
```

Precedence is built into the shape. The brackets are gone because the tree structure already says `(V_int_a - 2)` happens first.

## 6. `declare()` and the scope stack

`scopes` is a **stack of sets of name strings**: one "bucket" per scope. `pushScope()` puts a new empty bucket on top, and `popScope()` throws the top one away.

`declare(name)` does two things:

- Checks **every** bucket for the name. If found anywhere, returns `false` (duplicate).
- Otherwise adds it to the **top** bucket only and returns `true`.

In `parseFunction`, `p->isDecl = declare(p->id.full)` records "this name is new", and the next line fails if it wasn't.

Example:

```
F_int_top[void] d
    V_int_x = 1/
    if[True] d
        V_int_y = 2/
    b
b
```

What `scopes` looks like as parsing goes:

```
global scope:        [ {} ]
enter top's body:    [ {}, {} ]
after V_int_x:       [ {}, {V_int_x} ]
enter if block:      [ {}, {V_int_x}, {} ]
after V_int_y:       [ {}, {V_int_x}, {V_int_y} ]
leave if block:      [ {}, {V_int_x} ]        <- V_int_y gone
leave top:           [ {} ]                   <- V_int_x gone
```

An inner scope can't redeclare a name from an outer scope, and names disappear when their scope ends.

## 7. The tree is not a symbol table

- **The tree** is the program's structure.
- **A symbol table** is a separate lookup (name → type, kind, scope).

The parser's `scopes` only stores names, has no types, and is discarded when parsing ends. The real symbol table is built in **semantic analysis**: walk the tree, and each time a `NameNode` has `isDecl = true`, add an entry (name, type, scope). On later uses, look it up. A `map<string, Info>` per scope would replace the plain sets.

## 8. What comes next

**Semantic analysis** (walk the AST, check and annotate):

- `NameNode`: if `isDecl`, add to the table. Otherwise look up, and error if missing.
- `BinaryNode`: visit both sides, check types are compatible, set the result type (int/int means integer division).
- `AssignNode`: target type must match value type.
- `CallNode`: look up the function, check argument count and types, use its return type.
- `ReturnNode`: compare with the enclosing function's declared type.
- `UnaryNode` with `donate`: mark the variable freed so later use is an error.

This needs a `type` field on the base node. Once every node has one, you have the **annotated AST**.

**Code generation** walks the annotated AST (visitor pattern, one function per node type) and emits C. For the function above:

```c
int add(int a, int b) {
    return a + b * (a - 2);
}
```

## 9. Possible bug to check

`parseItem` calls `isFunctionDef()`, but the function defined in the file is `isFunction()`. Unless `isFunctionDef` is declared elsewhere in the header, this won't compile.