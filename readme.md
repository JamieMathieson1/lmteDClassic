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
- [Compiler stages](compiler_pipeline.md) - tokenization, parser, semantic analyser,
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

# D Classic Language Guide

This section introduces the basic syntax and features of the D Classic programming language. The examples demonstrate the language syntax currently implemented by the compiler.

## 1. Basic Syntax

D Classic uses a custom syntax rather than conventional C++ or C syntax.

Some important syntax rules include:

| Syntax | Purpose |
|---|---|
| `V_` | Variable declaration |
| `F_` | Function declaration or call |
| `C_` | Class declaration |
| `A_` | Array declaration |
| `d` | Begins a code block |
| `b` | Ends a code block |
| `/` | Terminates a statement |
| `;` | Accesses a class member |
| `[` `]` | Encloses function arguments, parameters and conditions |
| `{` `}` | Encloses string literals |

Statements are generally terminated using `/`, rather than a semicolon.

For example:

```text
V_int_number = 10/
V_int_result = V_int_number + 5/
```

### Comments

D Classic supports single-line and multiline comments.

Single-line comments begin with `FAH`:

```text
FAH This is a single-line comment.
V_int_number = 10/
```

Multiline comments begin with `FFAH` and end with `HAFF`:

```text
FFAH
This is a multiline comment.
It can span multiple lines.
HAFF
```

## 2. Data Types and Variables

Variables use the following naming convention:

`V_type_name`

The variable type is included in its identifier.

The currently supported primitive types are:

| Type | Description |
|---|---|
| `int` | Integer values |
| `float` | Floating-point values |
| `bool` | Boolean values |
| `char` | Character values |
| `string` | String values |
| `void` | No return value (used for functions) |

Class types can also be used when declaring variables.

### Variable Declaration

A variable can be declared without an initial value:

```text
V_int_age/
V_float_height/
V_bool_active/
V_string_name/
```

Alternatively, a variable can be declared and assigned a value simultaneously:

```text
V_int_age = 21/
V_float_height = 1.85/
V_bool_active = True/
```

For sting type, the value has to be declared within braces `{}`:

```text
V_string_name = {Max}/
```

For char type, the value has the be declared within sigle qutation marks `''`:

```text
V_char_grade = 'A'/
```


Boolean literals use `True` and `False`.

String literals are enclosed in curly braces rather than quotation marks.

## 3. Operators and Expressions

D Classic supports arithmetic, comparison, assignment, and bitwise operators.

### Arithmetic Operators

| Operator | Description |
|---|---|
| `+` | Addition |
| `-` | Subtraction |
| `*` | Multiplication |
| `\` | Division |
| `%` | Modulo |
| `**` | Exponentiation |

Example:

```text
V_int_a = 10/
V_int_b = 3/

V_int_sum = V_int_a + V_int_b/
V_int_difference = V_int_a - V_int_b/
V_int_product = V_int_a * V_int_b/
V_int_quotient = V_int_a \ V_int_b/
V_int_remainder = V_int_a % V_int_b/
V_int_power = V_int_a ** 2/
```

### Comparison Operators

| Operator | Description |
|---|---|
| `==` | Equal to |
| `!=` | Not equal to |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less than or equal to |
| `>=` | Greater than or equal to |

### Assignment Operators

D Classic supports standard and compound assignment:

| Operator | Description |
|---|---|
| `=` | Assignment |
| `+=` | Addition assignment |
| `-=` | Subtraction assignment |
| `*=` | Multiplication assignment |
| `\=` | Division assignment |
| `%=` | Modulo assignment |
| `**=` | Exponentiation assignment |
| `&=` | Bitwise AND assignment |
| `\|=` | Bitwise OR assignment |
| `^=` | Bitwise XOR assignment |

The language also supports the bitwise operators `&`, `|`, and `^`, as well as the unary `!` operator.

## 4. Functions

Functions use the naming convention:

`F_returnType_functionName`

Function parameters are enclosed in square brackets and separated by commas.

A function body begins with `d` and ends with `b`.

For example:

```text
F_int_add[V_int_a, V_int_b] d
    V_int_result = V_int_a + V_int_b/
    return V_int_result/
