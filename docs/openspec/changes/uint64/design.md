## Context

Narrow integers are carried as i64 holding the mathematical value and
range-checked after each operation (v3.44). That trick fails for UInt64: the
values above Int64.Max have no i64 mathematical representation. UInt64 needs
its own unsigned path, keyed by the semantic type of the expression
(`semantics_.typeOf`), as the narrow-type checks already are.

## Decisions

- **Representation.** i64 bit pattern, interpreted unsigned. Locals, fields,
  elements and State are i64 slots; no widening on load.
- **Arithmetic.** `+ - *` use `llvm.uadd/usub/umul.with.overflow.i64` and trap
  kind 1. `^` uses repeated checked multiplication (the existing power helper
  with an unsigned multiply). `div`/`mod` are `udiv`/`urem` with the zero check
  (kind 2). `shl` checks the count (kind 3) and overflow of the shifted value;
  `shr` is `lshr`. `bitnot` is `xor -1`.
- **Comparisons.** `icmp ult/ule/ugt/uge`; `=`/`#` unchanged.
- **Conversions.** `UInt64(I)` from a signed value: trap kind 8 when I < 0.
  `Integer(U)`/`Natural(U)`: trap kind 8 when U >u Int64.Max. Narrow targets:
  unsigned compare against the target's High. `Float(U)`: `uitofp`.
- **Constants.** The analyzer folds UInt64 constant expressions in an unsigned
  64-bit evaluator with overflow detection, separate from the Int64 folder.
  Without Q1, a UInt64 constant is an Int64 literal that is >= 0.
- **I/O.** libinoxrt gains `__inox_put_u64` and `__inox_get_u64` (invalid
  input or out of range → kind 6). Both are portable C++ (no `%llu` locale
  issues: written with `std::to_chars`/`std::from_chars`).
- **case.** Arms compare unsigned; ranges use `icmp uge/ule`. ADR-0011 already
  requires `else`.
- **for.** A UInt64 iterator uses unsigned bounds compare; the step check
  (kind 4) is unchanged.

## Risks / Trade-offs

- Every arithmetic and comparison site must branch on UInt64; a missed site is
  a silent signed operation. Mitigation: one helper per operation that takes
  the semantic type, and a test per operator with operands above Int64.Max.

## Runtime faults

Existing kinds only: 1, 2, 3, 4, 6, 8.

## Where the implementation differs

Recorded at implementation (canon r3.79); the decisions above stand
otherwise.

- **Literals.** A UInt64 context is the target of a declaration, an
  assignment, an argument, a return, a struct field, a `case` choice or a
  `UInt64(X)` conversion, and the other operand of a binary operator or the
  other branch of IIf. It reaches literals through arithmetic and bitwise
  operators and IIf branches, never into a call or an index. A literal there
  is a UInt64; a constant Integer next to a UInt64 is read as one when it is
  not negative (as `B + 1` reads 1 as a Byte).
- **Const.** A Const takes the type of its value (CANON-5), so a UInt64
  Const is `Const K := UInt64(18446744073709551615)`; `UInt64(constant)` is
  folded for that, the only conversion folded so far.
- **Helpers on demand.** The unsigned helpers (`__inox_addu_i64` ... and
  `__inox_read_u64`) are emitted only by a module that computes with UInt64,
  so the IR of every other program is unchanged.
- **Printing.** `PutLn` prints with the program's `printf("%llu")`, as
  Int64 uses `%lld`, which keeps the output in order with every other Put;
  `Get` reads through an IR helper like `__inox_read_i64`. libinoxrt gains
  only `__inox_file_write_u64`, for `Write`/`WriteLn` to a file.
- **shl.** A shift left that loses a 1 bit traps with kind 1, as the narrow
  unsigned types do; `shr` is `lshr`.
- **Indexes.** A UInt64 index above Int64.Max is out of bounds, also for an
  array whose bounds are negative.
- **case.** The analyzer orders UInt64 choices by a key that keeps the
  unsigned order in Int64 (the pattern with its top bit flipped), so overlap
  checks work unchanged.
- **Not lowered yet.** `Float(U)` and `UInt64(F)`: Float conversions are a
  later item of 0.2, for every integer type.
