## ADR-0016 — `ref` and `ref mut` parameters  (Status: Accepted, 2026-10-11)

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1903-1934) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12: "Os ADRs viram mudanças arquivadas no OpenSpec."

## Decision

Decision (Marcelo Fortes, 2026-10-11, approving the order "Array, then ref T /
ref mut T parameters, then Slice": "Ok Faça logo tudo na sua ordem!"). The
spelling follows the direction already recorded in the canon (`ref X T`,
`ref mut X T`); the rules are the safe subset agreed in the design discussion
with ChatGPT (many readers or one writer, references never outlive the call).
1. In a parameter list only: `ref X T` is a read-only reference to the
   caller's storage; `ref mut X T` is a writable one. A plain parameter stays
   an immutable copy; `mut X T` stays reserved (error). A method receiver is
   `Self`/`Self mut`, never `ref`.
2. A reference argument has exactly the parameter's type (no implicit
   conversion through a reference).
3. A `ref mut` argument is a writable place: a mutable local variable, a
   State variable, a `ref mut` parameter, or a field or array element of one.
   A constant, a literal, an expression or a value parameter is an error. A
   `ref` argument may be any value of the type (a temporary holds a value
   that is not a variable).
4. Exclusivity: the variable a `ref mut` argument is rooted at appears in no
   other argument of the same call (`Swap(X, X)`, `Swap(A[1], A[2])` and
   `Swap(P.X, P.Y)` are errors). A routine therefore never sees one variable
   through two names.
5. A reference exists only for the duration of the call: there are no
   reference variables, fields or results, so it cannot outlive what it
   refers to. Copying from it (`Y := X`) copies the value.
6. Writes through `ref mut` keep the type's checks (overflow, Natural range).
