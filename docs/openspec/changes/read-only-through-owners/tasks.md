## 0. Authorization

- [x] 0.1 Marcelo Fortes approves the design (high risk) and authorizes the implementation, 2026-10-31

## 1. Baseline

- [x] 1.1 `tools/ir_snapshot.py record` on the suite

## 2. Semantic check

- [x] 2.1 `Symbol.deepReadOnly`: `ref` parameters, plain `Self`, `with` bindings of deep read-only targets
- [x] 2.2 `writableRoot`: owners keep the read-only of their root; Links stay writable; value parameters keep their owned objects writable
- [x] 2.3 `Self mut` receiver through an owner; `Value(P) :=`; `Free`/`Move` of `Option<Own>` places
- [x] 2.4 `Link(...)` rejects deep read-only places and owners
- [x] 2.5 Diagnostics naming the root and the fix

## 3. Tests

Negative (tests/diagnostics, each `.inox` + `.err`):
- [x] 3.1 `ref-owner-write`: `ref P Own<TNode>`, `P.FValue := 1`
- [x] 3.2 `ref-nested-owner-write`: `ref H THolder`, `H.FChild.FValue := 1`
- [x] 3.3 `ref-owner-deep`: two owner levels, `H.FChild.FNext.FValue := 1`
- [x] 3.4 `ref-owner-compound`: `H.FChild.FValue += 1`
- [x] 3.5 `ref-owner-element`: an owned array, `P[1] := 5`
- [x] 3.6 `ref-owner-ref-mut-argument`: `Inc(H.FChild.FValue)` with `Inc(ref mut X Integer)`
- [x] 3.7 `ref-owner-self-mut-call`: `H.FChild.Bump` with `TNode.Bump(Self mut)`
- [x] 3.8 `ref-owner-value-write`: `Value(P) := TNode(FValue := 1)` with `ref P`
- [x] 3.9 `ref-owner-with`: `with H.FChild` then `.FValue := 1`
- [x] 3.10 `ref-owner-free-option`: `Free(H.FOptional)` with `FOptional Option<Own<TNode>>`
- [x] 3.10b `ref-owner-move-option`: `Move(H.FOptional)` with `ref H`
- [x] 3.10c `ref-option-parameter-free`: `Free(P)` of a `ref P Option<Own<TNode>>` (found while implementing)
- [x] 3.11 `self-owner-write`: `THolder.Poke(Self)`, `Self.FChild.FValue := 1`
- [x] 3.12 `link-from-ref-owner`: `Link(P)` with `ref P Own<TNode>`
- [x] 3.12b `link-from-ref-parameter`: `Link(X)` and `Link(X.FValue)` with a plain `ref X TPoint` (no owner)
- [x] 3.13 `link-from-ref-field`: `Link(H.FChild)` with `ref H`
- [x] 3.14 `link-from-self`: `Link(Self.FChild)` in a `Self` method
- [x] 3.15 `generic-ref-owner-write`: a generic routine with `ref B TBox<T>` writing through an owner field, instantiated

Positive (tests/runtime, `.inox` + `.out`):
- [x] 3.16 `read-only-through-owners`: `ref mut H` and `Self mut` write through nested owners; reads through `ref` and `Self`; a value owner parameter writes and links its object; a Link field reached from a `ref` parameter is written through (a Link is a write capability); `with` on a writable owner; a generic with `ref mut`

Regression:
- [x] 3.17 Every existing test and example unchanged (report any change)

## 4. Law and documents (after the results confirm the semantics)

- [x] 4.1 Constitution: CANON-10 Parameters and mutability (the ADR-0016 summary, receivers), CANON-25 access through pointers and Links; relock
- [x] 4.2 Specs `language/ref-parameters`, `language/structs`, `language/pointers`, edited in place (skip_specs)
- [x] 4.3 Manual (`docs/site/index.html`), CHANGELOG (Breaking), revision line, `kCanonRevision`

## 5. Gates (E19)

- [x] 5.1 IR snapshot compare (no difference), suite, ASan+UBSan with a 1 MiB stack, mutation fuzz seeds 1 and 7, backend_gaps 0 BUG, eh_state_balance, string_leak_check, grammar_consistency, openspec validate, constitution_check

Results (2026-10-31, unidentified cloud container, no .inox-machine):
- IR snapshot (baseline recorded on main before any change): 1021 existing
  programs, **0 differences**; 19 programs added (the new tests).
- Suite 1072/1072 (1053 before + 19 new), also under ASan+UBSan (g++) with
  a 1 MiB stack.
- Mutation fuzz: seed 1 OK=278 REJECTED=3722, seed 7 OK=292 REJECTED=3708,
  no crash, no timeout. Run on the same mutants, the compiler of main
  accepts 9 (seed 1) and 13 (seed 7) more; every one of those 22 is a
  mutant of a new D8 diagnostic test that keeps its violation. No mutant of
  an existing program changed its result.
- backend_gaps OK=54; eh_state_balance 2/2; string_leak_check 62/62;
  grammar_consistency 5/5; openspec validate 37/37; constitution_check 191 units, 0 problems.
- No existing test or example changed its result.
