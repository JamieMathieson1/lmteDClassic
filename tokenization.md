# Tokenization

`Tokenizer t(code)` splits the source into a `vector<Token>` at construction. Read it with `canPoll()`, `peek()`, `poll()`.
Each `Token` has `content`, `type`, `line`, and `charNum` (1-based column where the token starts).
Any invalid character or token throws `invalid_argument("Incorrect token")`.

## Names
Every name has a prefix saying what it is. The final name part is letters and digits only (no `_`).

| Kind | Form | Example |
|---|---|---|
| Variable | `V_<type>_<name>` | `V_int_x`, `V_string_s` |
| Function | `F_<returnType>_<name>` | `F_int_add`, `F_void_bark` |
| Class | `C_<name>` | `C_Dog` |
| Array | `A_<elem>[<size>]_<name>` | `A_int[3]_nums` |

**Types:** `int bool char string float void`.

**Arrays:** `<elem>` is a type or another array type, so arrays nest: `A_A_int[2][3]_grid`.
The last `[size]` belongs to the outermost array: `A_A_int[2][3]` is 3 elements of `A_int[2]`. Sizes are digits only.

## Structure
```
F_int_add[V_int_a, V_int_b] d     function: params in [ ], body in d ... b
  return V_int_a + V_int_b/
b

C_Dog d                            class: body in d ... b
  V_int_age/
b

lowkey F_int_bark[void] d          virtual class member function
  return 0/
b

highkey F_void_feed[V_int_x]/       abstract class member function

V_int_x = 10/                      variable; '/' ends a statement
A_int[3]_nums = (1, 2, 3)/         array literal in ( )
V_int_s = F_int_add[V_int_x, 4]/   call: args in [ ]
if[V_int_x >= 10] d ... b          control flow: condition in [ ]
condition[init, test, step] d ... b
```

## Token types
| Type | Matches |
|---|---|
| `OPERATOR` | `+ - * ** \ %`, `< <= == > >= !=`, `= += -= *= **= \= %=`, `& \| ^ &= \|= ^= !` |
| `KEYWORD` | `if elseif catch condition tested testing return worships beg donate lowkey highkey`, plus the type names |
| `PUNCTUATOR` | `[ ] ( ) , ; / d b` (`d`/`b` only when they stand alone) |
| `LITERAL` | numbers (digits, at most one `.`), `True`/`False`, strings `{...}` |
| `IDENTIFIER` | the name forms above |

Strings are wrapped in `{ }`, can contain spaces, and can't nest.

## Comments

`FAH` starts a single-line comment; all characters through the next newline
are ignored. `FFAH` starts a multiline comment, which ends at the next `HAFF`.
Multiline comments may contain newlines, and an unclosed multiline comment is
invalid. Comments are discarded before parsing and do not appear in generated
C++.

## How it works
One pass over the characters, building `content` until the token ends, then `finalizeToken` trims it, classifies it (checks in the table's order, `OPERATOR` first), and stores it.

- **Space / newline:** end the current token. A newline also increments `line`.
- **Punctuator:** ends the current token and becomes a one-character token.
- **Letters, digits, `_`, `.`:** added to the current token.
- **Operator mode** (`opM`): an operator char starts it. The next char joins only if it is also an operator char (2-char max, except `**=`).
- **String mode** (`strM`): a `{` at token start takes every char up to `}`.
- **Array mode** (`arrM`): a token starting with `A` may contain `[` and `]`.

All modes reset after each token.
