## 1. Decisions

- [x] 1.1 Record Marcelo's floating-point policy of 2026-10-21 as r3.86; open question 3 leaves the list

## 2. Compiler

- [x] 2.1 Float32 literals rounded once from their digits (analyzer, folding, emitter, field defaults)
- [x] 2.2 Universal real Consts: a Float32 in a Float32 context; computed Consts stay typed
- [x] 2.3 Std.Math: Pi, Tau and E with more digits than either type holds

## 3. Tests

- [x] 3.1 float-conversions: tie literal, Pi and a negated Const as Float32, NaN, infinities and underflow
- [x] 3.2 Diagnostics float32-from-float64-variable, float32-typed-const

## 4. Gates (E19)

- [x] 4.1 Suite, ASan+UBSan, fuzzing, backend_gaps, leak check, openspec validate, constitution check
- [ ] 4.2 Linux and Windows CI green
