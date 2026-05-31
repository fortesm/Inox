# Invalid Examples

These programs must fail parsing or semantic analysis.

| File | Expected diagnostic |
| --- | --- |
| `invalid-001.inox` | type mismatch: cannot assign `String` to `Int64` |
| `invalid-002.inox` | condition must be `Bool` |
| `invalid-003.inox` | `bitand` requires integer operands |
| `invalid-004.inox` | `Length` expects `String` |
| `invalid-005.inox` | `Ord` expects `Char` |
| `invalid-006.inox` | `break` outside loop |
| `invalid-007.inox` | `continue` outside loop |
| `invalid-008.inox` | cannot assign `Bool` to `Int64` |
| `invalid-009.inox` | cannot assign `Int64` to `Bool` |
| `invalid-010.inox` | cannot assign `Int64` to `String` |
| `invalid-011.inox` | cannot assign `String` to `Char` |
| `invalid-012.inox` | cannot return `String` from a function returning `Int64` |
| `invalid-013.inox` | `+` rejects incompatible operands |
| `invalid-014.inox` | `*` requires numeric operands |
| `invalid-015.inox` | `<` rejects incompatible operands |
| `invalid-016.inox` | `and` requires `Bool` operands |
| `invalid-017.inox` | `not` requires a `Bool` operand |
| `invalid-018.inox` | `bitor` requires integer operands |
| `invalid-019.inox` | `bitnot` requires an integer operand |
| `invalid-020.inox` | `shl` requires integer operands |
| `invalid-021.inox` | call to an unknown function |
| `invalid-022.inox` | wrong number of function arguments |
| `invalid-023.inox` | incompatible function argument type |
| `invalid-024.inox` | subroutine without return type cannot return a value |
| `invalid-025.inox` | function with return type must return a value |
| `invalid-026.inox` | `if` branch must not use `:` |
| `invalid-027.inox` | `;` must not appear between `if` and `else` branches |
| `invalid-028.inox` | conditional header must end with a line break |
| `invalid-029.inox` | `until` outside `repeat` |
| `invalid-030.inox` | `until` condition must be `Bool` |

`invalid-001.inox` uses an inferred local variable because typed local
declaration syntax is not yet specified canonically.
