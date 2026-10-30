## Shape

One folder holds the law and is the first thing anyone reads:

```
openspec/specs/constitution/
├── spec.md            the law (OpenSpec requirements; locked)
├── engineering.md     how the compiler is engineered (locked)
├── revisions.md       version, revision, open questions, revision log
└── constitution.lock  hash + approving change of each locked unit
```

- **Why OpenSpec format.** Every rule becomes a requirement with a scenario
  that names its test, so `openspec validate` and
  `tools/constitution_check.py` can check it; prose that cannot be tested
  is kept to the Purpose and "How to read" sections. OpenSpec in strict
  mode limits a requirement to 500 characters and one behavior, so a canon
  section becomes several requirements (118 in all), each starting with its
  anchor ("CANON-5 Const", "CANON-5 State", ...). The three tables (types,
  exception taxonomy, precedence) live in a "Reference tables" section
  before the requirements, locked with the preamble;
  `tools/grammar_consistency.py` reads the precedence table from there.
- **Why the CANON-N anchors stay.** Hundreds of code comments, tests and
  diagnostics cite CANON-2 … CANON-21. Keeping the anchors at the start of
  requirement names keeps all of them valid. The second CANON-20 (LLVM backend) becomes
  CANON-23; SECTION 17 (loops) and SECTION 14 (scoping) join CANON-12 and
  CANON-10; the four `SECTION NN` comments in code are rewritten.
- **Why a separate engineering.md.** It is constitutional (G6) but is about
  C++ code, not Inox programs, which OpenSpec requirements describe
  (config rule).
- **Why revisions.md is not locked.** The revision number moves with every
  approved change, including changes that touch only capability specs;
  locking it would force a relock of the law for each of them. It is
  append-only by rule (G3) and its header is checked against
  `inox --version` by the suite.
- **The lock.** Units: the text before `## Requirements` and each
  requirement of spec.md; the text before the first `##` and each `##`
  section of engineering.md. The lock line records the change that
  approved the text; `check` accepts it only if that change exists (active
  or archived) and its proposal quotes Marcelo Fortes. This is a guard
  against accidental and unapproved edits, not a proof of approval: the
  review of the pull request remains the maintainer's.
- **Capability specs.** They were "mirrors until approved" (G7 of the canon).
  With the canon retired into the constitution by the maintainer's
  decision, they are law under it; they detail, and may never contradict,
  the constitution. They are not locked: they change through ordinary
  approved OpenSpec changes, as before.

## What was removed or corrected (the "higienização")

Duplicates (kept once): AGENT RULES (twice), DEV-2 (twice), GROUP FUTURE
(twice), B-WORKS, B-PARSED, B-GAPS, B-EXAMPLES (twice each), the
numeric-semantics summary (repeated CANON-8), SECTION 13/14 (repeated
CANON-5/CANON-10), the language rules repeated in AGENTS.md.

Stale status removed from the law and rewritten from the code in
`docs/STATUS.md`: B-WORKS ("Types registered (31)", "not String yet"),
B-GAPS (String locals and State listed as gaps, closed in v3.55; explicit
conversions listed as a gap, done for integers in v3.44), B-CONFLICTS (all
resolved), E12 and SECTION 29 (EH-v3.16a open; closed in v3.41), the v3.17
lowering note, the temporary console ABI.

Superseded text replaced by the later decision:
- CANON-17 "ReadLn returning String … DEFERRED" — file `ReadLn` exists
  (r3.59).
- CANON-12 "unless: lowering incremental" — lowered since v3.42.
- CANON-13 "Set literal planned, may be deferred" — v3.50 decided the
  literal in a Set context, functions returning new sets, 256 values.
- CANON-13 Enum/Range — v3.50/v3.51 decisions merged (implicit Range check,
  `TEnum(I)` checked, `Low`/`High`/`Succ`/`Pred`, enum printing by name).
- CANON-8 — v3.50 UInt64 literal rules merged.
- FUTURE-7 "explicit type conversions; multi-variable declaration; ReadLn"
  — implemented (v3.44, v3.26, r3.59).
- Agent rule 15 "No `mut` parameters" — kept, with `ref`/`ref mut` (ADR-0016).
- Agent rule 19 "keep `dist/` examples in sync" — there is no `dist/`
  directory; packages are built from `examples/`.
- Module architecture directive "big reorganizations deferred; an AI must
  not refactor" — superseded by the approved modularization (change
  `compiler-modularization`); E21 now says splits go through an approved
  OpenSpec change and `ir_snapshot`.
- NON-NEGOTIABLE "this file is the source" and the footer "END OF
  INOX_CANONICAL.md v3.8" — replaced by the constitution's reading order.
- DEV-2 "bundled documentation must be a copy of THIS file; regenerate from
  INOX_CANONICAL.md" — packages ship the manual and the constitution.
- AGENTS.md "`Var` has no `:` and closes with `;`", "Local variables declared
  inline or in `Var`", `Vector[T]`, `Set[T]` (the copy in some clones) —
  gone with the duplicated rules.
- The canon's two governance notes on "B-PROPOSALS" — proposals are OpenSpec
  changes marked "Awaiting approval" (G2).

Nothing of Layer A was dropped: every rule is in `spec.md`,
`engineering.md` or a capability spec, and `docs/history/CANON_MAP.md`
names the destination of each of the 268 units of the last canon.

## Questions for the maintainer (not decided here; revisions.md)

1. `State :` uses `:` although CANON-4 gives `:` one meaning.
2. The roadmap's 0.1.x–0.9.x milestones predate the r3.60 version scheme.
3. Driver subcommands (`inox run/build/check`) vs today's switches.
4. Exception bindings and the unread-local rule.
5. What `Std.Debug.Assert` does when it fails.
6. Open items of the change `std-strings`.

## Verification

- `openspec validate --all --strict`; `tools/constitution_check.py check`
  (every unit locked, every named test exists).
- The suite on Linux (both runners read the version from revisions.md).
- `tools/grammar_consistency.py` reads the CANON-20 table from the
  constitution.
- Before deleting the canon, a final run of `tools/canon_trace.py` showed
  268 units and every one with a final destination (no unit `planned`).
