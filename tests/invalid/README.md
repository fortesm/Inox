# Invalid Examples

These programs must fail semantic analysis.

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

`invalid-001.inox` uses an inferred local variable because typed local
declaration syntax is not yet specified canonically.
