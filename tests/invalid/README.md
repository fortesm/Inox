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

`invalid-001.inox` uses an inferred local variable because typed local
declaration syntax is not yet specified canonically.
