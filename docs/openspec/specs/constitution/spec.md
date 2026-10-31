# constitution Specification

## Purpose
The constitution of Inox: the language law and the rules that govern it. It
is the minimized, consolidated successor of `docs/INOX_CANONICAL.md`, which
was retired at canon revision r3.61 (decided by Marcelo Fortes, 2026-10-12: the canon survives, minimized and
cleaned, as this constitution). Its purpose is that nobody — human or AI,
with or without the history of a conversation — has to be told again what
Inox has already decided.

## How to read this constitution
Inox is described in three layers that are never confused:

- **Law** — what Inox is. This file, `engineering.md` next to it, and the
  capability specs in `docs/openspec/specs/language/*` and
  `docs/openspec/specs/stdlib/*`, which detail a topic of this file (a capability spec may add detail, never contradict this file).
  The law changes only by the maintainer's dated approval (G2).
- **Status** — what the compiler does today: `docs/STATUS.md`,
  `docs/BACKEND_GAPS.md` (generated) and `CHANGELOG.md`. Status follows the
  code. When status and law differ, the difference is a conformance gap,
  never a reason to edit the law.
- **History** — why: `revisions.md` (the revision log), `docs/history/`
  (the change log of the retired canon, v1.0 to r3.60) and
  `docs/openspec/changes/archive/` (every decided change and ADR-0001..0017,
  verbatim, with the maintainer's words).

Reading order for anyone without context: this file, `engineering.md`,
`revisions.md` (current version, revision, open questions), then the
capability spec of the topic at hand, then `AGENTS.md` (how to work in this
repository).

Each requirement starts with a stable anchor (G1–G8, CANON-1 to CANON-25,
VERSIONING, FUTURE) that code comments, diagnostics and tests cite; one
anchor may span several requirements. Each scenario names the test that
proves it, or says "test to add" (a recorded gap). Everything in this file
and in `engineering.md`, the tables below included, is locked by
`constitution.lock` (G8).

## The idea of the syntax: write less, do more
Inox removes the verbosity of Ada and Pascal and the cryptic symbols of C,
C++ and Rust (Marcelo Fortes, 2026-10-12: Inox is about writing less and doing more). A Pascal routine needs `procedure`, `begin` and `end`; Ada
needs `is` and `end`; C needs `{` and `}`. Inox keeps only what carries
meaning:

```text
Procedure Sum(A : Integer; B : Integer) : Integer;   Pascal
Begin
End;

Sum(A Integer, B Integer) Integer :                   Inox
    Return A + B
;
```

The `:` does two jobs at once: after a name and a signature it tells the
compiler that this is a subroutine, and it opens the body, like `begin` or
`{`. Nothing else needs it:
control structures and `State` open their body at the line break, `Type`
and `Const` are declarators, and `;` closes every block. A `:` without
a name and a signature before it is meaningless, so a bare `:` block does
not exist. New syntax is judged by the same balance: elegant, ergonomic,
short, never cryptic and never verbose.

## Reference tables

### CANON-8 built-in types

| Type | Representation | Notes |
|------|----------------|-------|
| `Bool` | i1 | `false < true`; `Boolean` is not Inox |
| `Integer` = `Int64` | i64 | the default integer, target of integer inference |
| `Natural` | i64 | subtype of Integer, 0..Int64.Max (ADR-0014, ADR-0015) |
| `Byte` = `UInt8` | i8 | 0..255 |
| `Int8` `Int16` `Int32` `Int64` | i8 to i64 | signed fixed widths |
| `UInt8` `UInt16` `UInt32` `UInt64`, `UInteger` = `UInt64` | i8 to i64 | unsigned fixed widths |
| `Float` = `Float64`, `Float32` | double, float | the default real is Float64 |
| `Currency` | i64 scaled 10^6 | exact decimal, 6 places |
| `Crypto` | i128 scaled 10^18 | exact decimal, 18 places |
| `Char` | i32 | Unicode scalar value |
| `String` | reference | UTF-8, immutable, never null, default `""` |

### CANON-15 standard exception taxonomy

```text
Exception
    IOError
        FileNotFound
        PermissionDenied
        AlreadyExists
        DiskFull
    ConversionError
    ArithmeticError
        DivisionByZero
        OverflowError
        DomainError
    RangeError
        IndexError
```

### CANON-20 precedence table

```text
Level  Operators                              Associativity
-----  -------------------------------------  -------------
 1     postfix: call f(...), index A[i]       left
 2     ^  (exponentiation)                    RIGHT
 3     unary: not  -x  +x  bitnot             (prefix, unary)
 4     *  /  mod  rem                         left
 5     +  -                                   left
 6     shl  shr  (bit shifts)                 left
 7     bitand                                 left
 8     bitxor                                 left
 9     bitor                                  left
10     ..  (range construction)               NON-associative
11     in  (membership)                       NON-associative
12     = # < <= > >=  (relational)            left
13     and                                    left
14     xor                                    left
15     or                                     left
16     :=  (assignment, statement level)      right
```

## Requirements

### Requirement: G1 The law wins on design, the code on status
The law SHALL win on language design and the code SHALL win on current
status. Code that disagrees with the law has a bug: it is fixed, or recorded
as a conformance gap in `docs/STATUS.md`. The law is never edited to match
the code.

#### Scenario: A gap is recorded, not legislated
- **WHEN** the compiler does not yet implement a rule of the law
- **THEN** the gap is listed in docs/STATUS.md and the rule stays as it is (review)

### Requirement: G1 Disagreements are reported, never resolved silently
A disagreement between two sources SHALL be reported and fixed explicitly;
nobody invents a third behavior or silently picks a side. Authority, from
most to least: this file and `engineering.md`; the capability specs;
`docs/grammar/grammar.ebnf` (a mirror); the manual `docs/site/index.html`;
examples and tests (evidence of what is implemented). `AGENTS.md` holds no
language rules.

#### Scenario: The grammar mirrors the law
- **WHEN** the precedence table of CANON-20 and the level annotations of `docs/grammar/grammar.ebnf` differ
- **THEN** the suite fails (tools/grammar_consistency.py, run by tools/scripts/run-tests.sh)

### Requirement: G2 Authority
Only Marcelo Fortes SHALL approve a change to the law. An agent may propose a
change, in an OpenSpec change marked "Awaiting approval", and never applies a
proposal as fact. Open questions stay open until he answers them; an agent
implements only the approved parts.

#### Scenario: Approval is recorded
- **WHEN** an OpenSpec change modifies the law
- **THEN** its proposal records Marcelo Fortes's approval with its date and states the decision, or says "Awaiting approval" and no code implements it (docs/openspec/config.yaml rules; review)

### Requirement: G3 Every decision is recorded
Every approved change to the law SHALL be recorded in the OpenSpec change
(archived after the merge), in a revision line of `revisions.md`, in the
specs, and in `CHANGELOG.md` when the compiler changes. A record states the
technical decision in English, who approved it and when; it never
transcribes a conversation. Records are append-only. If the code
contradicts a recent decision, the code is behind; the decision is
intentional.

#### Scenario: Revision and version agree
- **WHEN** `inox --version` runs
- **THEN** it prints the version and revision at the top of revisions.md, with any BUILD (tools/scripts/run-tests.sh and tools/scripts/run-tests.ps1, "inox --version matches the constitution")

### Requirement: G4 No silent scope expansion
A behavior that the law does not cover SHALL NOT be inferred from another
language (C, C++, Rust, Ada, Pascal, Python, Go, Java, Visual Basic or any
other). The work stops and a language decision is requested; once approved,
it is recorded (G3).

#### Scenario: An uncovered question
- **WHEN** an implementation needs a rule the law does not state
- **THEN** the question goes to the maintainer and is listed under "Open questions" in revisions.md until answered (review)

### Requirement: G5 Verifiability
Every rule of the law SHALL be backed by a test named in a scenario, or be
listed as "test to add". That the code obeys the law is proven by running
the suite and the gates of `engineering.md` E19, never asserted; a green
suite alone never proves the absence of regressions.

#### Scenario: Named tests exist
- **WHEN** a spec under docs/openspec/specs names a test file that does not exist
- **THEN** CI fails (tools/constitution_check.py check)

### Requirement: G6 Engineering quality is constitutional
The compiler, its runtime and its tools SHALL be engineered as
`engineering.md` requires. It binds every new change and every code area a
change touches; existing code improves incrementally, never by untested
cosmetic rewrites.

#### Scenario: Gates of a change
- **WHEN** a change to the compiler is reported done
- **THEN** its tasks end with the E19 gates and the report says which ran (docs/openspec/config.yaml rules)

### Requirement: G7 OpenSpec carries every change
Every change to the language, compiler, runtime or standard library SHALL
start as an OpenSpec change in `docs/openspec/changes/<name>/` (proposal,
requirement deltas with test-named scenarios, design, tasks) and be archived
after its pull request is merged, so `docs/openspec/specs` matches `main`.

#### Scenario: Specs are valid
- **WHEN** a pull request is opened
- **THEN** `openspec validate --all --strict` passes in CI (.github/workflows/ci.yml)

### Requirement: G7 High-risk changes wait for design approval
A change whose proposal says `Risk: high` — it touches exception lowering,
the runtime ABI or memory ownership, or rewrites a compiler phase — SHALL
wait for the maintainer's dated approval of its design before any code is
written.

#### Scenario: Design approval
- **WHEN** a high-risk change is implemented
- **THEN** its design.md ends with "Design approval:", the maintainer's name and the date (docs/openspec/config.yaml rules)

### Requirement: G8 The lock
This file and `engineering.md` SHALL be protected by `constitution.lock`:
the hash of each unit and the OpenSpec change that approved its text. Text
that differs from its hash fails CI. Locked text changes only through
`python3 tools/constitution_check.py relock --approval <change>`, where the
change exists and records Marcelo Fortes's approval.

#### Scenario: An unapproved edit
- **WHEN** a locked requirement is edited, added or removed without a relock through an approved change
- **THEN** CI fails (tools/constitution_check.py check)

### Requirement: CANON-1 Identity of Inox
Inox SHALL be a compiled, strongly and nominally typed systems language for
software where silent failure is unacceptable: aviation, finance and
exchanges, monetary and crypto-asset systems, science, aerospace, medicine,
energy, cryptography, parallel computation. When explicit safety and
convenience conflict, safety wins. It learns from many languages and copies
none of their mistakes.

#### Scenario: Silent failure is refused
- **WHEN** a value would overflow, be divided by zero or index outside its array
- **THEN** the program is rejected or traps, never continues with a wrong value (tests/diagnostics/const-add-overflow.inox, tests/runtime/add-overflow.inox, tests/runtime/array-index-out-of-bounds.inox)

### Requirement: CANON-1 Post-object-oriented design
Inox SHALL have no classes, classical inheritance, Java-style interfaces,
mixins, duck typing, class taxonomies or OO visibility model: visibility
belongs to the module (CANON-3), never to a class. `Struct` is
data; free functions, subroutines and associated methods declared outside
the struct are behavior; `P.Method(args)` is sugar for a static call;
composition replaces inheritance: a struct reuses other structs' fields and
methods (CANON-9) and never becomes them.

#### Scenario: Behavior outside the data
- **WHEN** a struct gets methods declared as `TPoint.Move(Self mut, ...)` outside its `Struct` block
- **THEN** `P.Move(1, 2)` is a static call that mutates P (examples/associated-methods.inox, tests/semantic/valid/method-and-struct-value.inox)

### Requirement: CANON-1 Safety defaults
Inox SHALL have no universal `null` or `nil`, no unsafe pointers in the safe
core, no silent integer overflow, no unchecked integer division, no unchecked indexing, no
implicit narrowing and no implicit aliasing; parameters are immutable by
default and a receiver is mutated only through `Self mut`. The core needs no
tracing garbage collector: memory follows ownership, moves and deterministic
release.

#### Scenario: Every variable has a value
- **WHEN** a program declares a scalar without a value
- **THEN** it is rejected (tests/diagnostics/scalar-without-initializer.inox)

### Requirement: CANON-2 Names are case-insensitive
Keywords and identifiers SHALL be case-insensitive (`PutLn`, `putln` and
`PUTLN` are one name); names that differ only by case are the same name and
conflict. Documentation uses one canonical spelling.

#### Scenario: Case-only difference
- **WHEN** a scope declares `Amount` and then `amount`
- **THEN** it is a redeclaration error (tests/semantic/invalid/case-insensitive-redeclaration.inox)

### Requirement: CANON-2 Comments and literals
A line comment SHALL start with `==` and run to the end of the line; there
are no block comments. Strings use double quotes, Chars single quotes
(`'a'`, `'é'`, `'😀'`). Integers are decimal `42` or hexadecimal `$2A` or
`0x2A` (the same literal); reals are `3.14` or, with an exponent after the
fraction, `1.5e3` and `2.0E-7` (r3.89; `1e3` is an error). A leading zero
never means octal: `017` is seventeen. An integer literal must fit Int64
(UInt64: see CANON-8), a real literal Float64.

#### Scenario: Literals
- **WHEN** a program uses `$FF`, `0xFF` and `017`
- **THEN** they are 255, 255 and 17 (tests/runtime/hex-literals.inox); a literal beyond Int64 is rejected (tests/diagnostics/literal-out-of-range.inox)

#### Scenario: Exponents
- **WHEN** a program writes `1.5e3`, `2.0E-7`, `1e3` and `1.0e400`
- **THEN** the first two are reals (tests/runtime/real-exponent.inox); `1e3` and a literal beyond Float64 are rejected (tests/diagnostics/real-exponent-without-fraction.inox, tests/diagnostics/real-literal-out-of-range.inox)

#### Scenario: Unterminated string
- **WHEN** a string literal is not closed
- **THEN** it is a lexical error (tests/lexer/invalid/unterminated-string.inox)

### Requirement: CANON-2 Digit separator
The underscore SHALL separate digits of any numeric literal for readability
(`10_000_000`, `3.141_592`, `$FF_FF`) and only between two digits: at the
start, at the end or doubled it is an error.

#### Scenario: Separators
- **WHEN** a program writes `1_000` and `1__0` or `1_`
- **THEN** the first is 1000 (tests/runtime/digit-separators.inox) and the others are errors (tests/diagnostics/digit-separator-doubled.inox, tests/diagnostics/digit-separator-trailing.inox)

### Requirement: CANON-2 Symbols
The symbols SHALL be: `:=` declaration and assignment; `=` equality, `#`
and `!=` inequality (one operator, two spellings; a lone `!` is not an
operator, negation is `not`, r3.87); `<` `<=` `>` `>=`; `..` range; `:` routine declaration
(CANON-4); `;` block close; `.` member; `,` separator; `|` between `case`
alternatives; `^` power, never XOR; `+=` `-=` `*=` `/=` `^=` compound
assignment. `{` and `}` are reserved until generics and concurrency show
their use (Marcelo Fortes, 2026-10-16).

#### Scenario: Tokens
- **WHEN** the lexer reads keywords, literals and symbols
- **THEN** it produces the expected tokens (tests/lexer/valid/tokens-keywords-literals.inox)

#### Scenario: Inequality spelled != and a lone !
- **WHEN** a program compares with `!=` and `#`, or writes `if ! (A = B)`
- **THEN** both spellings compare alike (tests/runtime/not-equal-operator.inox) and `!` is rejected, pointing to `not` (tests/diagnostics/bang-negation.inox)

### Requirement: CANON-2 Reserved words and predefined names
The reserved words SHALL be: Use Type Const State Struct Enum if elif else
unless while repeat until for in step case try except On do ensure raise
retry with mut ref mod rem and xor or not shr shl Return pre pos old
invariant variant Override Public. Predefined names, never declared but free as
field and method names: True False Exit leave continue IIf Fill bitand
bitor bitxor bitnot. `Main`, `Var` and `Module` are ordinary names (2026-10-16).

#### Scenario: Predefined names
- **WHEN** a program declares True, Exit, BitAnd, False or Leave, or names a field Exit
- **THEN** the declarations are rejected (tests/diagnostics/predefined-true-declared.inox, tests/diagnostics/predefined-exit-assigned.inox, tests/diagnostics/predefined-bitand-parameter.inox, tests/diagnostics/predefined-false-routine.inox, tests/diagnostics/predefined-leave-declared.inox, tests/diagnostics/fill-predefined.inox) and the field works (tests/runtime/predefined-names.inox)

#### Scenario: Reserved words
- **WHEN** a program declares a variable named Old or Do
- **THEN** it is rejected (tests/diagnostics/old-reserved.inox, tests/diagnostics/do-reserved.inox)

### Requirement: CANON-2 Contextual words
`Export` SHALL be a word only in `Export Use M`, and `as` only in `Use M as
A` (r3.99); elsewhere both are ordinary names.

#### Scenario: Export and As as names
- **WHEN** a program names a variable Export and a parameter As
- **THEN** both work (tests/runtime/contextual-words.inox)

### Requirement: CANON-2 Words Inox does not have
`End`/`end` SHALL NOT be a keyword and never closes a block. `break` and
`finally` are ordinary identifiers: the loop exit is `leave` and the try
cleanup clause is `ensure` (ADR-0007). `otherwise` is not a word: the last
arm of a `case` is `else` (r3.74).

#### Scenario: Braces reserved
- **WHEN** a program writes `{` or `}`
- **THEN** it is rejected as reserved (tests/diagnostics/braces-reserved.inox)

#### Scenario: Former keywords
- **WHEN** a program uses `break` or `finally` as names, or closes a block with `end`
- **THEN** the names are valid (tests/semantic/valid/former-keywords-as-identifiers.inox) and `end` is rejected (tests/semantic/invalid/end-block-close.inox), as is a `case` arm `otherwise` (tests/diagnostics/case-otherwise-removed.inox)

### Requirement: CANON-3 Modules and Use
A source file SHALL be one logical module named after its file, with no
header (Marcelo Fortes, 2026-10-16): `Calc.Core.inox` or `Calc/Core.inox`
is the module `Calc.Core`; the entry file may have any name. `Use Name`
declares a semantic dependency: the compiler loads that module's public
types and signatures; it is never textual inclusion or manual linking.

#### Scenario: Multi-file program
- **WHEN** `Main` uses `Math.Basic` from its own directory
- **THEN** it compiles and runs (tests/integration/modules/Main.inox)

#### Scenario: No Module header
- **WHEN** a file starts with `Module Name`
- **THEN** it is rejected (tests/diagnostics/module-header-removed.inox)

### Requirement: CANON-3 Use is not transitive
A module SHALL see only the modules it uses itself. `Export Use M` passes
M's public declarations on to the modules that use it, without changing
what is public in M; `Export` exists only in that form (Marcelo Fortes,
2026-10-26).

#### Scenario: Use is not transitive; Export Use passes a module on
- **WHEN** Main uses a module that uses another, or one that re-exports another with `Export Use`
- **THEN** the first's dependency is not visible to Main (tests/modules/use-not-transitive/Main.inox, tests/modules/library-not-transitive/Main.inox) and the re-exported one is (tests/modules/export-use/Main.inox, tests/modules/std-pair-reexports/Main.inox)

### Requirement: CANON-3 Names of a module
Each module SHALL have its own names (Marcelo Fortes, 2026-10-29). A name
written without qualification is the module's own declaration first, then
the one public declaration of that name among the modules it uses; two of
them is an ambiguity error, never a choice of the compiler. `M.Name`
(`Bank.Accounts.Fee`) reaches a public declaration of a module the file
uses or that one re-exports to it; a method is still called on its value;
no variable takes the name of a used module or alias.

#### Scenario: Own names first, ambiguity is an error
- **WHEN** a module declares a name a used module also has, or uses two modules with the same public name
- **THEN** its own wins (tests/modules/own-name-first/Main.inox, tests/modules/same-private-names/Main.inox) and the unqualified use is rejected, naming the qualified forms (tests/modules/ambiguous-import/Main.inox)

#### Scenario: Qualified names
- **WHEN** a program writes `Bank.Accounts.Fee(10)`, a qualified type, a private name qualified, or a variable named like a used module
- **THEN** the first two work (tests/modules/qualified-names/Main.inox) and the others are rejected (tests/modules/private-qualified/Main.inox, tests/modules/variable-named-module/Main.inox)

### Requirement: CANON-3 Aliases and selective use
`Use Bank.Accounts as Accounts` SHALL name the module `Accounts` in this
file only, and then only the alias qualifies; `Export Use ... as` is an
error. `Use Bank.Accounts.(Deposit, Withdraw)` takes only the listed public
names unqualified, with parentheses even for one name and the list before
`as`; a name the module declares itself cannot be listed, and the others
stay reachable qualified (Marcelo Fortes, 2026-10-29).

#### Scenario: Aliases
- **WHEN** a program uses two modules through aliases, writes the full name after an alias, writes `Export Use ... as`, or names a variable like an alias
- **THEN** the first works (tests/modules/alias-qualified/Main.inox) and the others are rejected (tests/modules/alias-hides-full-name/Main.inox, tests/modules/export-use-alias/Main.inox, tests/modules/variable-named-alias/Main.inox)

#### Scenario: Selective use
- **WHEN** a program lists names in `Use M.(...)`, with and without `as`, reads a public name it did not list, or lists a private, unknown or own name, or nothing
- **THEN** the listed names work and the rest stay qualified (tests/modules/selective-import/Main.inox, tests/modules/selective-with-alias/Main.inox), and the others are rejected (tests/modules/selective-limits/Main.inox, tests/modules/selective-private/Main.inox, tests/modules/selective-unknown/Main.inox, tests/modules/selective-own-name/Main.inox, tests/modules/selective-empty/Main.inox)

### Requirement: CANON-3 Module resolution
`Use A.B` SHALL look for `A.B.inox`, then `A/B.inox`, in the directory of
the entry file, then in each `--path` directory in order (an error when
one is not a directory), then in each `INOX_PATH` entry (`;` separates them
on Windows, `:` elsewhere; a missing entry is skipped), then in the
standard-library directory (CANON-19); the first file found is the module
(Marcelo Fortes, 2026-10-29). Dependencies load recursively; a cycle is a
compile error.

#### Scenario: Cycle
- **WHEN** two modules use each other
- **THEN** compilation fails (tests/integration/cycles/Cycle.A.inox)

#### Scenario: Search order
- **WHEN** a module is found through `--path` and INOX_PATH, through INOX_PATH alone, or `--path` names a missing directory
- **THEN** `--path` wins over INOX_PATH, a missing INOX_PATH entry is skipped, and the missing `--path` is an error (tools/scripts/run-tests.sh, tools/scripts/run-tests.ps1: "D1: --path before INOX_PATH, missing entry skipped", "D1: INOX_PATH alone", "D1: --path to a missing directory")

### Requirement: CANON-3 The Std name
`Std` SHALL name the standard library (Marcelo Fortes, 2026-10-29): `Use
Std.X` looks only there, and a module named `Std` or `Std.*` anywhere else
is an error, never a replacement.

#### Scenario: Std is reserved
- **WHEN** a Std.Math file stands beside a program that uses Std.Math, or a program file is named Std.Thing
- **THEN** both are rejected (tests/modules/std-reserved/Main.inox; tools/scripts/run-tests.sh, tools/scripts/run-tests.ps1: "D2: a program file named Std.*")

### Requirement: CANON-3 Visibility
Every declaration of a module SHALL be private unless it is `Public`
(Marcelo Fortes, 2026-10-26). `Public` is the last word of a header: of a
type (`TPerson Struct(TBeing) Public`), of a routine or method, after its
result type and `Override`, just before `:`
(`TPerson.UpdateName(Self mut, Name String) String Public :`), and of a
Const (`Const MaxRate Public := 0.05`). A State variable is never public:
other modules reach it through the module's public routines.

#### Scenario: Private by default
- **WHEN** a module calls a private routine of a module it uses, or the two declare the same private name
- **THEN** the call is rejected (tests/modules/private-routine/Main.inox) and the names do not collide (tests/modules/same-private-names/Main.inox)

#### Scenario: A name alone on its header line
- **WHEN** a type's name stands alone on its line and its header follows on the next
- **THEN** it declares the same type (tests/runtime/type-header-line-break.inox)

### Requirement: CANON-3 Nothing public exposes something private
A method SHALL be declared in the module of its type. A public routine does
not name a private type, a private type has no public method or field, and
a public struct does not reuse a private one (r3.99, as in Ada and Rust).

#### Scenario: Exposing something private
- **WHEN** a public routine names a private type, a private type has a public method or field, a public struct reuses a private one, or a method is declared outside its type's module
- **THEN** each is rejected (tests/modules/public-exposes-private-type/Main.inox, tests/modules/public-method-private-type/Main.inox, tests/modules/public-fields-private-type/Main.inox, tests/modules/public-reuses-private/Main.inox, tests/modules/method-in-other-module/Main.inox)

### Requirement: CANON-3 Struct encapsulation
A struct's fields SHALL be private to its module unless they follow its
`Public` line, after the private ones (Marcelo Fortes, 2026-10-29). A
private field or method is read, written, constructed or called only in
its type's module. A struct with a private field without default is built
only by its module (`TName(...)`, a bare declaration, `New`). Reuse across
modules (CANON-9) offers only the part's public members. Read-only fields
come later.

#### Scenario: The balance of an account
- **WHEN** another module changes an account only through its public methods, or reads, writes, constructs or calls a private member
- **THEN** the first runs (tests/modules/encapsulated-account/Main.inox, examples/bank/Main.inox) and the others are rejected (tests/modules/private-field-read/Main.inox, tests/modules/private-field-write/Main.inox, tests/modules/private-field-construction/Main.inox, tests/modules/private-field-with/Main.inox, tests/modules/private-method/Main.inox)

#### Scenario: Built only by its module
- **WHEN** another module declares or constructs a struct with a private field without default, or gets it from the module's routine
- **THEN** the first two are rejected (tests/modules/built-only-in-module/Main.inox, tests/modules/built-only-in-module-construction/Main.inox) and the routine works (tests/modules/built-by-module-routine/Main.inox)

#### Scenario: Reuse and invariants across modules
- **WHEN** a struct reuses a part from another module, or another module breaks an invariant through a public field
- **THEN** only the part's public members are offered (tests/modules/reuse-sees-public/Main.inox, tests/modules/reuse-private-field/Main.inox) and the invariant traps (tests/modules/invariant-after-public-write/Main.inox)

### Requirement: CANON-4 Semicolon closes, colon declares
`;` SHALL close blocks and is not a statement terminator. `:` has one
meaning, after a routine's name and signature: it declares the function or
subroutine and opens its body (the word other languages spell `procedure`,
`begin` or `{`). No other construct uses `:` (see "The idea of the syntax").

#### Scenario: Routine declaration
- **WHEN** a routine is declared `Sum(A Integer, B Integer) Integer :` … `;`
- **THEN** it compiles (examples/functions.inox)

### Requirement: CANON-4 Control structures use no colon
`if`/`elif`/`else`, `unless`, `while`, `for`, `repeat`, `case`, `with` and
`try` SHALL open their body at the line break after the header and close
with `;`; none uses `:`. A bare `:` block inside a routine, at any depth, is
illegal: Inox has no anonymous nested blocks.

#### Scenario: Colon after a header
- **WHEN** a `while`, `for`, `if` or `Type` header ends with `:`
- **THEN** it is a parse error (tests/parser/invalid/while-colon.inox, tests/parser/invalid/for-colon.inox, tests/invalid/invalid-026.inox, tests/parser/invalid/type-colon.inox)

#### Scenario: Bare colon block
- **WHEN** a routine body contains `:` … `;`
- **THEN** it is rejected (tests/diagnostics/colon-block-in-routine.inox, tests/diagnostics/colon-block-in-loop.inox)

### Requirement: CANON-4 One statement per line
A simple statement SHALL end at a line break: two statements on one line, or
a `;` followed by anything but a line break, are errors. A bare name or
literal is not a statement. A `(`, `[` or `.` that begins a new line does
not continue the expression of the previous line.

#### Scenario: Two statements on a line
- **WHEN** a line holds two calls, `X := 1 ; Y := 2`, or a bare name
- **THEN** it is a parse error (tests/diagnostics/statement-two-calls-one-line.inox, tests/diagnostics/statement-semicolon-mid-line.inox, tests/diagnostics/statement-bare-identifier.inox)

#### Scenario: Parenthesis on the next line
- **WHEN** a line starts with `(`
- **THEN** it is not a call on the previous line (tests/diagnostics/call-paren-on-next-line.inox)

### Requirement: CANON-4 Sections and removed forms
`Type` SHALL have no `:` and no closing `;`, ending where the next module
item starts; in it `Struct` and multi-line `Enum` close with `;`, `Range`
and short `Enum` are single lines. `State` has no `:` and closes with `;`.
`Const Name := Value` is one line. `State :` and `Name : Type` are rejected
with a message showing the right form.

#### Scenario: Sections without colon
- **WHEN** a State section is written `State :`, misses its `;`, or declares `Name : Type`, or a Const declares `Const Name : Type`
- **THEN** it is rejected (tests/diagnostics/state-with-colon.inox, tests/diagnostics/state-without-semicolon.inox, tests/diagnostics/state-legacy-colon.inox, tests/diagnostics/const-legacy-colon.inox)

### Requirement: CANON-5 Three declaration forms
A local variable SHALL be declared inline, anywhere in a block, as `A := 5`
(inferred type), `B Integer := 6` (written type) or `P TPoint` (a struct or
aggregate at its type defaults). The first appearance of a name declares it
and later ones assign it. A bare identifier is never a declaration.

#### Scenario: Inline declarations
- **WHEN** a block declares locals in the three forms between statements
- **THEN** they compile (tests/semantic/valid/grouped-local-declarations.inox, tests/semantic/valid/local-block-scope-and-assignment.inox); a bare identifier is rejected (tests/diagnostics/statement-bare-identifier.inox)

### Requirement: CANON-5 Every variable is born with a value
A scalar declaration SHALL have an initializer: `C Integer` is "scalar
declaration requires initializer: C", for locals, Strings and State alike.
An enum variable needs an initializer (strict Ada rule). Every array,
whatever its element type, SHALL be initialized where it is declared,
never with an implicit zero, False or first enum literal (r3.81; `Fill`,
CANON-13). Sets, and structs whose type defaults are complete, may omit
`:=` and take their type defaults.

#### Scenario: Scalars need a value
- **WHEN** a local, String or State scalar has no initializer
- **THEN** it is rejected (tests/diagnostics/scalar-without-initializer.inox, tests/diagnostics/string-without-initializer.inox, tests/diagnostics/state-scalar-without-initializer.inox)

#### Scenario: Struct defaults
- **WHEN** a struct is declared without `:=`
- **THEN** it holds its field defaults (tests/runtime/struct-defaults.inox)

#### Scenario: Enum initializer
- **WHEN** an enum variable is declared without `:=`
- **THEN** it is rejected (tests/diagnostics/enum-without-initializer.inox)

#### Scenario: Arrays are filled
- **WHEN** a local or State array is declared without `:=`, or with `Fill(Value)`
- **THEN** the first is rejected (tests/diagnostics/array-without-initializer.inox, tests/diagnostics/enum-array-without-initializer.inox, tests/diagnostics/state-array-without-initializer.inox) and the second holds Value in every element (tests/runtime/array-fill.inox)

### Requirement: CANON-5 Initialization is transitive
Explicit initialization SHALL hold through structs, arrays and type
parameters (r3.82). A struct field of an enum, Char, array or type-parameter type
has no implicit value: it has a default (a literal, or `Fill(Literal)` for
an array) or is given in the construction. A struct takes its type defaults
only when every component has one, nested structs and parts included; no
instance of a generic struct brings a default its declaration does not
guarantee.

#### Scenario: Required fields
- **WHEN** a struct whose array, enum or type-parameter field has no default is declared without `:=`, directly, nested, as a part, in State, by `New(TName)` or as an instance `TBox<Integer>`
- **THEN** it is rejected (tests/diagnostics/struct-array-field-required.inox, tests/diagnostics/enum-field-without-default.inox, tests/diagnostics/nested-required-field.inox, tests/diagnostics/part-required-field.inox, tests/diagnostics/state-required-field.inox, tests/diagnostics/new-required-field.inox, tests/diagnostics/generic-field-required.inox, tests/diagnostics/generic-field-required-enum.inox)

#### Scenario: Required fields given
- **WHEN** such fields have defaults, or are given in the construction, also nested and in generic instances
- **THEN** the values hold them (tests/runtime/required-fields.inox)

### Requirement: CANON-5 Unread locals are errors
A local that is never read SHALL be a compile error ("local variable never
read: Name"), so a misspelled assignment `Coutner := Counter + 1` is caught.
Assigning a variable or one of its fields is not reading it. Parameters,
`for` iterators, State and Const are exempt; an exception binding (`On E T`)
is exempt only until exceptions have readable members, then the rule
applies and a handler that ignores the exception writes `On T`.

#### Scenario: Unread locals
- **WHEN** a local is only assigned, or a typo declares a new name
- **THEN** "local variable never read" is reported (tests/diagnostics/unread-typo.inox, tests/diagnostics/unread-assigned-only.inox, tests/diagnostics/unread-field-write-only.inox)

#### Scenario: Exemptions
- **WHEN** parameters, iterators, exception bindings, State and Const are never read
- **THEN** the program compiles (tests/semantic/valid/unread-exemptions.inox)

### Requirement: CANON-5 Grouped declarations
`A, B, C T := X` SHALL evaluate X exactly once; the first name receives the
value and each later name a copy, in order; afterwards they are independent.
`P, Q TPoint` gives each the type defaults; a scalar group needs an
initializer. Grouped move-only types are decided with Vector.

#### Scenario: Evaluated once
- **WHEN** `A, B Integer := Next(1)` runs
- **THEN** Next runs once (tests/runtime/grouped-declaration-once.inox); `A, B Integer` is rejected (tests/diagnostics/grouped-scalar-without-initializer.inox)

### Requirement: CANON-5 Assignment is a statement
`:=` SHALL form a statement and never appears inside an expression, a
condition or an initializer (the only `:=` in an argument list is named
construction, CANON-9). A chain `A := B := C := X` evaluates X once and
stores it right to left; each target is a variable or field path, never a
call or an index, and follows the first-appearance rule (ADR-0009).

#### Scenario: Chain
- **WHEN** `A := B := C := Next(10)` runs
- **THEN** Next runs once and all three hold its value (tests/runtime/chained-assignment.inox)

#### Scenario: Not an expression
- **WHEN** `:=` appears inside an expression, a condition or an initializer
- **THEN** it is rejected (tests/diagnostics/assignment-in-expression.inox, tests/diagnostics/assignment-in-condition.inox, tests/diagnostics/assignment-in-initializer.inox)

### Requirement: CANON-5 Compound assignment
`L += R`, `-=`, `*=`, `/=` and `^=` SHALL mean `L := L op R` with L evaluated
once (ADR-0012). The target exists already (a variable, field, or element
with side-effect-free indices); the form is not chainable and is not a read
of L. `/=` divides as `/` does, an Integer too (r3.95); `^=` is power.

#### Scenario: Compound forms
- **WHEN** a program uses the five forms
- **THEN** each updates its target (tests/runtime/compound-assignment.inox, `N /= 5` in tests/runtime/integer-slash.inox); a chain is an error (tests/diagnostics/compound-chained.inox)

### Requirement: CANON-5 Safe inference
Inference SHALL pick the most general safe type of a family, never a narrow
one: an integer literal is Integer, a real literal is Float, a call gives its
result type; `5` and `5.0` are different families. Narrow types (Natural,
Byte, fixed widths, Currency, Crypto) come only from a written type.

#### Scenario: Inferred types
- **WHEN** `X := 10` is declared and a String is then assigned to X
- **THEN** it is rejected because X is an Integer (tests/invalid/invalid-001.inox)

### Requirement: CANON-5 Const
`Const Name := Value` SHALL declare a module-level, immutable, single-line
constant whose type is the value's; a Const never has a written type
(`Const Mask := UInt8($FF)` chooses one by conversion). Its value may be a
constant expression, and a Const may follow the routine that uses it.

#### Scenario: Const forms
- **WHEN** a Const has a written type, or uses a conversion or an expression
- **THEN** the first is rejected (tests/diagnostics/const-typed-rejected.inox) and the others compile (tests/semantic/valid/const-conversion.inox, tests/runtime/const-expression.inox)

### Requirement: CANON-5 State
A `State` section (`State`, its variables, `;`) SHALL hold a module's
mutable global variables, explicit and rare: one declaration per line; scalars and
arrays need initializers; structs and Sets may omit them; `mut` is forbidden. Initializers run in source order
before `Main`. A module may have several State sections. Details:
`language/module-state`, `language/declaration-sections`.

#### Scenario: State variables
- **WHEN** routines read and write State variables initialized in order
- **THEN** they behave as module globals (tests/runtime/state-lowering.inox, tests/runtime/state-aggregates.inox)

#### Scenario: Several State sections
- **WHEN** several State sections stand between a Const, a Type section, a function, a method and Main
- **THEN** every routine sees every variable (tests/runtime/state-section-boundaries.inox)

### Requirement: CANON-5 Type section forms
A `Type` section SHALL declare Struct, Range and Enum types only. Inox has no
type alias form: `TName := Integer` is an error; the built-in aliases of
CANON-8 are the only aliases.

#### Scenario: No alias form
- **WHEN** a Type entry is `TName := Integer`
- **THEN** it is rejected (tests/diagnostics/type-assignment-form.inox); the three forms parse (tests/semantic/valid/type-section-forms.inox)

### Requirement: CANON-6 Functions and subroutines
A routine with a result type SHALL be a function (`Sum(A Integer) Integer
:`), one without a subroutine (`Show(X Integer) :`, `Main :`). A function
returns with `Return Expression` on every path and never falls through its
end; `Return Expression` is forbidden in a subroutine, which may fall
through. `Main` is the program's entry.

#### Scenario: Return rules
- **WHEN** a subroutine returns a value, or a function may reach its end without Return
- **THEN** it is rejected (tests/invalid/invalid-024.inox, tests/invalid/invalid-025.inox, tests/diagnostics/function-falls-through-if.inox); a function whose paths all return compiles (tests/runtime/function-all-paths-return.inox)

### Requirement: CANON-6 Exit
`Exit` SHALL leave a subroutine or `Main` without a value and is forbidden
in a function.

#### Scenario: Exit in a function
- **WHEN** a function uses `Exit`
- **THEN** it is rejected (tests/diagnostics/exit-in-function.inox)

### Requirement: CANON-7 Empty parentheses are forbidden
Parentheses SHALL appear only around at least one parameter or argument. A
routine without parameters is declared `Main :` / `Report :` and called
`Report` / `Account.Print`; `Main() :`, `Report()`, `Account.Print()`,
`Get()` and `TPoint()` are errors.

#### Scenario: Empty parentheses
- **WHEN** a declaration, call or method call is written with `()`
- **THEN** it is a parse error (tests/parser/invalid/empty-paren-declaration.inox, tests/parser/invalid/empty-paren-call.inox, tests/parser/invalid/empty-paren-method-call.inox, tests/parser/invalid/get-empty-parens.inox); the forms without parentheses parse (tests/parser/valid/no-empty-parentheses.inox)

### Requirement: CANON-7 A function is not a statement
A function or a method with a result SHALL NOT be called as a statement, as
in Ada (r3.88): `Twice(2)`, `Answer` or `C.Next` alone is a compile error
that names it and shows `X := Twice(...)` and `_ := Twice(...)`. Its result
is used by an expression or discarded on purpose. A subroutine is called as
a statement, without `_ :=`.

#### Scenario: Results dropped
- **WHEN** a function, a function without arguments, a method or a library function is called as a statement
- **THEN** each is rejected (tests/diagnostics/function-result-discarded.inox, tests/diagnostics/function-result-discarded-bare.inox, tests/diagnostics/function-result-discarded-method.inox, tests/diagnostics/function-result-discarded-builtin.inox)

### Requirement: CANON-7 Explicit discard
`_ := Call` SHALL drop the result of a call on purpose (r3.93). `_` is a
discard target, not a variable: no storage, no declaration, no part in name
resolution, not subject to OPEN-4. The call is evaluated exactly once, with
its effects, exceptions and contracts; an owned result is released.

#### Scenario: Discarded calls
- **WHEN** functions, a function without arguments, methods, a generic routine, a library function and functions returning a String, an owner, a struct or a Currency are discarded, and a discarded call raises or breaks a precondition
- **THEN** each runs once and nothing leaks (tests/runtime/discard.inox); the exception reaches its handler; the precondition traps (tests/runtime/discard-contract.inox)

### Requirement: CANON-7 What is discarded
The right of `_ :=` SHALL be a call of a function or a method that produces
a value. A literal, a variable, a field, an element, an operation, a
conversion, a construction or a call without a result is rejected: a
discard never stands for a read of a variable (OPEN-4). `_` is never read,
declared, compounded or chained. Types or functions with special handling
obligations may later forbid discarding their results.

#### Scenario: Invalid discards
- **WHEN** `_ :=` takes a literal, a variable, an operation, a field, an element, a conversion, a construction, a subroutine, PutLn or a method without a result, or `_` is read, declared, used as a parameter or routine name, compounded or chained
- **THEN** each is rejected (tests/diagnostics/discard-literal.inox, tests/diagnostics/discard-variable.inox, tests/diagnostics/discard-operation.inox, tests/diagnostics/discard-field.inox, tests/diagnostics/discard-index.inox, tests/diagnostics/discard-conversion.inox, tests/diagnostics/discard-construction.inox, tests/diagnostics/discard-subroutine.inox, tests/diagnostics/discard-putln.inox, tests/diagnostics/discard-method-subroutine.inox, tests/diagnostics/discard-read.inox, tests/diagnostics/discard-declaration.inox, tests/diagnostics/discard-parameter.inox, tests/diagnostics/discard-routine-name.inox, tests/diagnostics/discard-compound.inox, tests/diagnostics/discard-chain.inox)

### Requirement: CANON-8 Built-in types
The built-in types SHALL be exactly those of the table "CANON-8 built-in
types" above; each friendly alias names one base type, with at most one
friendly alias per width. `BigCurrency` (a big integer scaled 10^6) is
future (FUTURE).

#### Scenario: Aliases
- **WHEN** a program uses Byte where UInt8 is expected
- **THEN** they are the same type (tests/semantic/valid/byte-is-uint8.inox)

### Requirement: CANON-8 The numeric set is closed
Inox SHALL have no `Double`, `Extended`, `Decimal`, `Variant` or
`OleVariant`, and no dynamically typed value (ADR-0014).

#### Scenario: Unknown numeric type
- **WHEN** a program declares a Double
- **THEN** it is rejected as an unknown type (tests/diagnostics/double-is-not-inox.inox)

### Requirement: CANON-8 Conversions
Only safe widening defined by Inox SHALL be implicit (Natural to Integer).
Narrowing is never implicit: `T(X)` converts explicitly, checks T's range at
run time, and a constant outside T is a compile error. `Integer(FloatExpr)`
truncates toward zero and fails when out of range or NaN; `Float(I)`
rounds; `Float32(F)` rounds and fails beyond Float32. Storing an Integer
into a Natural is range-checked implicitly (ADR-0015). Text converts both
ways (r3.90, `language/reals`).

#### Scenario: Checked conversion
- **WHEN** `Byte(X)` receives 300 at run time, or `Byte(300)` is written
- **THEN** the first traps (tests/runtime/byte-conversion-runtime-range.inox) and the second is rejected (tests/diagnostics/byte-conversion-out-of-range.inox)

#### Scenario: Real conversions
- **WHEN** a program converts between integers, Float and Float32
- **THEN** the results round or truncate as `language/reals` says (tests/runtime/float-conversions.inox), and `Byte(F)` with F = 256.9 traps (tests/runtime/float-to-integer-range.inox)

#### Scenario: Implicit Natural check
- **WHEN** a negative Integer is stored into a Natural
- **THEN** it traps (tests/runtime/natural-assignment-negative.inox)

### Requirement: CANON-8 Numbers and text
`String(X)` of an integer or a real SHALL be the text Put writes; `T(S)` of
a String into an integer type, Float or Float32 SHALL read the whole text,
an optional `-` and digits (and for a real a fraction and an exponent),
within T; other text raises `ConversionError` (CANON-15), which a handler
catches (r3.90).

#### Scenario: Both ways
- **WHEN** numbers become text and text becomes numbers, valid or not
- **THEN** the values arrive and invalid text raises ConversionError (tests/runtime/number-text.inox)

### Requirement: CANON-8 Constants in fixed-width types
A constant Integer expression SHALL be read in a fixed-width type when it
fits — as an initializer, assigned value, argument, return or operand next
to that type (`B Byte := 200`, `B + 1`) — and is a compile error otherwise
(`B Byte := 256`). An Integer variable narrows only through `T(X)`.

#### Scenario: Constant fits
- **WHEN** a constant fits a narrow type, or an Integer variable is stored into a Byte
- **THEN** the first compiles (tests/runtime/fixed-width-integers.inox) and the second is rejected (tests/diagnostics/byte-from-integer-variable.inox, tests/diagnostics/byte-literal-out-of-range.inox)

### Requirement: CANON-8 UInt64 literals
In a UInt64 context only, an integer literal SHALL be allowed up to 2^64-1
(decimal or hexadecimal); elsewhere a literal must fit Int64. Unary minus on
a UInt64 is an error, and UInt64 and Integer never mix without a written
conversion.

#### Scenario: UInt64 values
- **WHEN** a UInt64 holds a literal above Int64.Max
- **THEN** it compiles and prints it (tests/runtime/uint64.inox)

#### Scenario: Outside a UInt64 context
- **WHEN** a program writes `X := 18446744073709551615`, `-U` or `U + I` with an Integer variable I
- **THEN** each is rejected (tests/diagnostics/uint64-literal-needs-context.inox, tests/diagnostics/uint64-unary-minus.inox, tests/diagnostics/uint64-mixed-integer.inox)

### Requirement: CANON-8 Exact decimals
A real literal in a Currency or Crypto context SHALL be read exactly in that
type, and more decimal places than the type holds is a compile error;
without context `Currency(19.99)` constructs one exactly, while
`Currency(FloatExpr)` is the explicit, possibly lossy conversion. Float and
decimal never convert implicitly; decimal overflow is an error.

#### Scenario: Decimal literals
- **WHEN** `P Currency := 19.99` and `Currency(19.999_999_9)` are written
- **THEN** the first is exact (tests/runtime/decimals.inox) and the second is a compile error (tests/diagnostics/decimal-too-many-places.inox)

### Requirement: CANON-8 Decimal arithmetic
Currency and Crypto SHALL have `+ - * /`, unary minus and order; `+` and
`-` are exact, `*` and `/` exact then rounded half to even to the type's
places (A1); beyond the type is the trap "decimal overflow". A literal next
to a decimal is read in its type (`Price * 3`), a variable of another type
is converted explicitly (A2); `^`, `mod` and `rem` do not apply (r3.91).

#### Scenario: Arithmetic
- **WHEN** a program computes with Currency and Crypto, mixes an Integer variable, or overflows
- **THEN** the results are exact or rounded half to even (tests/runtime/decimals.inox), the mix is rejected (tests/diagnostics/decimal-mix-integer.inox) and the overflow traps (tests/runtime/decimal-overflow.inox)

### Requirement: CANON-8 Decimal conversions
`Integer(C)` SHALL truncate toward zero and Std.Math `Round(X)` round half
to even, `Trunc(X)` toward zero (A3); `Crypto(C)` of a Currency is exact and
`Currency(Cr)` rounds half to even, as `Currency(F)` of a Float (A4); a
value beyond the target traps ("integer conversion out of range") (r3.91).

#### Scenario: Conversions
- **WHEN** decimals become integers, reals and the other decimal, and back
- **THEN** each truncates, rounds or is exact as stated (tests/runtime/decimals.inox), and a Crypto beyond Currency traps (tests/runtime/decimal-conversion-range.inox)

### Requirement: CANON-8 Operators
The operators SHALL be: Boolean `and` `or` `xor` `not`; integer bitwise
`bitand` `bitor` `bitxor` `bitnot` `shl` `shr`; `^` power; `/` division;
integer `mod` and `rem`. As in Ada, `/` of two integers is the integer division,
truncated toward zero, and of two reals or two decimals their division; an
integer and a real never meet in `/` without a conversion. `div` is not a
word of Inox (r3.95).

#### Scenario: Integer and real slash
- **WHEN** a program divides integers of every kind, reals and decimals with `/`, uses `/=` on an Integer and names a variable `div`
- **THEN** each gives its type's division (tests/runtime/integer-slash.inox); an Integer and a Float are rejected (tests/diagnostics/slash-mixed-types.inox); `A div 2` is a parse error (tests/diagnostics/div-is-a-name.inox)

### Requirement: CANON-8 Checked left shift
`A shl N` SHALL be the product A * 2^N, on signed and unsigned integers
alike, whenever it is representable in A's type (`-1 shl 1` is -2, `1 shl
63` fits a UInt64, not an Integer); otherwise it is a compile error for a
constant and a trap at run time, never lost bits (r3.97). N is checked
apart: 0..63. A shift that drops bits on purpose would be a distinct,
named operation.

#### Scenario: Shifts that fit and shifts that do not
- **WHEN** values of every sign are shifted within their type, a constant leaves Int64, and run-time shifts leave Int64, Int64.Min and a Byte
- **THEN** the first print A * 2^N (tests/runtime/shl-checked.inox), the second is rejected (tests/diagnostics/const-shl-overflow.inox) and the others trap (tests/runtime/shl-overflow.inox, tests/runtime/shl-overflow-negative.inox, tests/runtime/shl-byte-overflow.inox)

### Requirement: CANON-8 Modulus and remainder
As in Ada (r3.96), `A mod B` SHALL have the sign of B and `A rem B` the
sign of A, with A = (A / B) * B + A rem B: `-7 mod 2` is 1, `-7 rem 2` is
-1, `7 mod -2` is -1. `Int64.Min mod -1` and `Int64.Min rem -1` are 0. On
UInt64 both are the unsigned remainder. `rem` is a reserved word.

#### Scenario: Signs and edges
- **WHEN** mod and rem take every combination of signs, Int64.Min, constants, UInt64 and Byte, a zero divisor at run time or a constant one, or a Currency
- **THEN** the results follow Ada (tests/runtime/div-mod-signs.inox, tests/runtime/mod-rem-edges.inox, tests/runtime/integer-slash.inox); the zero traps (tests/runtime/rem-by-zero.inox) or is rejected (tests/diagnostics/const-rem-zero.inox); the Currency is rejected (tests/diagnostics/decimal-rem.inox); `rem` is not a name (tests/diagnostics/rem-reserved.inox)

#### Scenario: Operand types
- **WHEN** `bitand` gets a Bool or `and` gets an Integer
- **THEN** it is rejected (tests/invalid/invalid-003.inox, tests/invalid/invalid-016.inox)

### Requirement: CANON-8 Overflow is never silent
Integer overflow SHALL be invalid: never wraparound, never saturation, never
undefined. A constant overflow is a compile error; a run-time overflow is a
deterministic trap (CANON-19); the same holds for every fixed width (255 + 1
in a Byte traps).

#### Scenario: Overflow
- **WHEN** a constant expression overflows, or a value overflows at run time
- **THEN** the first is a compile error (tests/diagnostics/const-add-overflow.inox) and the second traps (tests/runtime/add-overflow.inox, tests/runtime/byte-overflow.inox)

### Requirement: CANON-8 Division by zero and power
A constant zero divisor of integer `/`, `mod` or `rem` SHALL be a compile error and a
run-time zero divisor a trap. Integer `^` needs a non-negative exponent (a
negative one is an error, never an implicit Float) and traps on overflow; on
Float `^` is the real power.

#### Scenario: Zero divisor
- **WHEN** a divisor is the constant 0, or becomes 0 at run time
- **THEN** the first is rejected (tests/diagnostics/const-div-zero.inox) and the second traps (tests/runtime/div-by-zero.inox)

#### Scenario: Negative exponent
- **WHEN** an integer power has a negative exponent
- **THEN** a constant is rejected (tests/diagnostics/const-pow-negative-exponent.inox) and a run-time one traps (tests/runtime/pow-negative-exponent.inox)

### Requirement: CANON-8 Generic syntax
Generic arguments SHALL be written in angle brackets — `Vector<Integer>`,
`Set<TSuit>`, `Slice<Integer>` — because `[T]` collides with indexing;
`<...>` is generic only in type position, where `<` and `>` are not
comparisons. `[...]` is for indexing, slicing and array bounds
(`Array[1..10] Integer`). A routine or a struct MAY declare type parameters
after its name: `Max<T>(A T, B T) T :`, `TPair<K, V> Struct`.

#### Scenario: Slice type
- **WHEN** a routine takes a `Slice<Integer>`
- **THEN** it compiles (tests/runtime/slices.inox)

#### Scenario: Generic routines
- **WHEN** a program calls generic routines with Integers, Floats, Strings, Bools, arrays, slices and ref mut arguments, and one generic routine calls another
- **THEN** each call runs its own instance (tests/runtime/generics-routines.inox)

#### Scenario: Generic structs
- **WHEN** a program builds generic structs from written types, from the target and from the values of the fields
- **THEN** each list of types is a struct of its own (tests/runtime/generics-structs.inox)

### Requirement: CANON-8 Generic instances
A generic routine or struct SHALL be a template: each use with its types
makes an instance (`Max<Int64>`, `TPair<String, Int64>`), checked and
compiled like ordinary code, one compiled copy per list of types. There are
no constraints: T may be any type, and an operation T lacks is an error of
the instance that needs it. Marcelo Fortes approved the three decisions on
2026-10-17.

#### Scenario: No constraints: the instance reports what T lacks
- **WHEN** a program calls `Max` with two structs
- **THEN** it is rejected in the instance Max<TPoint>, used in Main (tests/diagnostics/generics-no-operator.inox)

### Requirement: CANON-8 Type arguments are inferred
Type arguments SHALL come from the arguments of a call, a literal taking the
type the other arguments fix, or from the type of its target, and SHALL NOT
be written in an expression.

#### Scenario: Type arguments are never written in an expression
- **WHEN** a program writes `Max<Integer>(3, 7)`
- **THEN** it is rejected with the rule (tests/diagnostics/generics-written-arguments.inox)

#### Scenario: A type argument nothing fixes
- **WHEN** a program calls `Zero` with no argument and no typed target
- **THEN** it is rejected with how to give the type (tests/diagnostics/generics-unknown-type.inox)

### Requirement: CANON-9 Structs are nominal value types
A struct SHALL be declared in a `Type` section as `TName Struct` … `;` and
hold fields only; `TName Struct (TPart)` also reuses other structs. Assignment,
parameters and results copy it; the backend may pass receivers by pointer
without creating reference semantics. Details: `language/structs`.

#### Scenario: Value semantics
- **WHEN** a struct is copied and the copy is changed
- **THEN** the original is unchanged (tests/semantic/valid/struct-value-copy.inox, tests/runtime/struct-initializers.inox)

### Requirement: CANON-9 Fields
A field SHALL have a type and may have a literal default (`Fill(Literal)` for
an array field); a scalar, String, Set or pointer field without one starts at
its type's default, and an enum, array or type-parameter field without one
is given in the construction (CANON-5, r3.82). Type names conventionally start with `T`
and fields with `F` (style, not an error). A struct that contains itself,
directly or through other structs, is an error.

#### Scenario: Field rules
- **WHEN** a struct contains itself, or a field default is out of range
- **THEN** it is rejected (tests/diagnostics/struct-recursive.inox, tests/diagnostics/struct-recursive-indirect.inox, tests/diagnostics/struct-default-out-of-range.inox)

### Requirement: CANON-9 Associated methods
Methods SHALL be declared outside the struct: `TPoint.Move(Self mut, DX
Integer) :`, `TPoint.Sum(Self) Integer :`. The receiver is `Self`
(read-only) or `Self mut` (mutable); its type comes from the prefix, so
`Self TPoint` is not Inox; `Self owned` is reserved. `P.Move(3)` and `P.Sum`
are static calls, never virtual dispatch.

#### Scenario: Methods
- **WHEN** a method mutates through `Self mut` and another reads through `Self`
- **THEN** both work on any struct place (examples/associated-methods.inox, tests/runtime/struct-nested.inox)

#### Scenario: Receiver with a type
- **WHEN** a method writes `Self TPoint`
- **THEN** it is rejected (test to add: today it is rejected with a generic message)

### Requirement: CANON-9 Method reuse
`TName Struct (TPart, ...)` SHALL reuse each part: a field named after its
type that brings its fields and methods to the struct, at every level; a
reused method runs on its part. A name reached by two paths, or hidden by
the struct's own field, is an error at the declaration; a method replaces a
reused one only when marked `Override`. No subtyping. (Marcelo Fortes,
2026-10-13: inheritance is replaced by the reuse of methods.)

#### Scenario: Reuse through parts
- **WHEN** a struct reuses a struct that reuses a third and overrides one method
- **THEN** members are reached directly or by path and callers get the override (tests/runtime/method-reuse.inox, examples/method-reuse.inox)

#### Scenario: Ambiguity and hidden overrides
- **WHEN** two parts bring one name, or a method takes a reused name without Override
- **THEN** it is rejected (tests/diagnostics/reuse-name-two-paths.inox, tests/diagnostics/reuse-method-without-override.inox)

### Requirement: CANON-9 Methods of built-in types
A built-in value type SHALL accept associated methods like a struct's:
`String.UpdateStr(Self mut, Value String) :` serves every String variable
or field (`A.FPlan.UpdateStr("x")`). `Self` is a copy, `Self mut` the
caller's variable; a method belongs to its exact type.

#### Scenario: Methods of String and Integer
- **WHEN** methods of built-in types are called on variables, fields and expressions
- **THEN** they work (tests/runtime/builtin-type-methods.inox)

### Requirement: CANON-9 Named construction
`TPoint(FX := 10, FY := 20)` SHALL build a struct value as an expression:
named arguments only, in any order; a duplicate or unknown field is an
error; an omitted field takes its default, and an omitted field without
default (scalar, enum, array, type parameter, or a struct without complete
defaults) is an error. `TPoint()` is invalid; `P TPoint` gives all
defaults.

#### Scenario: Construction rules
- **WHEN** a construction is positional, repeats or invents a field, or omits a scalar without default
- **THEN** it is rejected (tests/diagnostics/construction-positional.inox, tests/diagnostics/construction-duplicate-field.inox, tests/diagnostics/construction-unknown-field.inox, tests/diagnostics/construction-omitted-scalar.inox, tests/diagnostics/struct-array-field-omitted.inox, tests/diagnostics/enum-field-omitted.inox, tests/diagnostics/nested-field-omitted.inox, tests/diagnostics/generic-field-omitted.inox); valid constructions run (tests/runtime/struct-construction.inox)

### Requirement: CANON-9 Structs are not compared
Structs SHALL never be compared with `=`, `#` or the ordering operators, as
in Pascal; a program compares their fields (maintainer decision, v3.57).

#### Scenario: Struct comparison
- **WHEN** two structs are compared
- **THEN** it is rejected with advice to compare fields (tests/diagnostics/struct-equality.inox, tests/diagnostics/struct-ordering.inox)

### Requirement: CANON-10 Parameters and mutability
A parameter SHALL be an immutable copy unless written `ref X T` (read-only
reference) or `ref mut X T` (writable reference, exclusive within the call;
ADR-0016, `language/ref-parameters`). `mut X T` parameters are reserved and
rejected. Local variables are mutable; a receiver mutates only through
`Self mut`.

#### Scenario: Immutable parameters
- **WHEN** a routine assigns a value parameter, or declares `mut X Integer`
- **THEN** it is rejected (tests/semantic/invalid/parameter-assignment.inox, tests/diagnostics/mut-parameter-reserved.inox)

#### Scenario: Read-only receivers and fields
- **WHEN** a method writes through `Self`, or a routine writes a field of a value parameter
- **THEN** it is rejected (tests/diagnostics/self-readonly-assignment.inox, tests/diagnostics/parameter-field-assignment.inox)

#### Scenario: References
- **WHEN** a routine writes through `ref mut` and reads through `ref`
- **THEN** the caller's variable changes and nothing is copied (tests/runtime/ref-parameters.inox)

### Requirement: CANON-10 Read-only through owners
What a `ref` parameter or a plain `Self` owns SHALL be read-only too,
through `Own` and `Option<Own>` at any depth, and no `Link` SHALL be made
from it (r3.109). A value parameter of an owner type owns its objects, and
a path through a `Link` stays writable.

#### Scenario: Read-only through owners
- **WHEN** a routine writes or links what a `ref` parameter or a plain `Self` owns, and another writes through `ref mut`, `Self mut`, a value owner parameter and a Link
- **THEN** the first are rejected and the others run (tests/diagnostics/ref-nested-owner-write.inox, tests/diagnostics/self-owner-write.inox, tests/diagnostics/link-from-ref-field.inox, tests/runtime/read-only-through-owners.inox)

### Requirement: CANON-10 Scope of locals
A local SHALL be visible from its declaration to the end of its block; using
it before or after is an error. Blocks: the routine body, each `if`/`elif`/
`else` branch, `while`, `repeat`, `for`, each `case` arm, `try`, `except`,
`ensure` and `with`.

#### Scenario: Scope ends with the block
- **WHEN** a name is used after its block or before its declaration
- **THEN** it is rejected (tests/semantic/invalid/local-out-of-scope.inox, tests/semantic/invalid/use-before-declaration.inox)

### Requirement: CANON-10 No shadowing
Shadowing SHALL be forbidden in the same and in every enclosing scope,
including names that differ only by case; `:=` to a visible outer name
assigns that variable. Diagnostics: "shadowing is forbidden: Name", "use
before declaration: Name".

#### Scenario: Shadowing
- **WHEN** an inner block declares a name visible outside it, or a scope declares a name twice
- **THEN** it is rejected (tests/semantic/invalid/shadowing-if-local.inox, tests/semantic/invalid/redeclare-same-scope.inox)

### Requirement: CANON-10 Loop iterators
A `for` iterator SHALL be implicit, read-only and visible only in its loop;
it may not conflict with any visible name ("loop iterator conflicts with
existing symbol: Name"; "cannot assign to read-only loop iterator: Name").
Sequential loops may reuse a name; nested loops may not.

#### Scenario: Iterators
- **WHEN** the body assigns its iterator, an iterator reuses a visible name, a nested loop reuses the outer name, or sequential loops reuse one name
- **THEN** the first three are rejected and the last compiles (tests/semantic/invalid/for-iterator-assignment.inox, tests/semantic/invalid/for-iterator-conflict.inox, tests/semantic/invalid/nested-for-same-iterator.inox, tests/semantic/valid/for-sequential-iterator-reuse.inox)

### Requirement: CANON-11 The with statement
`with Expression` SHALL open a block where `.Member` names a member of that
expression (the Visual Basic dot-prefix model) and unprefixed names follow
normal scope. In nested `with` blocks `.Member` binds to the innermost.
`with` declares no names, so no shadowing arises; no `:`; `;` closes. The
target, a variable or a value (never a type), is evaluated once on entry;
`.Member` acts on its storage, checked at each use through a pointer.

#### Scenario: Dot-prefix members
- **WHEN** `with P` assigns `.FX` and reads an unprefixed name
- **THEN** `.FX` is `P.FX` and the name resolves normally (tests/parser/valid/with-basic.inox, tests/parser/valid/with-scope.inox, examples/with-statement.inox)

#### Scenario: Nested with
- **WHEN** two `with` blocks nest
- **THEN** `.Member` binds to the inner one (tests/parser/valid/with-nested.inox)

#### Scenario: Any place, evaluated once
- **WHEN** the target is a field, an element, a field of a ref mut parameter, State, or reached through Own or Link, or an object is freed inside the body
- **THEN** `.Member` acts on that storage and the index is evaluated once (tests/runtime/with-places.inox); the freed object traps (tests/runtime/with-freed-target.inox); a type as target is rejected (tests/diagnostics/with-type-target.inox, tests/diagnostics/with-subroutine-target.inox)

### Requirement: CANON-12 if, elif, else and unless
`if C` … `elif C` … `else` … `;` SHALL use no `then` and no `:`, with one
`;` for the whole structure and none between branches; conditions are Bool.
`unless C` … `;` runs its body when C is false.

#### Scenario: if forms
- **WHEN** an if chain runs, a branch writes `then`, or a condition is not Bool
- **THEN** the chain picks one branch (tests/runtime/loop-if-elif-else.inox), and the others are rejected (tests/diagnostics/if-statement-then.inox, tests/invalid/invalid-002.inox, tests/invalid/invalid-027.inox)

#### Scenario: unless
- **WHEN** `unless` runs with a false and a true condition
- **THEN** only the first runs the body (tests/runtime/unless-lowering.inox)

### Requirement: CANON-12 Conditional expression
`IIf(C, A, B)` SHALL be the conditional expression (Marcelo Fortes,
2026-10-16; formerly `if C then A else B`, ADR-0013): three arguments; each
branch is an expression, never a block; only the chosen branch is evaluated,
unlike Visual Basic's IIf; the condition is Bool and the branches have
compatible types; it nests. It is never a statement.

#### Scenario: Lazy branches
- **WHEN** the branch not chosen would divide by zero
- **THEN** it never runs (tests/runtime/conditional-expression-lazy.inox); a missing value, a statement use or the old `if C then A else B` is rejected (tests/diagnostics/conditional-missing-else.inox, tests/diagnostics/conditional-as-statement.inox, tests/diagnostics/conditional-if-then-removed.inox)

### Requirement: CANON-12 while, leave and continue
`while C` … `;` SHALL repeat while C is true. `leave` exits the nearest loop
and `continue` starts its next iteration; both are valid only inside a loop.

#### Scenario: Transfers
- **WHEN** `leave` and `continue` run inside nested loops, or outside any loop
- **THEN** each acts on its own loop (tests/runtime/nested-loop-transfers.inox), and outside a loop they are rejected (tests/diagnostics/leave-outside-loop.inox, tests/invalid/invalid-007.inox)

### Requirement: CANON-12 repeat and until
`repeat` … `;` SHALL be a general loop; `until C` exits the nearest repeat
when C is true and may appear anywhere in its body, more than once, also
inside nested statements; it is valid only inside a repeat.

#### Scenario: until positions
- **WHEN** `until` appears at the start, middle and end of a repeat and inside nested statements
- **THEN** it exits its repeat (tests/runtime/until-positions.inox); outside a repeat it is rejected (tests/invalid/invalid-029.inox)

### Requirement: CANON-12 for loops
`for I in A..B` and `for I in A..B step S` (ADR-0010) SHALL include both
bounds: A < B counts up, A > B down, A = B runs once. The step is a positive
Integer: a constant step <= 0 is a compile error, a run-time one traps. The
bounds are range-level expressions (`1..N + 1`); the old `(S)` step form is
rejected; there is no `do`.

#### Scenario: Direction and step
- **WHEN** a loop counts down, has a step, or has a zero step
- **THEN** it runs as stated (tests/runtime/for-descending-range.inox, tests/runtime/for-step-keyword.inox), or traps or is rejected (tests/runtime/for-step-zero-runtime.inox, tests/diagnostics/for-step-zero-constant.inox, tests/diagnostics/for-old-step-spaced.inox, tests/parser/invalid/for-do.inox)

### Requirement: CANON-12 for evaluates its header once
The start, end and step of a `for` SHALL be evaluated exactly once, in that
order, before the first iteration (DECISION P-B); changing a variable used
in a bound does not change the range. The iterator never leaves A..B and the
loop never computes past Int64.Min or Int64.Max.

#### Scenario: Header evaluated once
- **WHEN** the bounds call functions, or the range ends at Int64.Max
- **THEN** each call happens once (tests/runtime/for-bounds-evaluated-once.inox, tests/runtime/for-step-evaluated-once.inox) and the loop ends (tests/runtime/for-range-ends-at-max.inox)

### Requirement: CANON-12 case
`case Selector` SHALL choose one arm (ADR-0011): a discrete or String
selector evaluated once; arms of static values or ranges (not of Strings)
separated by `|`; no overlap and no fall-through; `else` at most once, last,
and required unless the choices cover every value (always for a String); no
`of`, `when`, `=>`, `:` or `do`. `case` as an expression is reserved.
Details: `language/case-statement`.

#### Scenario: case
- **WHEN** a case runs, or overlaps, or misses values without `else`
- **THEN** it picks one arm (tests/runtime/case-lowering.inox) or is rejected (tests/diagnostics/case-overlap-range.inox, tests/diagnostics/case-integer-incomplete.inox)

#### Scenario: case on a String
- **WHEN** a case selects on a String with String choices and `else`, or without `else`
- **THEN** it picks one arm (tests/runtime/case-strings.inox) or is rejected (tests/diagnostics/case-string-without-else.inox)

### Requirement: CANON-13 Array
`Array[Low..High] T` SHALL be a fixed-size value type whose bounds are part
of the type; `Array[1..3, 1..4] T` is an array of arrays indexed `M[I, J]`.
Every index is checked; `Low`, `High` and `Length` are constants; arrays are
not compared; array literals are reserved. `Fill(Value)` is the array its
target expects, every element (or row) a copy of Value, evaluated once,
following the rules of types, copy and construction (r3.81). Details:
`language/fixed-arrays`.

#### Scenario: Bounds
- **WHEN** a program indexes outside the bounds at run time, or with a constant
- **THEN** it traps (tests/runtime/array-index-out-of-bounds.inox) or is rejected (tests/diagnostics/array-constant-index-out-of-bounds.inox)

### Requirement: CANON-13 Slice
`A[L..H]`, `A[..H]`, `A[L..]` and `A[..]` SHALL be read-only views of type
`Slice<T>` (ADR-0017) that keep the array's indices, are never empty, have
checked bounds, are bound once, and are never returned or stored, so a
slice never outlives its array. Details: `language/slices`.

#### Scenario: Slices
- **WHEN** a slice element is assigned or a slice is returned
- **THEN** it is rejected (tests/diagnostics/slice-readonly.inox, tests/diagnostics/slice-return.inox)

### Requirement: CANON-13 Vector
`Vector<T>` (future) SHALL be dynamic, 0-based, bounds-checked and owning:
assignment and by-value passing move it in O(1) and invalidate the source;
`Clone` copies; it never aliases implicitly.

#### Scenario: Move
- **WHEN** a Vector is assigned
- **THEN** it moves (test to add with the Vector change)

### Requirement: CANON-13 Range
`TMonth Range 1..12` and `TLetter Range 'A'..'Z'` SHALL be single lines of a
`Type` section. An Integer Range is a type of its own, as Ada's
`type T is range`: its values mix only with values of the same Range and
with constants, its arithmetic yields the Range, and the result is
range-checked when stored ("value out of range"); a constant outside is a
compile error. `Integer(X)` and `T(X)` convert explicitly, also between two
Ranges (r3.106).

#### Scenario: Range declarations
- **WHEN** a Range has no `Low..High`
- **THEN** it is rejected (tests/diagnostics/range-without-bounds.inox); a Range value is checked when stored, at run time and for a constant (tests/runtime/ranges.inox, tests/runtime/range-runtime-out-of-range.inox, tests/diagnostics/range-constant-out-of-range.inox)

#### Scenario: Range types do not mix
- **WHEN** a program mixes two Ranges, or a Range and an Integer variable, in an expression, an assignment or an argument
- **THEN** it is rejected (tests/diagnostics/range-mix-two-ranges.inox, tests/diagnostics/range-mix-integer-variable.inox, tests/diagnostics/range-into-integer-parameter.inox); with written conversions it runs (tests/runtime/range-types.inox)

### Requirement: CANON-13 Enum
An enum SHALL be short `TSuit (Club, Diamond)` or multi-line `TDay Enum` …
`;`: nominal and ordinal from 0, never converted implicitly to or from
Integer. `Ord(E)` gives the ordinal and `TEnum(I)` converts with a range
check. A literal name belongs to one enum only and is an identifier, never a
string.

#### Scenario: Enum declarations
- **WHEN** a multi-line enum puts two literals on one line
- **THEN** it is rejected (tests/diagnostics/enum-literals-per-line.inox); enum values are ordinals that never mix with Integer (tests/runtime/enums.inox, tests/diagnostics/enum-implicit-integer.inox, tests/runtime/enum-conversion-range.inox)

### Requirement: CANON-13 Enum operations
`Low(T)`, `High(T)`, `Succ` and `Pred` (checked) SHALL apply to enum and
range types; enum ranges are valid in `for`; `PutLn(E)` prints the
literal's name as declared (`Hearts`); an enum variable needs an
initializer (CANON-5).

#### Scenario: Printing an enum
- **WHEN** `PutLn(E)` runs with E = Hearts
- **THEN** it prints `Hearts` (tests/runtime/enums.inox); Succ past the last literal stops the program (tests/runtime/enum-succ-past-end.inox)

### Requirement: CANON-13 Set
`Set<T>` SHALL be a finite mathematical set over an Enum or finite Range
base of at most 256 values (not Integer, Float or String); a value type,
empty by default, never a hash set. `in` tests membership, `=`/`#` compare,
`<=`/`>=` test subset and superset; `Union`, `Intersection`, `Difference`,
`SymmetricDifference`, `Include`, `Exclude` return new sets and never
change their arguments, so a call that drops its result is an error (r3.81);
`[A, B]` is a literal in a Set context.

#### Scenario: Sets
- **WHEN** a program builds and tests sets
- **THEN** the results follow set algebra (tests/runtime/sets.inox); a base that is not an Enum or a Range of at most 256 values is rejected (tests/diagnostics/set-invalid-base.inox, tests/diagnostics/set-too-large.inox); `Include(S, X)` as a statement is rejected (tests/diagnostics/set-function-statement.inox)

### Requirement: CANON-14 String values
`String` SHALL be UTF-8, immutable and never null, with `""` as its default;
an absent string is a future `Option<String>`. A String lives in locals,
parameters, results, fields (default `""` or a literal), array elements,
State and `ref` parameters, with value semantics. Details:
`language/strings`.

#### Scenario: Strings
- **WHEN** a program stores and copies Strings everywhere a value lives
- **THEN** they behave as values (tests/runtime/strings.inox, tests/runtime/strings-aggregates.inox)

### Requirement: CANON-14 String comparison
`=` and `#` SHALL compare the bytes of two Strings; `<` `>` `<=` `>=` order
them byte by byte over UTF-8, a prefix first: Unicode code point order,
case-sensitive, never a locale collation (`"Z" < "a" < "á"`). Other
orderings are library functions (CANON-18). Details:
`language/comparisons`.

#### Scenario: Ordering
- **WHEN** Strings are compared and ordered
- **THEN** the results follow code point order (tests/runtime/strings-ordering.inox)

### Requirement: CANON-14 Concatenation
`A + B` SHALL join two Strings into a new String and `S += X` SHALL append X
to S; `+` on Strings needs no `Use`. A String and a value of another type
SHALL NOT be joined: there is no implicit conversion to String. A chain is
joined with one allocation, and `S := S + X` appends in place when S alone
holds its block, so N appends take amortized linear time and no other
String changes. Details: `stdlib/strings`.

#### Scenario: Join and append
- **WHEN** a program joins literals and variables, appends to a local, a State variable, a field, an element and a `ref mut` parameter, and appends 100 000 times
- **THEN** it prints the joined text, a copy made before an append keeps its value, and every block is freed (tests/runtime/strings-concat.inox)

#### Scenario: No implicit conversion
- **WHEN** a program writes `"N=" + N` with N an Integer
- **THEN** it is rejected (tests/diagnostics/string-plus-integer.inox)

### Requirement: CANON-14 String functions
`Use Std.Strings` SHALL give `Length`, `ByteLength`, `Substr`, `Position`,
`UpperCase` and `LowerCase` (also of a Char), `CharAt`, the Char classes
`IsLetter`, `IsDigit`, `IsSpace`, `IsUpper`, `IsLower`, `IsPunctuation`
(r3.89) and `FormatFloat` (r3.90). `Use Std.StrUtils` gives these and the
conveniences of the next requirement. Without the `Use`, the names are
free. Details: `stdlib/strings`.

#### Scenario: Without Use
- **WHEN** a program calls Length of a String or Trim without the module that gives it, or declares its own Trim and Left without `Use Std.StrUtils`
- **THEN** the first two are rejected naming the module, and the own routines are called (tests/diagnostics/string-length-without-use.inox, tests/diagnostics/string-function-without-use.inox, tests/runtime/string-names-free.inox)

### Requirement: CANON-14 String conveniences
`Use Std.StrUtils` SHALL give `Trim`, `LTrim`, `RTrim`, `Left`, `Right`,
`PadLeft`, `PadRight`, `Replicate`, `Replace`, `Contains`, `StartsWith`,
`EndsWith`, `SameText`, `CompareText`, `FieldCount` and `Field` (r3.90).

#### Scenario: Conveniences
- **WHEN** a program uses them on ASCII, multi-byte and empty Strings, or FieldCount with only Std.Strings
- **THEN** each gives the result of `stdlib/strings` (tests/runtime/strings-functions.inox, tests/runtime/fields.inox), and the last is rejected (tests/diagnostics/field-needs-strutils.inox)

### Requirement: CANON-14 Characters, not bytes
The String functions SHALL count code points: lengths and positions, the
first position being 1; `ByteLength` alone counts UTF-8 bytes. Case
mapping, case folding and white space SHALL follow Unicode, never one human
language or a locale.

#### Scenario: Functions
- **WHEN** a program calls every function on ASCII, multi-byte and empty Strings
- **THEN** each gives the result of `stdlib/strings` (tests/runtime/strings-functions.inox)

### Requirement: CANON-14 Reserved String operations
`S[I]` and `GraphemeLength` SHALL stay reserved until a later change
defines them; meanwhile `Substr(S, I, 1)` gives the character at position
I. `Pos` is a reserved word (CANON-2), so the search function is named
`Position`.

#### Scenario: Reserved operations
- **WHEN** a program writes `S[1]` with S a String
- **THEN** it is rejected, pointing to Substr (tests/diagnostics/string-index-reserved.inox)

### Requirement: CANON-14 Char
`Char` SHALL be a Unicode scalar value, U+0000..U+10FFFF without the
surrogates U+D800..U+DFFF: not a byte, an integer or a grapheme cluster. A
surrogate literal is a compile error; `Char(I)` (checked) and `Integer(C)`
are the only conversions with integers, `String(C)` the one with Strings.
A Char is not an ordinal: no `Ord`, `Succ`, `Pred` or arithmetic; Chars
compare by code point and form ranges (`'a'..'z'`). Details:
`language/chars`.

#### Scenario: Char assignment
- **WHEN** a String is assigned to a Char
- **THEN** it is rejected (tests/invalid/invalid-011.inox); a surrogate literal is rejected (tests/diagnostics/char-surrogate-literal.inox)

#### Scenario: Char values
- **WHEN** a program converts, compares, prints and stores Chars, iterates a Char range and reads `CharAt`
- **THEN** each follows `language/chars` (tests/runtime/chars.inox)

#### Scenario: Ord of a Char
- **WHEN** a program writes `Ord('A')`
- **THEN** it is rejected (tests/diagnostics/ord-of-char.inox)

### Requirement: CANON-15 The try statement
Exception handling SHALL be structured control flow over nominal exception
types without classical inheritance or general RTTI. `try` needs `except`,
`ensure` or both, in that order; it uses no `:` and no `Do`. A plain
`except` body catches every Inox exception; one final `;` closes the `try`.

#### Scenario: Forms of try
- **WHEN** a try has no handler, or writes `On E: T` or `Do`
- **THEN** it is rejected (tests/parser/invalid/try-without-handler.inox, tests/parser/invalid/on-binding-colon.inox, tests/parser/invalid/on-legacy-do.inox); the plain form catches (tests/integration/exceptions/plain-except.inox)

### Requirement: CANON-15 Typed handlers
A typed `except` SHALL hold `On T` or `On Name T` handlers (the binding is
immutable and local to the handler) and an optional `Else`, each closed by
its own `;`. Handlers are tried in source order; the first match wins; a
duplicate, or one an earlier handler covers through the taxonomy, is an
error; without a match or `Else` the exception propagates.

#### Scenario: Handler order
- **WHEN** a handler is duplicated or unreachable
- **THEN** it is rejected (tests/semantic/invalid/duplicate-exception-handler.inox, tests/semantic/invalid/unreachable-exception-subtype-handler.inox); matching and propagation run as stated (tests/integration/exceptions/taxonomy-range.inox, tests/integration/exceptions/typed-ensure.inox)

### Requirement: CANON-15 Raise
`Raise X` SHALL throw a new exception value; a bare `Raise` rethrows the
exception being handled, keeping its type, and is valid only in a handler.

#### Scenario: Rethrow
- **WHEN** a handler rethrows, or `Raise` appears outside a handler
- **THEN** the outer handler receives the same exception (tests/integration/exceptions/rethrow.inox), and the second is rejected (tests/semantic/invalid/raise-outside-handler.inox)

### Requirement: CANON-15 Retry
`Retry(N)`, valid only in an `On` or `Else` handler, SHALL run the whole try
body again, at most N more times, after `ensure` runs; when the budget is
exhausted it rethrows. It never retries one instruction or the routine, adds
no delay, and refers to the try whose handler is running.

#### Scenario: Retry counts
- **WHEN** a handler retries until success, until exhaustion, with zero, and nested
- **THEN** the counts and the rethrow are as stated (tests/integration/exceptions/retry-success.inox, tests/integration/exceptions/retry-exhausted.inox, tests/integration/exceptions/retry-zero.inox, tests/integration/exceptions/retry-nested.inox)

#### Scenario: Retry placement
- **WHEN** Retry appears outside a handler, in a plain except or in ensure
- **THEN** it is rejected (tests/semantic/invalid/retry-outside-handler.inox, tests/semantic/invalid/retry-plain-except.inox, tests/semantic/invalid/retry-ensure.inox)

### Requirement: CANON-15 ensure
`ensure` SHALL be cleanup, not a handler: it runs exactly once whenever
control leaves the protected construct — normally, by a handled or
propagating exception, `Raise`, `Retry`, `Return`, `Exit`, `leave`,
`continue` or `until`. An exception raised inside `ensure` replaces the one
propagating.

#### Scenario: Every departure
- **WHEN** control leaves a try by Return, Exit, leave, continue, until or an exception
- **THEN** ensure runs once (tests/integration/exceptions/ensure-control-transfers.inox, tests/integration/exceptions/until-crossing-ensure.inox, tests/integration/exceptions/ensure-propagation.inox)

### Requirement: CANON-15 Exception taxonomy
The standard exception types SHALL be those of the table "CANON-15 standard
exception taxonomy" above, part of the prelude. A handler matches its type
and its descendants, and `On Exception` must be the last handler. They are
types in a dedicated taxonomy, not enum values, and grant no inheritance of
fields or methods.

#### Scenario: Category matching
- **WHEN** `On IOError` meets FileNotFound and `On ArithmeticError` meets DivisionByZero
- **THEN** both match (tests/semantic/valid/exception-taxonomy.inox, tests/integration/exceptions/taxonomy-arithmetic.inox)

#### Scenario: ConversionError
- **WHEN** `On ConversionError` and `On Exception` meet text that is not a number
- **THEN** both match (tests/runtime/number-text.inox)

### Requirement: CANON-15 Deferred exception features
User-defined exception types, payload fields and reflection SHALL wait for
an approved design. A future Result value complements exceptions for
expected failures; contract violations stay a distinct concept; a run-time
arithmetic fault is not an exception (CANON-19).

#### Scenario: Unknown exception type
- **WHEN** a handler names a type outside the taxonomy
- **THEN** it is rejected (tests/semantic/invalid/unknown-exception-handler.inox)

### Requirement: CANON-17 Put and PutLn
`Put` and `PutLn` (`Std.IO`) SHALL take one or more arguments and write them
in order, without building a concatenated string; `PutLn` adds one line
break after the last argument: `PutLn("A", 10, "B", true)`.

#### Scenario: Several values
- **WHEN** PutLn receives several values of different types
- **THEN** they print in order on one line (examples/put-output-basic.inox)

### Requirement: CANON-17 The console
The console SHALL belong to libinoxrt: `Put`, `PutLn`, `Get`, `GetLn` and
Std.Terminal reach it only through the runtime, so output stays in order and
pending output is shown before the program waits for input. On Windows the
console is UTF-8 (input and output) with VT sequences while the program
runs, and is given back at exit (r3.88).

#### Scenario: Prompt and answer
- **WHEN** a program writes a prompt with Put and reads a line
- **THEN** the prompt comes before the read and the line arrives unchanged (tests/runtime/console-prompt.inox)

### Requirement: CANON-17 How reals are written
`Put`, `PutLn`, `Write`, `WriteLn` and `String(F)` SHALL write a Float or a
Float32 as the shortest text that reads back as the same value of its type:
fixed notation for exponents -4 to 15 with a digit after the point
(`2.5`, `1500.0`, `0.30000000000000004`), an exponent otherwise
(`1.0e16`, `1.5e-5`); `-0.0`, `NaN`, `Inf`, `-Inf` (r3.90).

#### Scenario: Reals printed
- **WHEN** a program prints ordinary, large, small, special and Float32 values
- **THEN** each prints its shortest text (tests/runtime/float-printing.inox)

### Requirement: CANON-17 How decimals are written
`Put`, `PutLn`, `Write`, `WriteLn` and `String(C)` SHALL write a Currency or
a Crypto with at least two decimals and no trailing zeros after them
(`19.99`, `5.00`, `0.123456`, `-0.50`) (A5); `Get` reads one strictly, an
optional `-`, digits and at most the type's decimals (r3.91).

#### Scenario: Decimals written and read
- **WHEN** a program prints decimals, reads two and writes them to a file, then reads one with too many places
- **THEN** each is written as stated and the last traps "invalid number input" (tests/runtime/decimals.inox, tests/runtime/decimals-input.inox)

### Requirement: CANON-17 Get and GetLn
`Get(X)` SHALL read one value into the variable X, and `GetLn(X)` reads one
and consumes the rest of the line; both take several variables in order.
`Get` and `GetLn` without arguments discard one token or the rest of the
line (the console pause). Arguments are assignable variables of an integer
type, Float, Float32, Char or (GetLn) String.

#### Scenario: Reading values
- **WHEN** GetLn reads two Integers from one line, or receives a literal
- **THEN** both values arrive (tests/integration/input/getln-two-integers.inox), and the literal is rejected (tests/semantic/invalid/get-literal.inox)

### Requirement: CANON-17 Integer input is strict
Integer input SHALL skip leading whitespace, accept an optional `-` and
digits, and end at whitespace or the end of input; anything else, or a
value outside the variable's type, traps ("invalid integer input").

#### Scenario: Integer input
- **WHEN** an Integer is read from valid input, with a plus sign, trailing garbage, at the end of input, or beyond Int64
- **THEN** the first is stored (tests/integration/input/get-integer.inox) and the others trap (tests/integration/input/get-integer-plus-sign.inox, tests/integration/input/get-integer-trailing-garbage.inox, tests/integration/input/get-integer-eof.inox, tests/integration/input/get-integer-overflow-positive.inox)

### Requirement: CANON-17 Reading a Char and the end of input
`Get(C)` SHALL read the next character exactly, a space or a line end
included; `GetLn(C)` then consumes the rest of the line unless C was the
line end. `EndOfInput` is true when nothing is left to read. Reading a Char
past the end traps ("end of input"); bytes that are not UTF-8 trap
("invalid UTF-8 input") (r3.89).

#### Scenario: Characters
- **WHEN** a program reads every character of its input, then one more, or bytes that are not UTF-8
- **THEN** it reads them all, spaces and line ends included (tests/runtime/get-char.inox), and the others trap (tests/runtime/get-char-end-of-input.inox, tests/runtime/get-char-invalid-utf8.inox)

### Requirement: CANON-17 Real input is strict
`Get(F)` of a Float or a Float32 SHALL skip white space and read an
optional `-`, digits, an optional fraction and an optional exponent, ending
at white space or the end; anything else traps ("invalid number input"), a
value beyond the type traps ("value out of range"). A Float32 is rounded
once from the text (r3.89).

#### Scenario: Reals
- **WHEN** reals are read, or text that is not one, or a value beyond Float32
- **THEN** they are stored (tests/runtime/get-real.inox) and the others trap (tests/runtime/get-real-invalid.inox, tests/runtime/get-real-range.inox)

### Requirement: CANON-17 GetLn into a String
`GetLn(S)` with a String variable SHALL read the rest of the line without
the line break (a CR before it is dropped); at the end of input S is `""`.
Malformed UTF-8 traps ("invalid UTF-8 input"). `Get(S)` with a String is an
error.

#### Scenario: Lines
- **WHEN** lines are read into Strings, or one line is not UTF-8
- **THEN** each line arrives without its break (tests/runtime/strings-getln.inox), the bad line traps (tests/runtime/strings-invalid-utf8.inox), and `Get(S)` is rejected (tests/diagnostics/get-string.inox)

### Requirement: CANON-17 Files are visible only with Use
`Use Std.Files` SHALL make the type `File` and the basic operations
(`OpenRead`, `Create`, `OpenAppend`, `ReadLn`, `Eof`, `Write`, `WriteLn`,
`Close`) visible; `Use Std.IOUtils` adds the conveniences (`Exists`,
`DeleteFile`, `RenameFile`) and gives everything of `Std.Files`. Without the
`Use` these names are free for the program's own routines. Details:
`stdlib/files`.

#### Scenario: Names stay free
- **WHEN** a program uses a file function without `Use`, or declares its own `Create`
- **THEN** the first is rejected (tests/diagnostics/file-without-use.inox) and the second compiles (tests/runtime/file-names-free.inox)

#### Scenario: Basics and conveniences
- **WHEN** a program with only `Use Std.Files` writes and reads a file, or calls `Exists`
- **THEN** the first works (tests/runtime/files-basic-module.inox) and the second asks for `Use Std.IOUtils` (tests/diagnostics/file-utility-needs-ioutils.inox)

### Requirement: CANON-17 Opening a File; a File is never copied
`OpenRead(Path)`, `Create(Path)` (new or truncated) and `OpenAppend(Path)`
SHALL return a new File. A File is never copied: it is a local that receives
a file just opened, or a `ref mut` parameter; never a field, element, State
variable or result, and never compared.

#### Scenario: Never copied
- **WHEN** a File is copied, stored in a field or returned
- **THEN** it is rejected (tests/diagnostics/file-copy.inox, tests/diagnostics/file-field.inox, tests/diagnostics/file-return.inox)

### Requirement: CANON-17 A File closes itself
A File SHALL close when the routine holding it exits by any path; a File
declared in a loop closes when the next iteration stores another;
`Close(F)` closes it earlier and closing twice does nothing; storing a new
file into a File variable closes the old one.

#### Scenario: Closed on an exception
- **WHEN** an exception leaves the routine that holds a File
- **THEN** the file is closed (tests/runtime/files-close-on-exception.inox)

### Requirement: CANON-17 Text in files
File text SHALL be UTF-8. `Write(F, ...)` and `WriteLn(F, ...)` write the
values PutLn prints, `WriteLn` ending with "\n" on every system;
`ReadLn(F)` accepts "\n" and "\r\n", returns the line without it, and ""
past the end, where `Eof(F)` is true.

#### Scenario: Write and read back
- **WHEN** a program writes, appends and reads a text file
- **THEN** the content round-trips (tests/runtime/files-text.inox)

### Requirement: CANON-17 File system functions
`Exists(Path)` SHALL be true for a regular file only; `DeleteFile(Path)`
never deletes a directory; `RenameFile(From, To)` raises AlreadyExists when
To exists, on every system. Binary I/O, directories, positions and other
encodings are later changes.

#### Scenario: File system
- **WHEN** a program checks, renames and deletes files
- **THEN** the results and the AlreadyExists rule hold (tests/runtime/files-system.inox)

### Requirement: CANON-17 File errors are exceptions
Every file failure SHALL raise a CANON-15 exception: FileNotFound,
PermissionDenied, AlreadyExists, DiskFull, otherwise IOError (also for a
closed file, invalid UTF-8, or reading a file opened for writing), handled
with try, `On`, `Else`, `ensure` and `Retry` like any exception.

#### Scenario: Missing file
- **WHEN** a program opens a file that does not exist inside try
- **THEN** `On FileNotFound` handles it (tests/runtime/files-not-found.inox)

### Requirement: CANON-18 Library principles
The standard library SHALL live in `stdlib/` as Inox modules, portable across
Windows and Linux, without garbage collection, unsafe features or C interop.
Modules come in pairs: `Std.X` holds a subject's types, data and
fundamental operations, the ones nearly every program needs; `Std.XUtils`
holds the conveniences built on them and passes `Std.X` on with `Export
Use Std.X` (r3.99). No module is a catch-all.

#### Scenario: A module pair
- **WHEN** a program uses `Std.IOUtils`, or only `Std.Files`
- **THEN** the first has the type, the basics and the conveniences (tests/runtime/files-text.inox), the second the type and the basics (tests/runtime/files-basic-module.inox)

### Requirement: CANON-18 Library modules
The library SHALL hold `Std.System` (the implicit prelude), `Std.IO` (Put,
PutLn, Get, GetLn), `Std.Math` (Integer helpers, the Float64 functions,
Round, Trunc), `Std.Debug` (`Assert`, a trap when false, only with its
`Use`), `Std.Files` / `Std.IOUtils` (CANON-17), `Std.Strings` /
`Std.StrUtils` (CANON-14), `Std.Terminal`, `Std.DateTime` /
`Std.DateUtils` and `Std.SystemUtils` (`stdlib/terminal`,
`stdlib/datetime`, `stdlib/systemutils`).

#### Scenario: Std.Math through Use
- **WHEN** a program writes `Use Std.Math` and calls its helpers
- **THEN** the module resolves through the library path and runs (tests/integration/stdlib/StdMathDemo.inox, tests/integration/modules/math-showcase.inox)

#### Scenario: Assert
- **WHEN** Assert holds, fails, fails with a message, is used without `Use Std.Debug`, or a program declares its own Assert without the module
- **THEN** it does nothing, traps, traps with the message, is rejected, and the own routine is called (tests/runtime/debug-assert-holds.inox, tests/runtime/debug-assert-fails.inox, tests/runtime/debug-assert-message.inox, tests/diagnostics/assert-without-use.inox, tests/runtime/assert-name-free.inox)

### Requirement: CANON-18 Planned modules
`Std.Strings` / `Std.StrUtils` SHALL grow, by Unicode rules never tied to
one human language, with grapheme lengths, root collation, locale collation
later, normalization, and `Split`/`Join` with the collections of 0.5;
running other programs comes after 1.0 (r3.92).

#### Scenario: Not available
- **WHEN** a program uses a module that does not exist, such as `Std.Process`
- **THEN** the module is not found (tests/diagnostics/module-not-found.inox)

### Requirement: CANON-19 Run-time faults are traps
A run-time fault SHALL stop the program at once with `Inox runtime error:
<category>` on standard error and exit status 70 (DECISION P-A): no
unwinding, no handler catches it, no `ensure` runs. The compiler checks
before every operation that can fault and never relies on a hardware
exception. Status 70 is reserved for these traps.

#### Scenario: Not catchable
- **WHEN** an overflow happens inside `try` with a plain `except` and `ensure`
- **THEN** the program stops with the trap and nothing else runs (tests/runtime/fault-not-catchable.inox, tests/runtime/fault-not-catchable-plain-except.inox)

### Requirement: CANON-19 Compile-time faults
A constant overflow, constant zero divisor, constant shift outside the
operand's width, constant negative exponent, constant `for` step <= 0 and a
literal outside its type SHALL be compiler diagnostics; knowledge about a
variable's value never turns a run-time check into a compile error.

#### Scenario: Constant operands
- **WHEN** `A / 0`, `A shl 64` or `A ^ (-1)` are written with A a variable
- **THEN** each is a compile error (tests/diagnostics/variable-mod-literal-zero.inox, tests/diagnostics/variable-shift-literal-64.inox, tests/diagnostics/variable-power-negative-literal.inox)

### Requirement: CANON-19 Trap categories
The category texts SHALL be: `integer overflow`, `decimal overflow`,
`division by zero`, `invalid shift count`, `for-loop step must be
positive`, `negative exponent`, `value out of range for Natural`, `integer
conversion out of range`, `array index out of bounds`, `slice bounds out of
range`, `out of memory`, `assertion failed` (`: Message`), `string index
out of range`, those of CANON-24 and CANON-25, `value out of range`, and
those of input below.

#### Scenario: Categories
- **WHEN** each kind of fault happens at run time
- **THEN** the program prints its category and exits with 70 (tests/runtime/mul-overflow.inox, tests/runtime/mod-by-zero.inox, tests/runtime/shl-count-64.inox, tests/runtime/natural-assignment-negative.inox, tests/runtime/slice-bounds-runtime.inox, tests/runtime/string-substr-out-of-range.inox)

### Requirement: CANON-19 Input trap categories
Reading input SHALL trap with `invalid integer input`, `invalid number
input` (a real or a decimal), `invalid UTF-8 input` and `end of input` (a
character or a key past the end) (r3.88, r3.89, r3.91).

#### Scenario: Input faults
- **WHEN** a program reads a bad integer, a bad real, bytes that are not UTF-8 and a character past the end
- **THEN** each traps with its category (tests/integration/input/get-integer-trailing-garbage.inox, tests/runtime/get-real-invalid.inox, tests/runtime/get-char-invalid-utf8.inox, tests/runtime/get-char-end-of-input.inox)

### Requirement: CANON-19 What traps
These operations SHALL trap: `+ - *`, unary `-` and `Abs` on overflow;
integer `/`, `mod` and `rem` by zero and `Int64.Min / -1`; a shift outside
0..width-1 and a `shl` beyond its type; `^`
on overflow or with a negative exponent; a `for` step <= 0; a conversion or
store outside the target range; an index or slice bound outside its array;
a String position, count or width outside what the function accepts.

#### Scenario: Edge cases
- **WHEN** `Int64.Min / -1`, `-Int64.Min` and `Abs(Int64.Min)` run
- **THEN** each traps (tests/runtime/div-min-by-minus-one.inox, tests/runtime/neg-overflow.inox, tests/runtime/abs-overflow.inox); `Int64.Min mod -1` is 0 (tests/runtime/mod-min-by-minus-one.inox)

### Requirement: CANON-19 Compiler, library and runtime
The compiler (`inox`), the standard library (`stdlib/`) and the runtime
`libinoxrt` (console, exception transport, Strings, Files) SHALL stay
distinct; every program links libinoxrt (r3.88); the runtime transports
opaque exception identities and the taxonomy belongs to the language. The library is found at `INOX_STDLIB`, the package's
`stdlib/`, beside the executable, under the working directory, or in the
source's directory or a parent.

#### Scenario: Library discovery
- **WHEN** a program uses `Std.Math` with no configuration
- **THEN** the library is found (tests/integration/stdlib/StdMathDemo.inox)

### Requirement: CANON-19 Compiler limits
The compiler SHALL reject, with "maximum ... nesting depth exceeded",
programs deeper than 128 levels of expression tree, 256 nested
expression-parser calls or 64 nested statements; each limit is about half
the depth measured to crash on a 1 MiB stack in a Debug+ASan build and is
re-measured before it is raised.

#### Scenario: Limits
- **WHEN** a program is one level deeper than a limit
- **THEN** it is rejected (tests/diagnostics/limit-parenthesis-depth.inox, tests/diagnostics/limit-statement-depth.inox), and the deepest accepted program compiles (tests/runtime/limits-boundary-ok.inox)

### Requirement: CANON-20 Operator precedence
Expressions SHALL bind by the table "CANON-20 precedence table" above and no
other; it changes only by an approved ADR. It follows mathematics and avoids
the Pascal mistake (relational operators bind tighter than logical ones)
and the C mistake (bitwise operators bind tighter than relational ones).

#### Scenario: Relational before logical
- **WHEN** a program writes `A < B and C < D`
- **THEN** it parses as `(A < B) and (C < D)` (tests/parser/valid/precedence-relational-logical.inox; examples/operator-precedence.inox)

### Requirement: CANON-20 Associativity
`^` SHALL be right-associative (`2 ^ 3 ^ 2` is `2 ^ (3 ^ 2)`). `..` and `in`
are non-associative: `A..B..C` and `X in A in B` are errors, and their
operands resolve first (`X in 1..N + 1`, `X in S = Flag` is `(X in S) =
Flag`; ADR-0008). Level 16 holds `:=` and the compound forms and is not an
expression operator.

#### Scenario: Range and membership
- **WHEN** a program chains `..` or `in`, or writes `X in 1..N + 1` and `X in S = Flag`
- **THEN** chains are errors (tests/diagnostics/range-non-associative.inox, tests/diagnostics/in-non-associative.inox) and the others group as stated (tests/semantic/valid/precedence-in-range-additive.inox, tests/semantic/valid/precedence-in-relational.inox)

### Requirement: CANON-20 Mandatory parentheses
Mixing two different operators among `and`, `or`, `xor`; two different
bitwise families among `bitand`, `bitxor`, `bitor`; or a bitwise operator
with a shift SHALL be a parse error, never a guess. Chains of one operator,
arithmetic levels, a relational over arithmetic and a logical over
relationals need no parentheses.

#### Scenario: Ambiguous mixes
- **WHEN** a program mixes `and` with `or` or `xor`, `bitand` with `bitor`, or `bitand` with `shl`
- **THEN** it is a parse error (tests/parser/invalid/precedence-mix-and-or.inox, tests/parser/invalid/precedence-mix-and-xor.inox, tests/parser/invalid/precedence-mix-bitand-bitor.inox, tests/parser/invalid/precedence-mix-bitand-shl.inox); the parenthesized forms parse (tests/parser/valid/precedence-logical-grouped.inox, tests/parser/valid/precedence-bitwise-grouped.inox)

### Requirement: CANON-21 The test suite is part of the law
Every change to the compiler, runtime, library or documentation SHALL pass
the full suite on Linux and Windows before it is merged; the two runners
hold the same checks. A bug fix adds a test that fails without the fix, in
the narrowest layer. Tests never depend on host paths; expected outputs come
from the language rules, never from capturing the compiler.

#### Scenario: Both runners agree
- **WHEN** the suite runs on Linux and Windows
- **THEN** both pass with the same checks (tools/scripts/run-tests.sh, tools/scripts/run-tests.ps1, .github/workflows/ci.yml)

### Requirement: CANON-21 Test layers
Tests SHALL live in layers: `tests/lexer`, `tests/parser`, `tests/semantic`,
`tests/invalid`, `tests/diagnostics` (`NAME.err` holds part of the
message, so the reason is tested), `tests/codegen` (IR fragments),
`tests/runtime` (`NAME.out`, `NAME.trap`, optional `NAME.in`),
`tests/integration`, `tests/eh-lifetime`, `tests/runtime-unit` and the
`examples/`.

#### Scenario: A runtime test
- **WHEN** a runtime test has a `.trap` file
- **THEN** the program must compile, run and stop with that diagnostic (tests/runtime/div-by-zero.inox)

### Requirement: CANON-23 LLVM backend and the clang driver
LLVM SHALL be the backend. The compiler emits textual LLVM IR and, for
`--build` and `--run`, calls the external `clang` (`clang++` with the static
`libinoxrt` when needed): a build-time dependency, not technical debt; built
programs do not need clang. A prebuilt compiler checks and emits IR without
LLVM or clang. Replacing clang follows `engineering.md` E22.

#### Scenario: Toolchain failure is reported
- **WHEN** clang or the linker fails during `--build`
- **THEN** the driver names the tool, its exit code and its own messages (tools/scripts/run-tests.sh, "run-hello --build (toolchain diagnostics)")

### Requirement: CANON-23 Lowering rules
Integer `/` SHALL lower to `sdiv` (`udiv` for UInt64), `rem` to `srem` and
`mod` to `srem` corrected to the divisor's sign (`urem` for both on UInt64),
through the checked helpers; real `/` to `fdiv`; no `nsw`/`nuw` flags: a
check is an explicit trap, never poison the optimizer may use (r3.98). Structs may lower to LLVM aggregates and receivers to pointers;
neither changes value semantics.

#### Scenario: Emitted IR
- **WHEN** control flow, structs and exceptions are compiled to IR
- **THEN** the IR holds the expected fragments (tests/codegen/llvm-control-flow-smoke.inox, tests/codegen/llvm-struct-value-smoke.inox, tests/codegen/llvm-exceptions-smoke.inox)

### Requirement: CANON-23 Valid IR or a clear stop
A construct accepted by semantic analysis but not lowered yet SHALL be
reported as "not yet implemented in the LLVM backend"; any other codegen
failure after semantic acceptance is a bug.

#### Scenario: No BUG
- **WHEN** every probe of the gap tool is compiled
- **THEN** none is a BUG (tools/backend_gaps.py; docs/BACKEND_GAPS.md)

### Requirement: CANON-23 Driver commands
The driver commands SHALL be `inox file.inox` and `inox run file.inox`
(build and run), `inox build`, `inox check` (parse and check), `inox
emit-llvm`, `inox help` and `inox version` (Marcelo Fortes, 2026-10-13).
`--run`, `--build`, `--emit-llvm`, `--help` and `--version` stay as aliases
for a transition; `--parse-only`, `--dump-tokens` and `--dump-types` are
developer switches. Words after the file of a run are the program's
arguments, and a run ends with the program's exit status (r3.92).

#### Scenario: Commands and aliases
- **WHEN** a program is run with `inox file`, `inox run` and `--run`, checked with `inox check`, and `inox help`, `inox version` and `inox run` without a file are called
- **THEN** each behaves as stated (tools/scripts/run-tests.sh, tools/scripts/run-tests.ps1: "inox check <file>", "inox help", "inox version = inox --version", "inox run without a file is an error", tests/integration/run-hello.inox)

#### Scenario: Arguments and status
- **WHEN** `inox run args.inox "um arg" "dois"` runs a program that prints its arguments and halts with their count
- **THEN** it prints both and the run ends with status 2 (tools/scripts/run-tests.sh, tools/scripts/run-tests.ps1: "inox run passes arguments and keeps Halt's status", tests/integration/system/args.inox)

### Requirement: CANON-23 Module interface and search path
`inox interface file.inox` SHALL print what other modules see of a module:
its `Export Use` lines, public types with their public fields and
invariants, public Consts, and the headers of its public routines with
their contracts. `--path <dir>` before the file, repeatable, adds a
directory where used modules are searched (CANON-3; r3.99).

#### Scenario: A module's interface
- **WHEN** `inox interface` reads Bank.Accounts, Bank and Std.DateUtils
- **THEN** it prints their public surface (tools/scripts/run-tests.sh, tools/scripts/run-tests.ps1: "B4: inox interface of Bank.Accounts", "B4: inox interface of Bank", "B4: inox interface of Std.DateUtils")

### Requirement: CANON-23 Optimization
`inox run`, `inox build` and `inox file.inox` SHALL build an optimized
program (clang -O2) by default; `--debug` before the file builds it without
optimization (-O0), and after the file it is the program's argument (r3.98).
Optimization never changes what a program does: the IR has no `nsw`/`nuw`
flags and every check traps explicitly. Optimization chosen in the source
for LLVM is a later design.

#### Scenario: Optimized and debug builds
- **WHEN** the whole suite runs optimized, and a run with `--debug` passes `--debug` after the file, computes 157 expressions and traps
- **THEN** every result, trap and diagnostic is the same (tools/scripts/run-tests.sh, tools/scripts/run-tests.ps1: "inox run --debug builds without optimization; --debug after the file is an argument", "inox --debug: math-expressions gives the same values without optimization", "inox run --debug: a trap stops the program without optimization")

### Requirement: CANON-24 Design by contract
Routines and structs SHALL carry Eiffel's contracts (Marcelo Fortes,
2026-10-14: Eiffel's design by contract, written with `pre` and `pos`): `pre` (Eiffel require) on the first lines of a routine,
checked on every call; `pos` (Eiffel ensure), checked on every normal
return; `invariant` in a Struct. Details: `language/contracts`.

#### Scenario: Contracts that hold
- **WHEN** routines with pre and pos, a struct invariant, an Override and loop contracts run within their contracts
- **THEN** the program runs normally (tests/runtime/contracts.inox, examples/contracts.inox)

### Requirement: CANON-24 Old and Result
In a `pos`, `old X` SHALL be X evaluated when the routine started (after
its `pre`), and `Result` the value a function returns. `old` exists only in
a `pos`; `Result` only in a function's `pos`. A `pos` is not checked when
the routine ends by an exception.

#### Scenario: Old and Result misused
- **WHEN** old appears in a pre, or Result in a pre or in a subroutine's pos
- **THEN** it is rejected (tests/diagnostics/contract-old-in-pre.inox, tests/diagnostics/contract-result-in-pre.inox, tests/diagnostics/contract-result-in-subroutine.inox)

### Requirement: CANON-24 Struct invariants
A struct's `invariant` SHALL hold, together with the invariants of the
structs it reuses, on every new value (construction or defaults), before
and after every method called on it from outside (`Self` calls excepted),
and after a field or a ref mut argument is changed from outside its own
methods. An invariant may name private fields (CANON-3), and a write to a
public field from another module checks it too (r3.99).

#### Scenario: Broken invariants
- **WHEN** a method, a field write, a ref mut argument or a construction breaks an invariant
- **THEN** the program traps (tests/runtime/contract-invariant-method.inox, tests/runtime/contract-invariant-field.inox, tests/runtime/contract-invariant-ref-mut.inox, tests/runtime/contract-invariant-construction.inox, tests/modules/invariant-after-public-write/Main.inox)

### Requirement: CANON-24 Contracts of an Override
An Override SHALL inherit the contracts of the methods it replaces, as
Eiffel redeclaration: its precondition is the inherited one `or else` its
own `pre else`, its postcondition the inherited one `and then` its own
`pos then`. A plain `pre` or `pos` in an Override, or `pre else`/`pos then`
elsewhere, is an error.

#### Scenario: Weakened precondition
- **WHEN** an Override accepts calls its part refuses, and a call satisfies neither
- **THEN** the first runs and the second traps naming both alternatives (tests/runtime/contract-override-pre.inox)

#### Scenario: Override keywords
- **WHEN** an Override writes `pre`, or a routine that is not one writes `pre else`
- **THEN** it is rejected (tests/diagnostics/contract-plain-pre-in-override.inox, tests/diagnostics/contract-pre-else-not-override.inox)

### Requirement: CANON-24 Loop contracts
The first lines of a `while`, `repeat` or `for` body MAY hold `invariant`
lines, checked at the start of every iteration and, for while and repeat,
when the loop ends; `while` and `repeat` MAY hold one `variant`, an integer
that SHALL be >= 0 and smaller at every iteration than at the previous one.

#### Scenario: Loops
- **WHEN** a loop invariant fails, or a variant does not decrease or turns negative
- **THEN** the program traps (tests/runtime/contract-loop-invariant.inox, tests/runtime/contract-loop-variant.inox, tests/runtime/contract-loop-variant-negative.inox)

### Requirement: CANON-24 A broken contract is a trap
A broken contract SHALL be a bug, never an exception: a CANON-19 trap
(`precondition violated`, `postcondition violated`, `invariant violated`,
`loop invariant violated`, `loop variant violated`) naming the routine or
struct, the condition as written and its message (`pre X > 0, "message"`).
Contracts are checked in every build through 1.0, with no switch that
turns them off, and only read: no Self mut method, ref mut argument, file
or input.

#### Scenario: Not catchable, read-only
- **WHEN** a pre fails inside try, or a contract calls a Self mut method or a ref mut routine
- **THEN** the program traps (tests/runtime/contract-pre-violated.inox), or the contract is rejected (tests/diagnostics/contract-calls-self-mut.inox, tests/diagnostics/contract-calls-ref-mut.inox)

### Requirement: CANON-25 Owners
Every heap object SHALL have one owner, `Own<T>`, made by `New(Value)` or
`New(TStruct)` (its defaults). An owner is never copied; `Move(P)` gives it
away. Its object is freed by `Free(P)`, when it is overwritten, when its
routine returns, or when its declaration runs again; the objects it owns are
freed with it, in a loop. `Option<Own<T>>` may be `None`, and takes an
`Own<T>` as is. No reference counting, no collector, no null.

#### Scenario: Owners
- **WHEN** a routine builds a list, returns it, moves owners, frees them, and frees a list of 100000 nodes
- **THEN** every object is freed once and nothing leaks (tests/runtime/pointers.inox, tests/runtime/pointers-strings.inox)

#### Scenario: Never copied, never empty
- **WHEN** an owner is copied, declared without New, or left out of a construction
- **THEN** it is rejected (tests/diagnostics/pointer-owner-copied.inox, tests/diagnostics/pointer-owner-without-new.inox, tests/diagnostics/pointer-construction-without-owner.inox)

### Requirement: CANON-25 Moves are checked
Using an owner variable after `Move` or `Free` on any path SHALL be a
compile error until it is assigned again; a loop gives an owner back before
its next round. Only a variable gives an owner away; a field that may be
emptied is `Option<Own<T>>`. A path the analysis does not follow traps
(`use of an owner after Move or Free`).

#### Scenario: Use after Move or Free
- **WHEN** an owner is used after Move, freed twice, freed on one branch, or moved in a loop
- **THEN** it is rejected (tests/diagnostics/pointer-use-after-move.inox, tests/diagnostics/pointer-double-free.inox, tests/diagnostics/pointer-moved-on-a-branch.inox, tests/diagnostics/pointer-moved-in-loop.inox), and a path through leave traps (tests/runtime/pointers-moved-owner.inox)

### Requirement: CANON-25 Links
`Link(X)` SHALL reach an owned object, a variable, a field or an element
without owning it, never one that is read-only through its owners
(CANON-10). A Link is copied freely, born empty, and checked on every
use: an object freed, a block reused, or a variable whose routine returned
is never reached; `Alive(L)` asks, the use traps (`use of a freed object or
an empty Link`). A Link to a variable lives until its routine returns or its
declaration runs again; to State, always.

#### Scenario: Links
- **WHEN** Links reach variables, fields, elements and objects, and are written through
- **THEN** they read and write the same storage (tests/runtime/pointers.inox, tests/runtime/pointers-more.inox)

#### Scenario: Detected, never dangling
- **WHEN** a Link is used after Free, after its block is reused, after its variable's routine returned, or while empty
- **THEN** the program traps (tests/runtime/pointers-freed-link.inox, tests/runtime/pointers-reused-block.inox, tests/runtime/pointers-escaped-link.inox, tests/runtime/pointers-empty-link.inox)

### Requirement: CANON-25 Access through pointers
`P.Field`, `P[I]` and `P.Method` SHALL reach the object of an `Own` or a
`Link` with the dot; `Value(P)` is the whole object, read or written.
Through a Link the object is writable; through an owner it is as writable
as the owner's place (CANON-10). `Same(A, B)` asks whether two pointers reach one object; `=` on pointers,
printing them and converting them to numbers are errors. An `Option<Own<T>>`
is reached through `Link(X)`, empty when it is `None`.

#### Scenario: Through pointers
- **WHEN** fields, elements, methods and Value are used through owners and Links
- **THEN** they act on the object (tests/runtime/pointers.inox, tests/runtime/pointers-more.inox)

#### Scenario: Not compared, Option reached through a Link
- **WHEN** two pointers are compared with `=`, or an Option field is read directly
- **THEN** it is rejected (tests/diagnostics/pointer-compared.inox, tests/diagnostics/pointer-option-direct.inox)

### Requirement: CANON-25 Owners in routines and contracts
A value parameter of an owner type SHALL belong to the routine, which frees
it unless it gives it away; the caller writes `Move(P)` or a new value. A
`pos` reads an owner parameter only through `old X.Field`; `old` of an
owner is an error. A Link to a parameter is not supported yet.

#### Scenario: Owner parameters
- **WHEN** a routine with pre and pos takes an owner and a Link
- **THEN** the contract holds and the owner is freed (tests/runtime/pointers-contracts.inox)

#### Scenario: Rejected
- **WHEN** a pos reads an owner parameter, old copies an owner, or a Link reaches a parameter
- **THEN** it is rejected (tests/diagnostics/pointer-pos-reads-owner.inox, tests/diagnostics/pointer-old-of-owner.inox, tests/diagnostics/pointer-link-to-parameter.inox)

### Requirement: VERSIONING The version number
Inox SHALL have one version for the language and its compiler,
`MAJOR.MINOR.RELEASE.BUILD` plus a stage: MAJOR 0..99 (a generation; 1 is
the first stable), MINOR 0..999 (features; before 1.0 it may change the
language), RELEASE 0..999 (fixes), BUILD 0..9999 (the commit count of the
compiled history, set by every build, never by hand; 0 outside git).

#### Scenario: Version output
- **WHEN** `inox --version` runs
- **THEN** it prints MAJOR.MINOR.RELEASE.BUILD and the stage (tools/scripts/run-tests.sh)

### Requirement: VERSIONING Who changes the version
MAJOR, MINOR, RELEASE and the stage SHALL change only by the maintainer's
decision, recorded in `revisions.md` and `CHANGELOG.md`; a new MINOR resets
RELEASE, a new MAJOR resets MINOR and RELEASE; BUILD is never reset. Stages:
pre-alpha, alpha (1.0 scope complete), beta (frozen), stable (1.0). Tags and
releases use `vMAJOR.MINOR.RELEASE`.

#### Scenario: One source of numbers
- **WHEN** the version changes
- **THEN** `src/compiler/support/Version.h` and the top of revisions.md change together, and the build reads its project version from Version.h (tools/scripts/run-tests.sh, CMakeLists.txt)

### Requirement: VERSIONING Releases follow the version
When a push to `main` carries a version without its tag
`vMAJOR.MINOR.RELEASE`, CI SHALL publish it by itself: Linux and Windows
packages built in Release, tested and run as a user would (E23), with the
version's CHANGELOG section as notes and the stage in the title, as the
latest release (decided by Marcelo Fortes, 2026-10-29). A push
that keeps the version publishes nothing; old releases stay as history.

#### Scenario: Publishing a version
- **WHEN** the version in Version.h changes on main
- **THEN** the release workflow tags it and publishes both packages (.github/workflows/release.yml)

### Requirement: VERSIONING Canon revision
The canon revision `rN.NN` SHALL number a set of recorded decisions (G3) and
is never a version; entries up to v3.57 keep their `v3.xx` labels. `inox
--version` prints `inox MAJOR.MINOR.RELEASE.BUILD stage (canon rN.NN)`.

#### Scenario: Revision in the version line
- **WHEN** `inox --version` runs
- **THEN** it ends with the canon revision at the top of revisions.md (tools/scripts/run-tests.sh, tools/scripts/run-tests.ps1)

### Requirement: FUTURE Concurrency
Concurrency SHALL follow Chapel-style structured parallelism and Go-inspired
concurrency without unsafe shared mutable defaults, with data-race safety;
the construct is spelled `do` (reserved), never `go`; the `|...|` capture
idea is not decided. It needs an approved design before implementation.

#### Scenario: Reserved word
- **WHEN** a program uses `do` as a name
- **THEN** it is rejected (tests/diagnostics/do-reserved.inox)

### Requirement: FUTURE Generics and static proof
No decision SHALL introduce traits, protocols, interfaces or behaviors as
nominal entities: behavior is reused by method reuse (CANON-9) and
contracts (CANON-24) are checks, never types. Generics came first, in their
simplest form (CANON-8); written constraints come only if real programs ask
for them, and methods of generic structs follow. After 1.0: contract
monitoring levels per module (Eiffel style) and static proof (SPARK
style). Each needs an approved design.

#### Scenario: Methods of generic structs not yet in the language
- **WHEN** a program declares a method of a generic struct
- **THEN** it is rejected with the generic routine that does the job (tests/diagnostics/generics-method.inox)

### Requirement: FUTURE Ownership, absence and failure
Ownership beyond CANON-25 (one owner, checked Links) SHALL come through
approved designs: arenas as an optional optimization, `Self owned`; unsafe
boundaries, `Pointer<T>` and C interop stay outside the safe core.
`Option<T>` for any T and `Result<T, E>` express absence and failure as
values.

#### Scenario: mut stays reserved
- **WHEN** a program writes `mut` before a name
- **THEN** it is rejected (tests/diagnostics/mut-reserved.inox)

### Requirement: FUTURE Modules, variants and numbers
A package tool, downloading packages into a folder of the CANON-3 search
order, SHALL extend modules (2026-10-29). Variant structs (one of several
structs, examined with `case`) are how a collection mixes kinds without
subtyping
(a direction recorded by Marcelo Fortes,
2026-10-14). Metadata tags (JSON,
databases), BigCurrency, symbolic mathematics and `Float(Sum / N)` as the
exact quotient of two integers (2026-10-16) are later directions. Each
needs an approved design.

#### Scenario: No package tool yet
- **WHEN** a program uses a module that no searched directory holds
- **THEN** it is not found until the package tool exists (test to add)

### Requirement: FUTURE Encapsulation, routine types and lifecycle
Encapsulation is CANON-3 (r3.99); read-only fields, which other modules
read and only their module writes, SHALL come from a later design (deferred by Marcelo
Fortes, 2026-10-29). Routine types (a
routine kept in a variable, field or parameter, as Pascal's procedural
types) are recorded with the recommendation: named routines without
captured variables first, closures only after safe pointers. Whether Inox needs constructors and destructors is
the maintainer's open analysis.

#### Scenario: Not yet in the language
- **WHEN** a program declares a read-only field or a routine type
- **THEN** it is not accepted until the design is approved (test to add)

