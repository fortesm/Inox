## ADR-0011 — `case` in the Ada/SPARK style  (Status: Accepted, 2026-10-09)

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 2021-2053) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12 (the ADRs become archived OpenSpec changes).

## Decision

Decision (approved by Marcelo Fortes, 2026-10-09):

    case Value
        1 | 2
            PutLn("small")
        3..9
            PutLn("medium")
        10 | 20..29 | 40 PutLn("mixed")
        otherwise
            PutLn("other")
    ;

- The selector is discrete: the Integer family, Char, Bool, Enum, Range. It is
  evaluated once.
- A choice is a static value (literal, Const, constant expression) or a static
  inclusive range `A..B` with A <= B, of the selector's type. `|` separates the
  alternatives of an arm; it is punctuation, not an operator.
- Overlap between any two choices, in one arm or in different arms, is a
  compile error. There is no fall-through.
- `otherwise` appears at most once and is the last arm. It is REQUIRED unless
  the compiler proves that the choices cover every value of the selector type
  (Bool, Enum and finite ranges directly; Integer types by the union of their
  choices). UInt64 and Natural are never provable with Int64 constants.
- Arms keep the Inox block rules: no `of`, `when`, `=>`, `:` or `do`; a
  single-line arm is allowed.
