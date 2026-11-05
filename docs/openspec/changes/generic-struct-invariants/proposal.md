## Why

Q3 of `generic-struct-methods`: invariants of generic structs were left to
a small change right after it. A generic struct with `invariant` lines is
rejected by the parser, so its instances cannot state the rule every value
keeps (CANON-24).

Approval: Marcelo Fortes, 2026-11-05 (Q3 of `generic-struct-methods`, and
the order of the 0.5 work of the same day). Risk: low; no new syntax, no
lowering change.

## What Changes

- A generic struct may have `invariant` lines. Contract lowering makes its
  `__invariant` a method of the generic struct (`TRange<T>.__invariant`),
  and each instance of the struct gets its instance of that method when it
  is made, so every check of CANON-24 finds it: after a construction,
  around a method, after a field written from outside.
- An invariant an instance cannot check is reported like any instance's
  error (rule 8 of `generic-struct-methods`).
- Reused parts in a generic struct, and type parameters of a method's own,
  stay rejected.

## Impact

- `parser/Parser.cpp` (the rejection), `semantic/ContractLowering.cpp`,
  `semantic/SemanticGenerics.cpp` (`instantiateInvariant`).
- Specs: `language/generics`; site, STATUS, CHANGELOG, a revision line.
- No program that compiled before changes.
