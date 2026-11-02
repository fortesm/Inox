Nothing below starts before the design's approval is recorded with its date.
One pull request per step, each merged before the next (Q3).

## 0. Inventory

- [ ] 0.1 Approval of `design.md` recorded (who, date, answers to Q1-Q5)
- [ ] 0.2 `inventory.md`: every decision codegen takes today (name comparisons, side-table lookups, AST shape tests), each with the TIR node that will carry it

## 1. TIR built, not used

- [ ] 1.1 `tir/` nodes, arena, printer; `tir/Lower`; verifier (every node typed and resolved)
- [ ] 1.2 Built for every program in Debug; `--dump-tir` (Q4) and its tests
- [ ] 1.3 `tools/stack_frames.py`: codegen's recursion and the lowering added

## 2. Types and symbols from the TIR

- [ ] 2.1 Codegen takes types, symbols and constants from the TIR (lockstep with the AST, agreement asserted in Debug)

## 3. Builtins by kind

- [ ] 3.1 The analysis records each call's builtin; codegen dispatches on `Builtin::op`; no name comparison left in `codegen/`

## 4. Places and encodings

- [ ] 4.1 `__member`, `__index`, `__slice`, `__set`, `__check`, `__pure` as TIR nodes; `WithBinding` places; the analyzer's `__with` prefix test removed

## 5. Codegen walks only the TIR

- [ ] 5.1 No AST read in `codegen/`; side tables serving only codegen removed; D5 closed; `docs/ARCHITECTURE.md`

## 6. Gates (every step)

- [ ] 6.1 Suite, IR snapshot without any exception, `tools/stack_frames.py`, fuzz 1 and 7, differential, backend_gaps (0 BUG), openspec, constitution_check, version_check
- [ ] 6.2 CI: sanitized suite (1 MiB), Windows, `windows-stack` (963/451 KiB), `ir-snapshot` (label `refactor`)