b
```

The function above accepts two integer parameters and returns their sum.

### Calling Functions

Functions are called using square brackets:

```text
F_int_main[void] d
    V_int_result = F_int_add[10, 20]/
    return 0/
b
```

Functions without parameters use `void`:

```text
F_int_main[void] d
    return 0/
b
```

The return type is included in the function identifier, and return statements use the `return` keyword.

## 5. Control Flow

D Classic supports conditional statements and several types of loops.

### If Statements

Conditional statements use `if`, followed by a condition in square brackets and a code block.

```text
F_int_main[void] d
    V_int_number = 10/

    if[V_int_number > 5] d
        V_int_number = 20/
    b

    return 0/
b
```

Additional conditions can be added using `elseif`, with an optional `catch` block for the final alternative.

```text
if[V_int_number > 10] d
    V_int_result = 1/
b
elseif[V_int_number == 10] d
    V_int_result = 2/
b
catch d
    V_int_result = 3/
b
```

### For Loops

For loops use the `cardio` keyword.

The loop declaration contains three expressions separated by commas:

1. Initialisation
2. Condition
3. Increment or step

```text
F_int_main[void] d
    V_int_sum = 0/

    cardio[V_int_i = 0, V_int_i < 5, V_int_i += 1] d
        V_int_sum += V_int_i/
    b

    return 0/
b
```

The loop executes while its condition remains true.

### While Loops

While loops use the `cardif` keyword.

```text
cardif[V_int_counter < 5] d
    V_int_counter += 1/
b
```

### Do-While Loops

Do-while loops use the `do` keyword. The body executes before the condition is checked.

```text
do d
    V_int_counter += 1/
b cardif[V_int_counter < 5]/
```

## 6. Arrays

Arrays use the `A_` identifier prefix.

The array type and size are included in the declaration:

`A_type[size]_name`

For example:

```text
A_int[3]_numbers = (10, 20, 30)/
```

Array elements are initialised using parentheses and separated by commas.

Individual elements can be accessed using square brackets:

```text
V_int_first = A_int[3]_numbers [0]/
A_int[3]_numbers [1] = 50/
```

Arrays can also be used with loops:

```text
F_int_main[void] d
    A_int[3]_numbers = (10, 20, 30)/
    V_int_sum = 0/

    cardio[V_int_i = 0, V_int_i < 3, V_int_i += 1] d
        V_int_sum += A_int[3]_numbers [V_int_i]/
    b

    return 0/
b
```

The compiler also supports recursively defined array types, allowing multidimensional array declarations.

## 7. Classes and Object-Oriented Programming

Classes use the `C_` identifier prefix.

A class definition begins with `d` and ends with `b`. Class members can include variables and functions.

```text
C_Dog d
    V_int_age/
    V_int_weight/
b
```

### Creating Objects

Class-typed variables use the class name as their type:

```text
V_Dog_myDog/
```

Class members can be accessed using the `;` operator:

```text
V_Dog_myDog;V_int_age = 5/
V_Dog_myDog;V_int_weight = 20/
```

### Inheritance

Classes can declare parent classes using the `worships` keyword:

```text
C_Animal d
    V_int_age/
b

C_Dog worships C_Animal d
    V_int_weight/
b
```

### Function Modifiers

Class member functions support two modifiers:

- `lowkey` - declares a virtual function.
- `highkey` - declares an abstract function.

For example:

```text
C_Base d
    lowkey F_void_run[void]/
    highkey F_void_reset[void]/
b
```

These modifiers must appear before the member function declaration.

## 8. Dynamic Memory Management

D Classic includes explicit heap allocation and deallocation keywords.

The `beg` keyword is used to allocate memory on the heap.

The `donate` keyword releases a heap-allocated variable.

For example:

```text
F_int_main[void] d
    V_Dog_myDog = beg V_Dog/
    donate V_Dog_myDog/
    return 0/
b
```

The semantic analyser checks that variables passed to `donate` have been heap allocated and have not already been released.

Using a variable after it has been donated is also detected as a semantic error.

