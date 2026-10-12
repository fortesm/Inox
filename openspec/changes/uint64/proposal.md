## Why

`UInt64` (alias `UInteger`) is in the CANON-8 numeric table, but every other
integer type is carried as an i64 holding its mathematical value, and UInt64
values above Int64.Max do not fit that model. The backend rejects any UInt64
local today (`uint64-local` GAP). This is item 3 of the order approved on
2026-10-12.

Approval (Marcelo Fortes, 2026-10-12): "Todas as tomadas de decisão dos pontos
1 a 5 foram verificadas e eleitas para serem acatadas." Item 3: UInt64.

Risk: normal.

## What Changes

- UInt64/UInteger values in locals, parameters, results, struct fields, State,
  array elements, comparisons, `case` selectors (always with `otherwise`,
  ADR-0011) and printing.
- Unsigned arithmetic: `+ - *` and `^` trap on overflow (an unsigned result
  below zero is an overflow), `div`/`mod` are unsigned, comparisons are
  unsigned, `shr` is a logical shift, shift counts are 0..63.
- Checked conversions both ways: `UInt64(X)` from a negative value and
  `Integer(U)` / `Natural(U)` from a value above Int64.Max stop the program with
  "integer conversion out of range" (kind 8); `Float(U)` rounds to the nearest Float.
- `Get` into a UInt64 accepts 0..18446744073709551615 (invalid input, kind 6).
- Constants follow the v3.44 rule: a constant that fits is read as UInt64; a
  negative constant is a compile-time range error.

Decisions (Marcelo Fortes answered on 2026-10-12, item by item, in his message "Decisões que dependem de mim"); they are now Layer A (canon v3.50):
- **Q1. Literals above Int64.Max: accepted ("→ Sim").** In a UInt64 context
  only, an integer literal (decimal or `$`/`0x` hex) may be up to 2^64-1, like
  the decimal-literal rule of CHANGE LOG v3.3. Elsewhere a literal must still
  fit Int64 (CANON-19).
- **Q2. Unary minus on UInt64: compile error.** No signed type holds every
  negated UInt64; write `-Integer(U)`.
- **Q3. Mixing with Integer variables: compile error.** `U + I` with an
  Integer variable I needs a written conversion: `U + UInt64(I)` or
  `Integer(U) + I`.

Out of scope: Int128/Crypto, Currency, UInt64 ranges or enums.

Behavior that must not change: Natural stays an Int64 subtype (ADR-0014);
other fixed-width types keep their i64-carried representation.

## Capabilities

### New Capabilities
(none)

### Modified Capabilities
- `language/fixed-width-integers`: adds UInt64.

## Impact

- `LlvmIrEmitter.cpp`: an unsigned-64 path (udiv/urem, icmp u*,
  `llvm.uadd/usub/umul.with.overflow.i64`, lshr, uitofp, printing).
- `SemanticAnalyzer.cpp`: UInt64 type rules, constant folding in the unsigned
  domain (folding needs a 64-bit unsigned evaluator besides the Int64 one).
- libinoxrt: print and parse helpers for unsigned 64-bit.
- Canon change log (and the Q1–Q3 answers as Layer A text once approved), Layer
  B matrix, docs/BACKEND_GAPS.md, docs/site/index.html, CHANGELOG.md.
