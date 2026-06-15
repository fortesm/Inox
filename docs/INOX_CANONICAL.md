# INOX_CANONICAL.md
# ============================================================================
# THE SINGLE CANONICAL DOCUMENT FOR THE INOX PROGRAMMING LANGUAGE
# ============================================================================
# This file REPLACES the entire docs/ directory. Every former file under docs/
# (canonical/, decisions/, development/, future/, open-questions/, release/, the
# public LANGUAGE_REFERENCE.md, and the HTML manual) is CONSOLIDATED here as a
# named GROUP/SECTION. There are no separate spec files. One file, one truth.
#
# It is an ANTI-REGRESSION CONTRACT designed to survive total context loss.
# Any AI or human arriving with NO prior memory MUST reconstruct the correct
# understanding of Inox from THIS FILE plus the source code alone, and MUST NOT
# invent, infer, or implement anything outside this specification.
#
# Maintainer / sole design authority: Marcelo Fortes
# Version: v3.3 (0.1 design complete — zero open questions)
# Last updated: 2026-06-14
# Repository: github.com/fortesm/Inox
# License: Mozilla Public License 2.0 (MPL-2.0), without the "Incompatible With
#          Secondary Licenses" notice.
# ============================================================================
#
# DOCUMENT MAP — how the former docs/ tree maps into this file
# ----------------------------------------------------------------------------
#   GOVERNANCE + CHANGE LOG ............ new (anti-regression machinery)
#   LAYER A — CONSTITUTION (the law) ... canonical/* + decisions/* consolidated
#   GROUP: CANONICAL ................... canonical/language-reference, vision,
#                                        syntax, semantics, type-system, runtime,
#                                        llvm-backend, roadmap, testing
#   GROUP: DECISIONS ................... decisions/ADR-0001..0006
#   GROUP: DEVELOPMENT ................. development/toolchain + release/*
#   GROUP: FUTURE ...................... future/* (concurrency, contracts,
#                                        generics-advanced, package-manager,
#                                        protocols, symbolic-math) + roadmap 0.2+
#   GROUP: OPEN-QUESTIONS .............. open-questions/OPEN_QUESTIONS
#   LAYER B — IMPLEMENTATION STATUS .... volatile current state of the compiler
#
# NOTE ON EMPTY FILES: in the source docs/ tree, decisions/ADR-0001..0004 and all
# six future/*.md were EMPTY (0 bytes). Their TOPICS are preserved here as filled
# sections (their intended subject is documented), so no information path is lost.
#
# ============================================================================
# HOW TO READ THIS DOCUMENT (read this first, every time, no exceptions)
# ============================================================================
#
# TWO LAYERS that must never be confused:
#
#   LAYER A — THE CONSTITUTION (the LAW). Design decisions TRUE FOREVER until
#       changed by an approved ADR. NEVER goes stale. If the code disagrees with
#       Layer A, THE CODE HAS A BUG — fix the code, never edit Layer A to match.
#
#   LAYER B — IMPLEMENTATION STATUS (VOLATILE). The current state of the
#       compiler/toolchain/tests/examples. An AI MAY update Layer B to match the
#       code. An AI MUST NOT update Layer A to match the code.
#
# Reading order for a context-less AI:
#   HOW TO READ -> GOVERNANCE -> CHANGE LOG -> LAYER A -> LAYER B.
#
# ============================================================================
# GOVERNANCE — RULES THAT PROTECT THIS DOCUMENT (binding on all AIs and humans)
# ============================================================================
#
# G1. PRECEDENCE WHEN DOCUMENT AND CODE DISAGREE
#     - On LANGUAGE DESIGN  -> THE DOCUMENT WINS. The code has a bug. Fix code.
#     - On CURRENT STATUS   -> THE CODE WINS. Update Layer B.
#     - ANY divergence is a BUG TO REPORT, never silently resolved either way.
#
# G2. AUTHORITY TO CHANGE DESIGN
#     - ONLY Marcelo Fortes approves changes to Layer A.
#     - An AI MAY PROPOSE a change but MUST mark it PROPOSAL in Section B-PROPOSALS
#       and MUST NOT apply it as fact. Locked (ADR) items change only via a new,
#       dated, approved ADR. Never by silent edit.
#
# G3. CHANGE LOG IS MANDATORY
#     - Every approved design change adds a dated, attributed entry below.
#     - If code contradicts a RECENT change-log entry, the CODE is behind and
#       must be updated. The decision is intentional, not a mistake to "fix".
#
# G4. NO SILENT SCOPE EXPANSION
#     - If a behavior is not covered here, DO NOT infer it from another language
#       (C, C++, Rust, Ada, Pascal, Python, Go, VB...). STOP and request a
#       design decision; record it via ADR + CHANGE LOG once approved.
#       (This is the canonical "Agent rule" from the former vision.md.)
#
# G5. VERIFIABILITY
#     - Critical rules are backed by test fixtures. "Code obeys the document" is
#       PROVEN by running the suite, not asserted.
#
# ============================================================================
# CHANGE LOG (newest first — dated, attributed, append-only)
# ============================================================================
#
# v3.3 — 2026-06-14 — approved by Marcelo Fortes
#   - DIGIT SEPARATOR `_` adopted: `10_000_000` == `10000000`, cosmetic, all
#     numeric literals, only between digits (CANON-2).
#   - DECIMAL LITERALS context-free form decided: type constructor
#     `Currency(19.99)` / `Crypto(0.001)`, coherent with struct construction.
#     Closes OPEN-2 (CANON-8). The 0.1 language now has ZERO open design questions.
#
# v3.2 — 2026-06-14 — approved by Marcelo Fortes
#   - NAMED STRUCT CONSTRUCTION is canonical: `TPoint(FX := 10, FY := 20)`.
#     Named-only (positional invalid); `:=` separator (keeps `:` exclusive to
#     function declaration); order free; omitted field uses default or is an error
#     if a scalar without default; duplicate/unknown field is an error; empty
#     `Tipo()` invalid (use `C TPoint` for all-defaults). Closes OPEN-1.
#
# v3.1 — 2026-06-14 — approved by Marcelo Fortes
#   - DIVISION BY ZERO: constant divisor zero is a COMPILE-TIME ERROR; dynamic
#     divisor zero is a RUNTIME TRAP (CANON-8).
#   - EXPONENTIATION `^`: integer `^` returns Integer with non-negative exponent
#     only; negative exponent is an error (no implicit float); integer `^`
#     overflow is an error/trap; float `^` (future) returns Float (CANON-8).
#   - DECIMAL LITERALS (in-context): a real literal in a decimal-typed context is
#     read directly as that decimal type, exact, with excess-precision being a
#     compile-time error (Ada model). Context-free decimal literal spelling left
#     OPEN (see Layer B / B-OPEN).
#   - Tracked OPEN-1 (struct value construction) and OPEN-2 (context-free decimal
#     literal) in Layer B / B-OPEN.
#
# v3.0 — 2026-06-14 — approved by Marcelo Fortes
#   - FULL CONSOLIDATION: the entire docs/ tree is absorbed into this one file as
#     named groups (CANONICAL, DECISIONS, DEVELOPMENT, FUTURE, OPEN-QUESTIONS),
#     preserving the actual content of every non-empty file verbatim-in-substance
#     (not paraphrased away). docs/ is to be deleted.
#   - Added: full influences list (incl. Vala), mission-critical failure modes,
#     complete Set operations, Std.Math helper list, three runtime boundaries,
#     5-step stdlib discovery order, full test-mode list, Integer(FloatExpr)
#     truncation, MPL-2.0 license, prebuilt package layouts, toolchain profiles.
#
# v2.2 — 2026-06-14 — approved by Marcelo Fortes
#   - `with` added (VB dot-prefix model). `:` rule made explicit (functions only).
#
# v2.1 — 2026-06-14 — approved by Marcelo Fortes
#   - Two-layer structure (Constitution / Status) + Governance G1–G5.
#
# v2.0 — 2026-06-14 — approved by Marcelo Fortes
#   - `Var` block REMOVED; inline-only declarations; safe Ada/SPARK inference;
#     scalars/enums require initializers, structs use type-default init; type
#     hierarchy finalized; Currency i64x10^6, Crypto i128x10^18, BigCurrency 0.2+;
#     enum init strict Ada.
#
# v1.0 — 2026-06-13 — baseline audit of Inox.zip source tree.

# ############################################################################
# #                  LAYER A — THE CONSTITUTION (THE LAW)                     #
# #        True until changed by approved ADR. Code that disagrees has a bug. #
# ############################################################################
#
# Layer A is organized into the same GROUPS the docs/ tree used, so every former
# file has an obvious home: CANONICAL, DECISIONS, DEVELOPMENT, FUTURE,
# OPEN-QUESTIONS.


# ============================================================================
# GROUP: CANONICAL
# (absorbs docs/canonical/: vision, language-reference, syntax, semantics,
#  type-system, runtime, llvm-backend, roadmap, testing)
# ============================================================================

## CANON-1. VISION AND RATIONALE (was canonical/vision.md)

Inox is a compiled, strongly typed, post-object-oriented systems language for
software where silent failure is unacceptable. Its goal is not to imitate one
existing language. Inox deliberately takes inspiration from several traditions
and rejects defaults that are unsafe, obsolete, ambiguous, or hostile to
large-scale engineering. Inox prefers explicit safety over convenience when the
two conflict.

### Language references and influences (drinks from, does not copy mistakes)
- C and C++: performance, low-level realism, convenient operators — while
  rejecting undefined behavior as a design principle.
- Ada and SPARK: robustness, soundness, contract-oriented thinking,
  mission-critical discipline.
- Modula-2, Modula-3, Oberon, Component Pascal, Zonnon: modules, clarity,
  restraint, neglected good ideas.
- Eiffel and Sather: correctness, contracts, design-by-contract.
- Chapel: structured high-performance parallelism, data-locality.
- Modern Object Pascal, Delphi, Free Pascal: productivity, Pascal-family ergonomics.
- Go: composition, slices, simple concurrency ideas — while rejecting implicit
  aliasing hazards and GC-dependent latency as core assumptions.
- Rust: ownership, explicit mutability, composition, memory safety without GC.
- Swift, Kotlin, C#, Java: modern ergonomics and mature ecosystem lessons.
- Vala: useful systems-programming ergonomics where applicable.
- Julia: mathematics, scientific computing, finance, numerical expressiveness.
- Perl, Ruby, Python, PHP: expressive manipulation of strings, lists, maps,
  reductions, regular expressions, high-level data transformation.

### Mission-critical target domain
Aviation, air-traffic control, high-precision industry, finance, stock
exchanges, cryptoasset infrastructure, international monetary systems, scientific
computing, aerospace, medicine, hospital machinery, nuclear and hydroelectric
plants, electrical grid infrastructure, cryptography, large-scale parallel
computation.

In such domains these failure modes can cost lives, destroy capital, or
compromise infrastructure, and Inox is designed to prevent them: buffer overflow,
null dereference, use-after-free, silent integer overflow, silent division by
zero, accidental mutation, unchecked indexing, implicit narrowing, hidden
aliasing, nondeterministic latency.

### Core safety stance (Inox 0.1 defaults)
- no universal `null` or `nil`;
- no unsafe pointers in the safe language core;
- no silent integer overflow as guaranteed semantics;
- no integer `/`; use explicit `div` and `mod`;
- no unchecked array bounds;
- no implicit narrowing conversions;
- no implicit aliasing for future `Vector[T]`;
- parameters are immutable by default;
- mutating methods require `Self mut`;
- structs are data, not classes;
- associated methods provide behavior without inheritance;
- contracts/protocols/behaviors are future static capability checks, not
  Java-style interfaces.

### Post-object-oriented design
No classes, classical inheritance, Java-style interfaces, mixins, duck typing, or
class taxonomies. It keeps the ergonomic call form `Object.Method(args)` without
turning data into objects in the classical OO sense. Foundation: `Struct` for
data; free functions and subroutines; associated methods declared outside
structs; composition instead of inheritance; future contracts/protocols/
behaviors for static capability checks; strong nominal typing.

### Performance model
LLVM is the backend. The compiler itself must remain portable C++20 and build on
Windows/MSVC and Linux/GCC or Clang. Future portability should account for
FreeBSD, Solaris, AIX, HP-UX, UnixWare, and other Unix systems where realistic.
Inox should not rely on a tracing GC for the language core. Future memory work
should prefer explicit ownership, moves, arenas, deterministic resource
management, and controlled borrowing.

### Agent rule (canonical, from vision.md — see also GOVERNANCE G4)
When a design question is not covered by the canonical documentation, do not
infer from another language. Ask for a language decision and update the canonical
specification, ADRs, manual HTML, and tests.

## CANON-2. LEXICAL RULES (was canonical/language-reference §Lexical + semantics)

- Case-INSENSITIVE for keywords and identifiers. `PutLn`, `putln`, `PUTLN` are
  equivalent. Docs use canonical spelling. Name comparison is case-insensitive,
  so `Valor` and `valor` CONFLICT.
- Line comments use `==` until end of line: `X := 10  == comment`.
- Block comments do NOT exist in 0.1.
- String literals use double quotes `"..."`. Character literals use single
  quotes `'a'`, `'é'`, `'😀'`.
- Integer literal `42`; hex `$2A`, `$FF`. Real literal `3.14`, `0.0`.
- DIGIT SEPARATOR `_` (CHANGE LOG v3.3): an underscore may separate digits in any
  numeric literal for readability and is purely cosmetic (the lexer ignores it
  when forming the value). `10_000_000` == `10000000`. Applies to all numeric
  literals: integers `1_000_000`, reals `3.141_592_653`, hex `$FF_FF_FF`, and
  decimals `1_000_000.00`. RULES: `_` may appear ONLY between digits — never at
  the start (`_1000`), never at the end (`1000_`), never doubled (`1__000`); each
  of those is an error. No ambiguity with identifiers: a numeric literal starts
  with a digit, identifiers start with a letter or `_`.
- Operators: `:=` declare/assign-init, `=` equality, `#` inequality,
  `< <= > >=`, `..` range, `:` block-open for functions/subroutines ONLY (see
  CANON-4), `;` block-close (NOT a statement terminator), `.` member access,
  `,` separator, `^` exponentiation (never XOR).
- `End`/`end` is NOT a keyword and is NEVER a block closer. Only `;` closes
  blocks. (This deliberately avoids retaining Pascal/Ruby legacy alternatives.)

## CANON-3. MODULES AND USE (was canonical/language-reference §Modules + semantics)

- `Module Name` is the FIRST declaration in a file. No `;`. EOF closes the module.
- One `.inox` file = one logical module.
- `Use Name` declares a SEMANTIC dependency. It is NOT textual inclusion, NOT
  `#include`, NOT source copying, NOT manual linking. It tells the compiler to
  load imported module types and signatures.
- Compact form allowed: `Module Calc.Core Use Sys.IO Use Math.Basic Use Calc.Types`.
- Multi-file compilation is required for the 0.1 direction (implementation is
  incremental).
- In 0.1 ALL module symbols are public by default. `Export`, interface/body
  separation, aliases, selective imports, and visibility controls are reserved
  for future versions.
- The minimum 0.1 driver resolves imported modules relative to the entry-file
  directory and through the configured standard-library directory.
  `Use Math.Basic` checks `Math.Basic.inox`, then `Math/Basic.inox`, in each
  search root. Dependencies load recursively; cycles are REJECTED; imported
  signatures participate in semantic analysis before textual LLVM IR is emitted.

## CANON-4. BLOCKS AND STATEMENT SYNTAX — THE `:` RULE (was language-reference + syntax + ADR-0002/0005)

THIS IS THE RULE THAT MOST OFTEN TRIPS AIs. READ CAREFULLY.

`;` closes blocks. It is NOT a general statement terminator.
`End`/`end` is not part of Inox syntax and must not be accepted as a block
closer; only `;` closes Inox blocks.

The `:` symbol has ONE meaning in Inox: it DECLARES a function or subroutine AND
opens its body. `:` REPLACES the keyword other languages spell `function`,
`def`, `fn`, `sub`, `proc`, `procedure`, `method`, `defun`. Inox has none of
those words — the `:` IS that word.

    Sum(A Integer, B Integer) Integer :     == the `:` means "this is a
        Return A + B                        ==  subroutine/function; body starts"
    ;

NO CONTROL STRUCTURE USES `:`. Control structures open their body with the
newline after the header (Ruby style) and close with `;` (Ruby's `end` == Inox `;`).

CORRECT (Inox):                    WRONG (the Python/C vice that breaks AIs):
    if Condition                       if Condition :      == ERROR: stray `:`
        ...                                ...
    ;                                  ;

    while I > 0                        while I > 0 :        == ERROR
        ...                                ...
    ;                                  ;

These two are EXACTLY equivalent (the second uses Ruby `end` only to build
intuition; `end` is NOT valid Inox — only `;` closes):
    if Condition          ===          if Condition
        ...                                ...
    ;                                  end

Openers/closers summary:
- Function/subroutine: opens with `:`, closes with `;`.
- if/elif/else, while, for, repeat/until, case/otherwise, with: NO `:`; newline
  opens; `;` closes.
- `Type` section: no `:`, no closing `;` of its own.
- `State :` ... `;` for global mutable state.
- `Const Name := Expr`: single-line module-level declaration.
- `Var` block DOES NOT EXIST. `Var` is a RESERVED keyword the parser MUST REJECT
  with a migration diagnostic. (See CANON-5 / CHANGE LOG v2.0.)

## CANON-5. DECLARATIONS — VARIABLE MODEL (was language-reference §Declarations, CORRECTED to v2.0)

NOTE: the former docs described an OLD `Var` block. That model is REMOVED. What
follows is the corrected, current law.

### Type section
`Type` is a section/declarator, not a block. It has NO `:` and NO closing `;` of
its own.

    Type
        TPoint Struct
            FX Integer
            FY Integer
        ;
        TMonthRange Range 1..12
        TCardSuit (Club, Diamond, Heart, Spade)

`Struct` and multi-line `Enum` open blocks and close with `;`. `Range` and short
`Enum` are line declarations and do NOT use `;`.

### Local variables — three valid forms ONLY
    A := 5            FORM 1 — inferred type. Declares A (Integer), init 5.
    B Integer := 6    FORM 2 — explicit type + value. Declares B, init 6.
    P TPoint          FORM 3 — struct/aggregate, type-default init. Declares P.

Declarations appear INLINE, anywhere in a block, mixed freely with statements.
The OLD `Var ... ;` block is gone.

HARD RULES:
1. First appearance of a name is a DECLARATION; later appearances are ASSIGNMENT.
2. Every variable is born with a well-defined value. No uninitialized state.
3. SCALARS REQUIRE an initializer. `C Integer` (scalar, no `:=`) is a COMPILE
   ERROR: "scalar declaration requires initializer: C".
4. STRUCTS/AGGREGATES may omit `:=`. `P TPoint` is VALID — initialized with the
   type's field defaults. This is type-default init, not an uninitialized var.
5. ENUMS follow STRICT ADA RULES: an enum variable REQUIRES an explicit
   initializer. `Suit TCardSuit` is an ERROR; `Suit TCardSuit := Club` is valid.
6. A BARE IDENTIFIER is NEVER a declaration. `apple` alone is an ERROR. Inox is
   strongly typed (Object Pascal / Ada 2005), NOT Python/JS.
7. ASSIGNMENT TO A NON-EXISTENT NAME is an ERROR. A typo stays a bug.
8. SHADOWING is FORBIDDEN (current or any outer scope; case-insensitive). `:=` to
   a name visible in an OUTER scope is assignment to that outer variable.

Valid assignment to an outer variable:
    Main :
        X Integer := 1
        if true
            X := 2          == assigns the outer X (allowed)
        ;
    ;
Invalid shadowing:
    Main :
        X Integer := 1
        if true
            X Integer := 2  == ERROR: shadows outer X
        ;
    ;

Variables declared inside if/elif/else/while/repeat/for/case-arm/try/except/
finally/with do not escape that block. Use before declaration is an error.

### Safe type inference (Ada/SPARK universal-literal — CHANGE LOG v2.0)
- Integer literal -> `Integer` (Int64). Real literal -> `Float` (Float64).
- Function-call initializer -> the function's return type.
- `5` and `5.0` infer DIFFERENT families; no implicit cross-family coercion.
- Inference picks the MOST GENERAL SAFE family type, NEVER a narrow subtype.
  Restrictions (`Natural`, `Byte`, fixed widths, `Currency`, `Crypto`) only by
  explicit annotation. These are equivalent: `Veritas := 5` and
  `Veritas Integer := 5`.

### Const and State
`Const Name := Expr` declares an immutable constant (single line, inferred type).
Mutable global state must be explicit via `State :` ... `;`. Global mutable state
should be rare and visible. State scalars still require initializers; `mut` inside
State is forbidden.

## CANON-6. FUNCTIONS, SUBROUTINES, RETURN, EXIT (was language-reference + semantics)

    Sum(A Integer, B Integer) Integer :   == function: has a return type
        Return A + B
    ;
    PrintValue(X Integer) :               == subroutine: no return type
        PutLn(X)
    ;
    Main :                                == subroutine, no params, NO parens
        PutLn("hello")
    ;

- `Return Expression` is REQUIRED in functions with return values; FORBIDDEN in
  subroutines.
- `Exit` terminates the current subroutine without a value; allowed ONLY in
  subroutines without return values and in `Main`; FORBIDDEN in functions.
- Falling through the end of a subroutine is allowed.
- Functions must not fall through without returning a value.

## CANON-7. EMPTY PARENTHESES ARE FORBIDDEN (was language-reference + syntax + semantics)

Inox does not use C/Java-style empty parentheses. Parentheses exist only when
there is at least one parameter or argument.

Valid declarations without parameters:   Main :   / PrintReport :
Valid calls without arguments:            PrintReport   /   Account.Print
Invalid:                                  Main() :   /   PrintReport()   /   Account.Print()
Calls/declarations WITH parameters still use parens: Add(A Integer, B Integer) Integer : ; PutLn(42); Account.Deposit(100)

## CANON-8. TYPES AND NUMERIC SEMANTICS (was type-system + language-reference, CORRECTED/EXPANDED to v2.0)

### Canonical type hierarchy (THIS is the authoritative table)
Friendly aliases map to exactly one LLVM-backed base. At most one friendly alias
per width (no Delphi Cardinal/LongWord duplication).

  BOOLEAN
    Bool        -> i1      True/False. (`Boolean` is NOT canonical.)
  INTEGER — friendly aliases
    Integer     -> Int64   (i64)  default integer; target of integer inference
    Natural     -> UInt64  (i64)  floor 0; negatives are a RANGE ERROR (Ada semantics)
    Byte        -> UInt8   (i8)   0..255
  INTEGER — explicit fixed widths
    Int8(i8) Int16(i16) Int32(i32) Int64(i64)
    UInt8(i8) UInt16(i16) UInt32(i32) UInt64(i64)   UInteger -> UInt64
  FLOATING POINT
    Float       -> Float64 (double)  default real; target of real inference
    Float32 (float)   Float64 (double)
    [80-bit Extended EXCLUDED — x86-only, non-portable.]
  EXACT DECIMAL (fixed-point, exact, NO binary rounding) — TO BE IMPLEMENTED in 0.1
    Currency    i64  scaled 10^6   6 decimals   global money
    Crypto      i128 scaled 10^18  18 decimals  crypto assets
    BigCurrency BigInteger scaled 10^6  6 decimals  macro aggregates [0.2+, needs runtime]
  TEXT
    Char        i32   Unicode scalar U+0000..U+10FFFF (excl. surrogates D800..DFFF)
    String      ptr   UTF-8, immutable, non-null, default ""

`Decimal` is NOT a built-in 0.1 type.

Decimal rationale (locked): Currency = Int64 x 10^6 (6 dp covers all ISO 4217
currencies incl. 3-decimal dinars and 4-decimal cases, with margin for
intermediate FX/interest/proration precision; range ~±9.2 trillion; maps to i64;
add/sub direct, mul then /10^6, div by pre-scaling). Crypto = Int128 x 10^18 (ETH
wei needs 18 dp; ETH supply exceeds Int64; i128 native LLVM; one-wei rounding is a
consensus bug). BigCurrency = BigInteger x 10^6 for values exceeding Int64
(sovereign debt, global M2, total market cap); needs heap runtime -> 0.2+. All
decimals: value types; overflow is ERROR; no implicit conversion to/from float;
explicit conversion only.

### Generics use square brackets
    Vector[Integer]   Set[TCardSuit]   Array[1..10] Integer

### Conversions
Implicit conversions allowed ONLY for safe widening explicitly defined by Inox.
Narrowing is NEVER implicit. Explicit conversion uses `TypeName(Expression)`.
`Integer(FloatExpr)` truncates toward zero and traps/errors if out of range.
Constant narrowing with loss is a compile-time error.

### Decimal literal form (CHANGE LOG v3.3 — closes OPEN-2)
A real literal in a DECIMAL-TYPED CONTEXT is read directly as that decimal type,
exact, never through Float; excess decimal places for the type's precision are a
COMPILE-TIME ERROR (Ada model), not silent rounding:
    Preco Currency := 19.99        == 19.99 read directly as Currency (exact)
Without a type context, a real literal infers `Float` (CANON-5). To denote a
decimal value with NO context, use the TYPE CONSTRUCTOR — the same `Tipo(...)`
mechanism used for structs (CANON-9), so construction is ONE concept across the
language:
    X := Currency(19.99)           == constructs a Currency from the literal (exact)
    Y := Crypto(0.000000000000000001)  == constructs a Crypto (18 dp)
    SomaTudo(Currency(19.99))      == decimal value in a context-free position
Two readings of `Currency(...)`, both natural:
  - `Currency(19.99)` with a LITERAL  -> exact construction (no Float in between).
  - `Currency(someFloat)` with a Float EXPRESSION -> the EXPLICIT float->decimal
    conversion (implicit float<->decimal is forbidden; this is the allowed
    explicit form, with documented possible loss/range error).
Excess precision in the literal (more decimals than the type holds) is a
compile-time error, exactly as in the in-context case.

### Operators and numeric law
- Boolean: `and  or  xor  not`.
- Integer bitwise: `bitand  bitor  bitxor  bitnot  shl  shr`. `bitnot` is prefix.
- `^` is exponentiation, never XOR.
- Integer division: `A div B`, `A mod B`. Integer `/` is a COMPILE-TIME ERROR
  with a message directing to `div`. Future `Float` `/` lowers to LLVM `fdiv`.
- Integer overflow is INVALID behavior — not wraparound, not saturation. Constant
  overflow is always a compile-time error. Debug/checking mode should trap at
  runtime. Do NOT emit LLVM `nsw`/`nuw` until checks and optimization policy are
  mature.
- Division by zero for `div`/`mod`:
  - CONSTANT divisor zero (`X := 5 div 0`) is a COMPILE-TIME ERROR (like constant
    overflow). Rejected, as in Ada/SPARK/Delphi.
  - DYNAMIC divisor that becomes zero at runtime (e.g. an iterator passing through
    0, `X := 5 div A`) is a RUNTIME TRAP. The compiler cannot know the value, so
    the program stops with a clear error instead of producing garbage.
- Exponentiation `^` (CHANGE LOG v3.1):
  - On integers, `^` returns an Integer. The exponent MUST be non-negative
    (Ada `**` style). `2 ^ 10` = 1024.
  - Negative integer exponent (`2 ^ -1`) is an ERROR — it would require a
    fractional/float result, and Inox forbids implicit cross-family conversion.
    Use an explicit float (`Float(2) ^ -1`) or a future `Pow` function.
  - Integer `^` overflow is an ERROR/trap, like any integer overflow (ADR-0006).
  - On floats (future), `^` with a real exponent uses the floating definition
    (e^(y*ln x)) and returns a Float.

## CANON-9. STRUCTS AND ASSOCIATED METHODS (was language-reference + type-system + semantics + llvm-backend)

    Type
        TPoint Struct
            FX Integer
            FY Integer
        ;
        TConfig Struct
            FPort Integer := 8080
            FEnabled Bool := true
        ;

Rules:
- `Struct` is a reserved word; opens the struct body; `;` closes it.
- Struct declarations contain FIELDS ONLY. Structs do not declare methods and do
  not repeat method signatures.
- Type names conventionally begin with `T`; fields with `F` (style rules in 0.1,
  not fatal errors).
- Fields may have literal defaults for supported types.
- Structs are nominal VALUE TYPES. Assignment, ordinary parameter passing, and
  ordinary returns COPY the struct value. The backend may use pointers internally
  for associated receivers; this does not change language semantics and does not
  create reference semantics.

Associated methods are declared OUTSIDE the struct:
    TPoint.Move(Self mut, DX Integer, DY Integer) :
        Self.FX := Self.FX + DX
        Self.FY := Self.FY + DY
    ;
    TPoint.Sum(Self) Integer :
        Return Self.FX + Self.FY
    ;

- The receiver type is implied by the `TPoint.` prefix. `Self TPoint` and
  `Self mut TPoint` are NOT canonical (redundant).
- Call-site sugar: `P.Move(3, 7)` and `PutLn(P.Sum)` lower to static associated
  calls. NOT virtual dispatch; no classes/inheritance/interfaces/subtyping.
- `Self` = read-only receiver. `Self mut` = mutable receiver. `Self owned` is
  reserved for future ownership-consuming methods.

### Named struct construction (CHANGE LOG v3.2 — canonical)

A struct value may be constructed inline with NAMED fields using `:=` as the
field/value separator:

    P := TPoint(FX := 10, FY := 20)    == builds a TPoint with FX=10, FY=20

`:=` is used because filling a field IS conceptually an assignment ("FX receives
10"), consistent with `:=` everywhere else; and this keeps `:` exclusive to
function/subroutine declaration (CANON-4). Positional construction (`TPoint(10,
20)`) is NOT canonical: it binds to field ORDER, so reordering fields would
silently mis-fill — unacceptable for mission-critical code. Named construction is
immune to field reordering.

Rules:
- NAMED ONLY. Each argument is `FieldName := Expression`. Positional is invalid.
- ORDER IS FREE: `TPoint(FY := 20, FX := 10)` equals `TPoint(FX := 10, FY := 20)`,
  because each value names its field.
- OMITTED FIELDS: a field with a default uses its default; a SCALAR field with no
  default that is omitted is a COMPILE-TIME ERROR (consistent with "scalars are
  born with a value", CANON-5). A struct-typed field omitted uses its own
  type-default.
      C := TConfig(FPort := 9090)   == OK: FEnabled uses its default true
- DUPLICATE field (`TPoint(FX := 10, FX := 5)`) is a COMPILE-TIME ERROR.
- UNKNOWN field (`TPoint(FZ := 10)`, no such field) is a COMPILE-TIME ERROR.
- EMPTY `TPoint()` is INVALID (empty parentheses are forbidden, CANON-7). To build
  a struct using ALL defaults, use the type-default declaration form `C TPoint`
  (no parentheses). Thus the two creation forms coexist:
      C TPoint                       == all field defaults (no parens)
      C := TPoint(FX := 10, FY := 20)  == specify one or more fields by name
- The constructor is an EXPRESSION: it may appear anywhere a value is expected
  (assignment, argument, return), e.g.
      PutLn(SumPoint(CopyPoint(TPoint(FX := 10, FY := 20))))
  This makes hand-written "MakePoint"-style factory functions unnecessary.

## CANON-10. MUTABILITY AND OWNERSHIP (was language-reference + semantics; ADR-0006)

- Parameters are immutable by default.
- Local variables (inline or formerly Var) are mutable.
- Declaration `Name Type := Expr` vs assignment `Name := Expr` are distinct.
- Shadowing is forbidden in all local scopes (incl. case-only differences).
- Receiver mutation requires `Self mut`.
- `mut X Integer` for ordinary mutable parameters is RESERVED for a future
  version and MUST be a clear error in 0.1.
- Local declaration visible only declaration-point -> end of current block.
- Use before declaration is an error.
- Scoping blocks: subroutine/function body, if/elif/else, while, repeat, for
  body, each case arm, try/except/finally, with body.
- Future ownership work: `Self owned`, `ref X T`, `ref mut X T` — NOT part of the
  0.1 executable subset.

Canonical diagnostics:
  "shadowing is forbidden: Name"
  "loop iterator conflicts with existing symbol: Name"
  "cannot assign to read-only loop iterator: Name"
  "use before declaration: Name"
  "scalar declaration requires initializer: Name"

## CANON-11. `with` STATEMENT (CHANGE LOG v2.2 — Visual Basic dot-prefix model)

`with` is a RESERVED keyword and a control structure. It opens a block bound to
an expression (typically a struct value). NO `:`; newline opens; `;` closes.
Inside the block:
- A DOT-PREFIXED name `.Member` resolves as a member of the `with` expression.
- An UNPREFIXED name resolves by NORMAL scope rules (not a member access).
- Assignment is `:=` (never `=`).

    with theCustomer
        .Name := "Alice Smith"        == theCustomer.Name
        .City := "Seattle"            == theCustomer.City
        .AccountBalance := 250.50     == theCustomer.AccountBalance
        OtherThing                    == normal scope resolution
    ;

NESTED `with`: `.Member` ALWAYS binds to the INNERMOST enclosing `with` (VB rule).
For an outer object use its full name (`Outer.Member`).
NON-SHADOWING ALWAYS PREVAILS: `with` introduces no new symbols; the dot-prefix
makes member access explicit, so it never collides with scope names. The VB
dot-prefix fixes the classic Object Pascal `with` ambiguity.

## CANON-12. CONTROL FLOW (was language-reference + syntax + semantics)

NONE of these use `:` (CANON-4). All close with `;`.

### if / elif / else
    if A > B
        Return A
    elif A = B
        Return 0
    else
        Return B
    ;
No `then`; no `:`; one final `;` closes the whole structure; no `;` between
branches.

### while
    while I > 0
        I := I - 1
    ;
`break` exits the nearest loop; `continue` proceeds to the next iteration.

### repeat / until
`repeat` is a general loop; `until` is an INTERNAL conditional exit, not the
terminator. `until Condition` exits the nearest repeat when true; it may appear
at the beginning, middle, or end, and MORE THAN ONCE. `repeat` closes with `;`.
    repeat
        Work
        until Done
        MoreWork
    ;

### for in range
    for I in A..B
        ...
    ;
    for I in A..B (S)
        ...
    ;
- range endpoints are inclusive; direction comes from `A..B` (A<B ascending,
  A>B descending, A=B executes once);
- step is always positive; step zero/negative is an error if constant, or a
  runtime trap if dynamic;
- `continue` goes to the step/next iteration; `break` exits;
- iterator is declared implicitly, is read-only, visible only inside the loop
  body, and must not conflict with any already visible symbol;
- two SEQUENTIAL `for` loops may reuse the same iterator name after the first
  ends; a NESTED `for` must not reuse an outer loop's iterator name;
- enum ranges are valid in `for`.

### case
    case Suit
        Club
            PutLn("club")
        Diamond
            PutLn("diamond")
        otherwise
            PutLn("other")
    ;
Single-line arms allowed: `Club PutLn("club")`.
- no `of`, `when`, `=>`, `:`, or `do`;
- no fall-through; `otherwise` is optional;
- for `Enum`, a `case` without `otherwise` must be EXHAUSTIVE;
- ranges per arm, multi-values with `|`, and `case` as expression are reserved
  for future versions.

### unless
Negated single-condition guard (parsed; lowering incremental).

## CANON-13. AGGREGATES — ARRAY, VECTOR, RANGE, ENUM, SET (was language-reference + type-system; ADR-0006 #9-15)

### Array
    Values Array[1..10] Integer
    Matrix Array[1..10, 1..10] Integer
    Matrix[I, J] := 42
Fixed-size, bounds-checked, VALUE type. `Low(A)`, `High(A)`, `Length(A)` are
compile-time constants for fixed arrays. Low/High are part of the type. Array
literals are reserved for later.

### Vector (future)
    Items Vector[Integer]
`Vector[T]` is dynamic, 0-based, heap/runtime-managed, bounds-checked, distinct
from `Array`. Semantic direction is OWNERSHIP/MOVE: assignment and by-value
passing move the vector O(1) and invalidate the source. Deep copy requires
`Clone`. No implicit aliasing.

### Range
    Type
        TMonthRange Range 1..12
        TLetterRange Range 'A'..'Z'
`Range` does not open a block and does not close with `;`.

### Enum
Short: `TCardSuit (Club, Diamond, Heart, Spade)`.
Multi-line:
    Type
        TDayOfWeek Enum
            Monday
            Tuesday
            ...
        ;
Enums are nominal and ordinal. NO implicit conversion to/from `Integer`. Values
start at 0 by default. `Ord(E)` returns the ordinal. `TEnum(I)` converts
explicitly with bounds check/trap. Enum ranges are valid in `for`. Enum variable
declarations follow STRICT ADA init (require explicit initializer; see CANON-5).

### Set
    Suits Set[TCardSuit]
`Set[T]` is a finite mathematical set over a nominal ordinal base. `T` must be an
`Enum` or finite `Range`. `Set[Integer]`, `Set[Float]`, `Set[String]` are
INVALID. Sets are value types. Default is empty. Membership uses `in`. Equality
uses `=` and `#`. Subset/superset may use `<=` and `>=`. Canonical operations:
`Union`, `Intersection`, `Difference`, `SymmetricDifference`, `With`, `Without`.
Literal `[A, B, C]` is planned and contextual, but may be deferred. It is NOT a
generic hash set.

## CANON-14. STRINGS AND CHAR (was language-reference + semantics; ADR-0006 #7-8)

`String` is UTF-8, immutable, non-null, with `""` as its zero/default value.
There is no null string. Absence is future `Option[String]`.
0.1 string operations: string literal; local `S String := "..."`; parameter and
return type `String`; `Put`/`PutLn` for strings/literals; byte-by-byte equality
and inequality with `=` and `#`.
Reserved/not implemented in 0.1: `S[I]` indexing; string concatenation;
`ByteLength`, `CharLength`, `GraphemeLength`.
`Char` is a Unicode scalar value — not a byte, not an integer, not a grapheme
cluster; conceptually 32-bit; valid U+0000..U+10FFFF excluding surrogates
U+D800..U+DFFF; literals `'a'`, `'é'`, `'😀'`; surrogate literals are
compile-time errors. Conversions among `Char`, `Byte`, `Integer` are always
explicit.

## CANON-15. ERRORS, EXCEPTIONS, NULL, UNSAFE (was language-reference; ADR-0004/0006)

No universal `null`/`nil`. Normal types are non-null by default. Future absence is
`Option[T]`; future recoverable failure is `Result[T, E]`.
Exceptions exist in 0.1 syntax (lowering incremental):
    try
        ...
    except
        ...
    finally
        ...
    ;
`raise` throws or re-raises. NONE of these use `:`.
The safe 0.1 core has NO raw pointers, NO `Pointer[T]`, NO `unsafe`, NO direct C
interop. Those belong behind explicit future unsafe/interop boundaries.

## CANON-16. CONTRACTS, PROTOCOLS, BEHAVIORS (was language-reference §Contracts)

Future static capability checks. They are NOT Java interfaces, NOT abstract
classes, NOT mixins, NOT duck typing, NOT copied Rust traits. A type satisfies a
contract by providing required operations, likely associated methods.
Satisfaction may be explicit or derivable later, but struct declarations must not
duplicate method signatures.

## CANON-17. I/O — Put / PutLn (was language-reference + runtime §Output)

`Put`/`PutLn` are exposed canonically through `Std.IO`. They accept ONE OR MORE
arguments, Delphi/Object Pascal style, emitted SEQUENTIALLY — this is NOT string
concatenation and does NOT allocate a combined intermediate string. `PutLn`
appends exactly one newline after the final argument.
    Put("J=", J)
    PutLn("Ciclo numero ", J)
    PutLn("A", 10, "B", true)
Input (`Get`/`GetLn`/`ReadLn`) is FUTURE — depends on the I/O ABI. Do NOT use it
in compilable examples.

## CANON-18. STANDARD LIBRARY (was language-reference §Standard library + runtime)

The initial portable 0.1 standard library lives under `stdlib/`:
- `Std.Core` — conceptual prelude/core; anchors fundamental names and compiler
  intrinsics such as future array-bounds operations. Conceptually IMPLICIT.
- `Std.IO` — canonical `Put`/`PutLn` I/O facade. Explicit `Use Std.IO`.
- `Std.Math` — pure Integer helpers implemented in Inox source: `Min`, `Max`,
  `Clamp`, `IsEven`, `IsOdd`. Explicit `Use Std.Math`.
- `Std.Debug` — documents future `Assert`; intentionally UNAVAILABLE until
  trap/abort behavior is canonical.
The standard library must remain portable across Windows and Linux and must not
depend on GC, unsafe features, C interop, or a complex runtime. The current
`stdlib/` is an early layer + documentation anchor, NOT a complete standalone
runtime library.

## CANON-19. RUNTIME MODEL (was canonical/runtime.md)

Three different things must NOT be confused:
  1. The Inox compiler executable (`inox.exe` / `inox`) — currently a native C++
     program. On Windows a Release build may require the MSVC Redistributable. It
     does not require LLVM dynamic libraries merely to start.
  2. The Inox standard library (`stdlib/Std.*.inox`) — beginning of the stdlib,
     not yet a complete runtime library.
  3. The future Inox language runtime (`libinoxrt`, `inoxrt.lib`, or equivalent)
     — future work; may define ABI, startup, traps, allocation, strings, Unicode,
     arrays, vectors, I/O, platform services.

Standard-library discovery order (supports source tree AND prebuilt ZIP):
  1. `INOX_STDLIB`, when set.
  2. `stdlib/` next to the release package root (executable under `bin/`).
  3. `stdlib/` next to the executable.
  4. `stdlib/` under the current working directory.
  5. `stdlib/` in the source file directory or one of its parents.

Build artifact directory: `build/inox-artifacts/`. `INOX_OUTPUT_DIR` overrides.
This is generated output and must NOT be versioned.
Runtime traps/errors include division by zero, bounds errors, range errors, and
future overflow checks in checking mode.
Out of scope for the 0.1 safe core: raw `Pointer[T]`; `unsafe` blocks; direct C
interop; final ABI; complete standalone runtime library; arenas; borrow checker;
deterministic destructors/finalizers; full concurrency runtime.

## CANON-20. LLVM BACKEND (was canonical/llvm-backend.md)

LLVM is the OFFICIAL backend. Current implementation uses TEXTUAL LLVM IR for
incremental validation (readable, testable, accepted by Clang). The compiler must
remain portable C++20; host-specific code belongs in CMake or scripts.
- `div` -> `sdiv`; `mod` -> `srem`. Integer `/` is not valid Inox and must NOT
  lower to `sdiv`.
- Do NOT emit `nsw`/`nuw` until overflow checking and optimization policy are
  proven.
- Structs may lower to LLVM aggregate types. Ordinary struct params/returns are
  values. Associated-method receivers may lower to pointers for convenience —
  not reference semantics.
Current smoke-test backend supports a restricted executable subset: scalar
integer/bool ops, local variables, selected control flow, simple structs,
associated methods, field defaults, struct values, subroutines, temporary
`printf`-based output.
Temporary native driver: `inox --build file.inox` emits textual IR under
`build/inox-artifacts/` and invokes external `clang` to create a native exe;
`inox --run file.inox` builds and executes. `INOX_OUTPUT_DIR` overrides. The
driver loads local `Use` dependencies recursively (checks `A.B.inox` then
`A/B.inox` relative to entry dir and stdlib path), rejects cycles, emits the 0.1
subset as one textual LLVM module — a minimum module model, not a package manager
or final linker. A prebuilt compiler can parse/type-check/emit-IR without LLVM or
Clang installed.
Future backend work: final runtime ABI; richer module linking/exports/visibility/
package search; arrays/enums/ranges/sets/char/strings beyond literals;
checking-mode traps; final toolchain discovery; optional migration from textual
IR to the LLVM C++ API where justified.

## CANON-21. TESTING STRATEGY (was canonical/testing.md)

The test suite is PART OF the Inox language specification. Every parser,
semantic, codegen, runtime, or documentation change must pass the full suite
before commit. Capture regressions as small fixtures in the most specific layer
that exposes the bug.
Layers:
- `examples/*.inox` — small valid programs; double as smoke tests; stay readable.
- `tests/lexer/{valid,invalid}/*.inox` — tokenization (`--dump-tokens`) and
  lexical errors (unterminated literals, invalid chars). Prove spelling,
  normalization, case-insensitivity, literal scanning, diagnostics.
- `tests/parser/{valid,invalid}/*.inox` — syntax (`--parse-only`); must reject
  bad syntax before semantic analysis. Prove `Type` without `;`, inline
  declarations, `if/elif/else` with one final `;`, control structures rejecting
  `:`, `Var` keyword rejected.
- `tests/semantic/{valid,invalid}/*.inox` — name resolution, type checking,
  mutability, flow constraints.
- `tests/codegen/*.inox` — LLVM IR emission; each file registered in both runners
  with explicit required IR fragments.
- `tests/integration/` — end-to-end with expected stdout; when `clang` is
  available, emit IR, link, execute, compare; else report `[SKIP]` without
  failing the frontend suite. Also verifies `Use Std.Math` resolves through the
  stdlib search path and runs via the Clang-backed `--run`.
Portability: tests must not depend on host-specific absolute paths.
Compiler modes used by tests: `--dump-tokens`, `--parse-only`, `--dump-types`,
`--emit-llvm`, `--build`, `--run`. `INOX_OUTPUT_DIR` / `INOX_STDLIB` override paths.
Regression rule: when a bug is fixed, add a fixture that would have failed before,
in the narrowest layer (lexer -> parser -> semantic -> codegen -> integration).

## CANON-22. ROADMAP — 0.1 PRIORITIES (was canonical/roadmap.md §0.1)

1. Keep tests passing on Windows/MSVC and Linux/GCC/Clang.
2. Enforce decided canonical syntax (now: inline declarations + `Var` keyword
   rejected; `Type` without `:`; canonical `Self`/`Self mut`; integer `/`
   rejection; control structures reject `:`).
3. Extend the Clang-backed `--build`/`--run` driver beyond the minimum local
   module workflow.
4. Complete `case` lowering and enum exhaustiveness checks.
5. Extend local multi-file `Module`/`Use` with future export/visibility/package.
6. Implement arrays with `Array[Low..High] T`, indexing, bounds checks, `Low`,
   `High`, `Length`.
7. Implement `Enum`, `Range`, `Ord`, enum-range `for`.
8. Implement `Set[TEnum]`/`Set[TRange]` if time allows.
9. Extend portable `stdlib/` without GC/unsafe/C-interop; define canonical
   trap/abort before implementing `Std.Debug.Assert`.
(Plus v2.0/v2.2 items: remove Var block; implement `with`; Byte alias; Natural/
Currency/Crypto semantics; safe-inference messages; scalar-requires-initializer.)

# ============================================================================
# GROUP: DECISIONS (absorbs docs/decisions/ADR-0001..0006)
# ============================================================================
# These ADRs are the locked decision history. Their content is reflected in the
# CANONICAL group above; they are preserved here verbatim-in-substance for
# traceability. They change ONLY via a new approved ADR (GOVERNANCE G2).

## ADR-0001 — LLVM backend  [SOURCE FILE WAS EMPTY]
Topic intended: choice of LLVM as the official backend. Its substance is now in
CANON-20 (LLVM Backend) and CANON-19 (Runtime). LLVM is the official backend;
textual LLVM IR for 0.1; portable C++20 compiler; external Clang for --build/--run.

## ADR-0002 — Block syntax  [SOURCE FILE WAS EMPTY]
Topic intended: block/statement syntax. Its substance is now in CANON-4 (the `:`
rule), CANON-12 (control flow). `;` closes blocks (not a terminator); functions/
subroutines open with `:`; control structures use no `:`; `End`/`end` is not a
keyword.

## ADR-0003 — Type system  [SOURCE FILE WAS EMPTY]
Topic intended: type system. Its substance is now in CANON-8 (types/numeric) and
CANON-13 (aggregates). Integer=Int64, etc.; exact decimals; no implicit narrowing.

## ADR-0004 — Exceptions  [SOURCE FILE WAS EMPTY]
Topic intended: exception model. Its substance is now in CANON-15. `try`/`except`/
`finally`/`raise` exist in 0.1 syntax; lowering incremental; future Option/Result
complement, not replace.

## ADR-0005 — Consolidated 0.1 Language Decisions  (Status: Accepted)
Context: early decisions were clarified incrementally; consolidated to prevent
drift between human intent, Codex prompts, and implementation.
Non-negotiable rules:
- Inox is post-object-oriented.
- No classes, inheritance, Java-style interfaces, mixins, or duck typing.
- `;` closes blocks and is not a statement terminator.
- `if`/`elif`/`else` do not use `then` or `:` and are closed by one final `;`.
- `Type` is a section/declarator, not a block; no `:` and no closing `;`.
- `TName Struct ... ;` is the canonical struct form.
- Structs declare fields only. Methods are associated outside structs.
- Associated methods use explicit receiver parameters and call-site sugar.
- `repeat` is a general loop; `until Condition` is an internal conditional-exit.
- Compiler stays portable C++20 across Windows/MSVC and Linux/GCC/Clang, future
  Unix portability a goal.
Consequence: agents must update AGENTS.md, canonical docs, grammar, examples,
tests on any change; must not drift toward ObjectPascal/Java/Go/Rust/Python/C++
defaults unless explicitly accepted.

## ADR-0006 — Inox 0.1 Language Constitution  (Status: Accepted)
Context: records foundational decisions so humans, ChatGPT, Codex, and future
agents do not re-open settled matters or import other languages' defaults.
Decisions 1–20 (safe core):
 1. Receivers `Self`, `Self mut`; `Self owned` future. Do not repeat receiver
    type in `Self` (the method prefix supplies it).
 2. Params immutable by default. Locals (inline or formerly Var) mutable.
    `mut X Integer` reserved and rejected in 0.1.
 3. `Exit` only in subroutines without return values and in `Main`. Functions use
    `Return Expression`.
 4. Integer overflow invalid. Constant overflow = compile-time error. Runtime
    overflow should trap in checking mode. No wraparound promise.
 5. Integer `/` invalid; use `div`/`mod`. Future `/` is real division for Float.
 6. Narrowing never implicit. Explicit conversion `TypeName(Expression)`.
 7. `String` UTF-8, immutable, non-null, defaults to `""`.
 8. `Char` Unicode scalar value, not byte/integer/grapheme cluster.
 9. Fixed arrays `Array[Low..High] Type`; ranges part of the type; bounds-checked
    value types.
10. `Vector[T]` future dynamic 0-based owning/move; assignment moves; `Clone` copies.
11. `for I in A..B (S)` inclusive; direction from `A..B`; positive step; iterator
    implicit, read-only, loop-scoped, cannot shadow any visible symbol.
12. `Range` declarations in `Type` are line declarations, no `;`.
13. Enums short and multi-line; nominal and ordinal; no implicit Integer conversion.
14. `Set[T]` requires finite ordinal base (Enum or finite Range); not a hash set.
15. `case Expression` no fall-through; enum cases without `otherwise` exhaustive.
16. `Module` first, no `;`, EOF closes. `Use` semantic dependency, not textual
    inclusion. Multi-file support in 0.1.
17. No public/private/protected/published in 0.1. Future visibility prefers `Export`.
18. Future contracts/protocols/behaviors are static capability checks, not Java
    interfaces, OO subtyping, duck typing, or mixins.
19. No universal `null`/`nil`. Future absence `Option[T]`; failure `Result[T,E]`.
20. No raw pointers, `Pointer[T]`, `unsafe`, or C interop in the 0.1 safe core.
Local scope and shadowing: `Name Type := Expr` declares; `Name := Expr` assigns;
shadowing forbidden in all local scopes incl. case-only differences; local
visible declaration-point -> end of block; use-before-declaration invalid.
Block closing token: `End`/`end` is not a keyword and not a synonym for `;`.
Consequence: agents must not alter these without a new ADR; if implementation
lags spec, document the gap rather than changing the language silently.
Deferred 0.2+: borrowing, arenas, unsafe boundaries, contracts, Chapel-style
parallelism, vector runtime, string/Unicode runtime, module export, variant
structs, metadata tags, full package/build tooling.


# ============================================================================
# GROUP: DEVELOPMENT (absorbs docs/development/toolchain.md + docs/release/*)
# ============================================================================

## DEV-1. TOOLCHAIN REQUIREMENTS (was development/toolchain.md)

Usage profiles:
- Inox compiler developer: needs Clang/LLVM + C++ toolchain. Builds inox(.exe)
  from C++ source.
- Prebuilt-compiler user: needs nothing beyond the Release executable + normal OS
  runtime libraries.
- `--build`/`--run` user: needs a native toolchain (`clang` in PATH) for now; the
  driver delegates final native executable generation to external tools.

Compiler developers — Windows recommended setup: Git for Windows, PowerShell 7,
CMake, Ninja, LLVM/Clang, Visual Studio Build Tools with C++, Windows SDK.
    cmake -S . -B build -G "Ninja Multi-Config" -DCMAKE_CXX_COMPILER=clang++
    cmake --build build --config Debug
    pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
Windows build uses Clang with MSVC ABI target `x86_64-pc-windows-msvc`. VS Build
Tools provide SDK headers, import libs, MSVC STL, MSVC/UCRT runtime, linker.

Compiler developers — Linux/Unix:
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
    cmake --build build
    bash scripts/run-tests.sh

Users testing a prebuilt compiler need no LLVM/Clang/CMake/Ninja/C++ to run:
    inox examples/hello.inox
    inox --parse-only examples/hello.inox
    inox --emit-llvm examples/llvm-put-output-basic.inox
On Windows a Release build may require the MSVC Redistributable. A DEBUG build
must NOT be distributed (depends on non-redistributable Debug DLLs such as
MSVCP140D.dll, VCRUNTIME140D.dll, ucrtbased.dll).

Users building native programs need `clang` in PATH for now:
    inox --build examples/llvm-put-output-basic.inox
    inox --run examples/llvm-put-output-basic.inox
This does not require building the compiler source; the driver delegates final
native codegen and linking to an external platform toolchain. Future SDK work may
package backend/linker tools with Inox or replace this path.

## DEV-2. RELEASE / PREBUILT PACKAGES (was docs/release/*)

Packages (GitHub Release assets):
- inox-windows-x64.zip — https://github.com/fortesm/Inox/releases/latest/download/inox-windows-x64.zip
- inox-linux-x64.zip   — https://github.com/fortesm/Inox/releases/latest/download/inox-linux-x64.zip

Windows quick start:
    Expand-Archive .\inox-windows-x64.zip -DestinationPath C:\Tools
    cd C:\Tools\inox-windows-x64
    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
    .\set-inox-env.ps1
    inox .\examples\hello.inox
Linux quick start:
    unzip inox-linux-x64.zip -d "$HOME/tools"
    cd "$HOME/tools/inox-linux-x64"
    source ./set-inox-env.sh
    inox ./examples/hello.inox
The setup script sets PATH (includes bin/), INOX_STDLIB (-> stdlib/),
INOX_OUTPUT_DIR (-> output/).

Package layout (Windows / Linux analogous):
    inox-<plat>-x64/
        README.md
        set-inox-env.(ps1|sh)
        bin/inox(.exe)
        stdlib/
        examples/
        output/
        docs/ (or manual/index.html)   == bundled documentation
        licenses/
The `stdlib/` directory MUST ship with the compiler (used by `Use Std.*`). The
bundled documentation must be a copy of THIS file (or generated from it). The old
docs/LANGUAGE_REFERENCE.md + docs/index.html are superseded; regenerate any
shipped doc from INOX_CANONICAL.md.


# ============================================================================
# GROUP: FUTURE (absorbs docs/future/* — all were EMPTY files; topics preserved)
# ============================================================================
# These are deferred 0.2+ architectural topics. Per OPEN-QUESTIONS, they are NOT
# permission to invent semantics; each requires an explicit, approved ADR before
# implementation. The source files were empty; their TOPICS are recorded so no
# direction is lost.

## FUTURE-1. CONCURRENCY (was future/concurrency.md [EMPTY])
Direction: Chapel-style structured parallelism and Go-inspired concurrency,
WITHOUT unsafe shared mutable defaults. Data-race safety; non-mutable concurrent
data defaults. Requires ADR before implementation.

## FUTURE-2. CONTRACTS (was future/contracts.md [EMPTY])
Direction: design-by-contract (Eiffel/Sather lineage) as future static capability
checks — preconditions/postconditions/invariants — NOT Java interfaces, NOT OO
subtyping, NOT mixins, NOT duck typing. Requires ADR.

## FUTURE-3. PROTOCOLS / BEHAVIORS (was future/protocols.md [EMPTY])
Direction: a type satisfies a protocol/behavior by providing required operations
(likely associated methods); satisfaction explicit or derivable; struct
declarations must not duplicate method signatures. Static capability checks, not
Rust traits copied verbatim. Requires ADR.

## FUTURE-4. ADVANCED GENERICS (was future/generics-advanced.md [EMPTY])
Direction: generics beyond the current `Vector[T]`/`Set[T]`/`Array[..]` bracket
forms — constraints/bounds tied to contracts/protocols. Requires ADR.

## FUTURE-5. PACKAGE MANAGER (was future/package-manager.md [EMPTY])
Direction: a package/build manager and richer linking beyond the minimum local
`Module`/`Use` driver — exports, visibility, package search, dependency
resolution. Requires ADR.

## FUTURE-6. SYMBOLIC MATH (was future/symbolic-math.md [EMPTY])
Direction: mathematics/CAS direction leveraging Julia-inspired numerical and
symbolic expressiveness for scientific/financial computing. Requires ADR.

## FUTURE-7. OTHER DEFERRED 0.2+ TOPICS (from roadmap + open-questions + ADR-0006)
- ownership/borrow system beyond `Self`/`Self mut`; `Self owned`; `ref`/`ref mut`
  parameters; arena allocation and deterministic memory regions; unsafe
  boundaries, pointers, C interop;
- vector runtime with move semantics; full string/Unicode runtime;
- module visibility/export; interface/body separation;
- variant structs; JSON/DB metadata tags;
- explicit type conversions `TypeName(x)`; multi-variable declaration;
  `Get`/`GetLn`/`ReadLn` input once the I/O ABI exists;
- BigCurrency (+ big-integer runtime).


# ============================================================================
# GROUP: OPEN-QUESTIONS (absorbs docs/open-questions/OPEN_QUESTIONS.md)
# ============================================================================

There are NO blocking language-design questions for the current 0.1 safe-core
path. Decisions 1–20 are consolidated in the CANONICAL and DECISIONS groups above.

Deferred but directionally decided (NOT permission to invent semantics; each
requires an explicit ADR before implementation): ownership/borrow beyond
`Self`/`Self mut`; `Self owned`; `ref`/`ref mut` parameters; arena memory
management; unsafe blocks/`Pointer[T]`/C interop; contracts/protocols/behaviors;
Chapel-style structured parallelism; data-race safety and non-mutable concurrent
data defaults; vector runtime and move semantics; full string/Unicode runtime;
full module export/visibility system; variant structs and metadata tags.

Known implementation conformance gaps (spec may be ahead of the compiler): full
`case` lowering and enum exhaustiveness; module exports/visibility/package search
beyond the minimum local driver; arrays/enum/range/set implementation; vector
implementation; checking-mode overflow traps; final runtime ABI; canonical
trap/abort before `Std.Debug.Assert`.

When closing a gap, update code, tests, this document (Layer B), HTML manual, and
ADRs together.

# ============================================================================
# AGENT RULES (binding behavioral contract — applies across all groups)
# ============================================================================
1. This document is the single source of truth. Read it and the CHANGE LOG
   before any task. It replaces the entire docs/ directory.
2. Obey GOVERNANCE G1–G5. They override convenience.
3. Never reopen ADR-tagged/locked decisions without a new approved ADR.
4. Never edit Layer A to match the code. Code disagreeing with Layer A is a bug.
5. You MAY update Layer B to match the code. You MAY PROPOSE Layer A changes only
   in Section B-PROPOSALS, marked PROPOSAL, never applied as fact.
6. Never infer behavior from another language (G4 / the canonical Agent rule). If
   uncovered, STOP and ask for a language decision.
7. `:` is EXCLUSIVE to function/subroutine declaration. NO control structure
   (if/elif/else/while/for/repeat/until/case/with) uses `:`. (CANON-4.)
8. `End`/`end` never closes a block. Only `;`.
9. No empty parentheses on zero-arg declarations/calls.
10. `Var` block is removed; declarations are inline-only; `Var` keyword is rejected.
11. Scalars and enums REQUIRE initializers; structs may use type-default init;
    bare identifiers and assignment-to-undeclared are errors.
12. Inference picks the most general safe family type; restrictions only by
    annotation; no implicit cross-family or to/from-decimal conversion.
13. Overflow is error, never wraparound (especially Currency/Crypto). No nsw/nuw.
14. No null/nil. No `mut` parameters (reserved).
15. `with` uses VB dot-prefix; `.Member` binds innermost; `:=` for assignment; no
    `:`; non-shadowing prevails.
16. Every language change updates code + tests + this document's Layer B + a dated
    CHANGE LOG entry if it is a design change.
17. For every bug fixed, add a regression fixture in the narrowest layer
    (lexer/parser/semantic/codegen/integration).
18. Keep `dist/` examples in sync with `examples/`. Never commit build/, dist/*.zip.


# ############################################################################
# #              LAYER B — IMPLEMENTATION STATUS (VOLATILE)                   #
# #     True at the date below; may go stale as code advances. An AI MAY     #
# #     update this layer. NEVER treat Layer B as law.                       #
# ############################################################################
# Layer B last verified against source: 2026-06-14

## B-WORKS. WHAT WORKS TODAY (verified in source audit)
Lexer: 43 keywords, case-insensitive normalization, `$XX` hex, `==` comments.
Parser: recursive descent, 2-token lookahead for typed local declarations.
Semantic: scoped symbol table, forward signature pass, inference (empty type +
  initializer -> initializer type), prelude calls (Put/PutLn/Clamp/Min/Max),
  struct/method resolution, loop-iterator read-only enforcement, shadowing
  rejection, integer `/` rejection.
Codegen (textual LLVM IR): integer/bool scalars, locals (alloca/store/load),
  if/elif/else, while, repeat/until, for-range (+step), break/continue, functions,
  subroutines, structs, field defaults, struct values, associated methods,
  Put/PutLn via printf.
Driver: --parse-only, --dump-tokens, --dump-types, --emit-llvm, --build, --run.
Types registered (19): Bool, Int8/16/32/64, UInt8/16/32/64, Natural, Float32/64,
  Currency, Crypto, Char, String + aliases Integer->Int64, UInteger->UInt64,
  Float->Float64.

## B-PARSED. PARSED BUT NOT FULLY LOWERED
- `case`/`otherwise` (AST present; LLVM lowering incomplete; exhaustiveness off).
- `unless` (parsed; not lowered).
- `try`/`except`/`finally`/`raise` (parsed; lowering incomplete).
- Enum short/block forms (parsed; not lowered; strict-init not enforced).

## B-GAPS. CONFORMANCE GAPS (Layer A says it should exist; code doesn't yet)
1. v2 variable model: code still has the Var block path (parseVarStatement,
   VarBlockStatement, SectionKind::Var). MUST be removed; `Var` must become a
   rejected reserved keyword. (Blocks CANON-4/CANON-5.)
2. `with` (CANON-11): NOT implemented — no keyword, no parse, no lowering.
3. Scalar-requires-initializer enforcement (CANON-5 rule 3) — verify/implement.
4. Enum strict init (CANON-5 rule 5) — not enforced.
5. `Byte`->UInt8 alias not yet registered (CANON-8).
6. Natural range semantics (floor 0, negative=error) — name registered, check missing.
7. Currency arithmetic (i64 x10^6) — name registered, behavior missing.
8. Crypto arithmetic (i128 x10^18) — name registered, behavior missing, needs tests.
9. BigCurrency — not present; needs big-integer runtime (0.2+).
10. case LLVM lowering + enum exhaustiveness.
11. Array[Low..High] — not parsed or lowered.
12. Range/Enum/Set full implementation.
13. Checking-mode integer overflow traps.
14. Explicit type conversions `TypeName(x)`.
15. String indexing/concatenation/Unicode runtime.
16. Module exports/visibility.
17. Vector runtime (move semantics, Clone).
18. Final I/O ABI (replace printf); Get/GetLn/ReadLn input.
19. Std.Debug.Assert (needs canonical trap/abort).

## B-EXAMPLES. EXAMPLES STATUS (against current compiler + v2 rules)
Compilable now: hello, empty, llvm-inline-typed-local, llvm-local-variables,
  llvm-local-assignment, llvm-put-output-basic, llvm-putln-integer,
  llvm-struct-basic, llvm-struct-values, llvm-struct-field-defaults,
  llvm-associated-methods, repeat-until, showcase, type-checking, functions,
  bitwise, llvm-function-call.
Need fixing (Get/GetLn/ReadLn not implemented; multi-decl `A, B Integer := 0`;
  function decl inside Main; module-level loose code; wrong fn names; double `;`;
  `1..10(2)` spacing; undefined V; `mut` in State; unimplemented casts):
  llvm-bool-operators, llvm-bool-comparisons, llvm-elif-return, llvm-if-return,
  llvm-if-no-else, llvm-if-merge, llvm-integer-function, llvm-integer-operators,
  llvm-while-loop, llvm-while-break-continue, llvm-repeat-flexible-end,
  llvm-repeat-flexible-middle, llvm-repeat-flexible-start,
  llvm-repeat-break-continue, llvm-subroutine-calls, llvm-for-range-step,
  llvm-for-range-break-continue, control-flow, variables.
Aspirational (parse-only; keep, mark): control-flow, exceptions, types.

## B-CONFLICTS. KNOWN DOC/CODE CONFLICTS TO RESOLVE
- Var block still in code (B-GAPS #1).
- `mut` in State in variables.inox (violates CANON-10).
- Const inline syntax vs parser requiring `:` — adopt line form, update parser.
- Public docs/LANGUAGE_REFERENCE.md + docs/index.html are stale/superseded —
  delete; regenerate any shipped doc from THIS file.
- docs/canonical/type-system.md table was incomplete — superseded by CANON-8.

## B-OPEN. OPEN DESIGN QUESTIONS (0.1 — decided items removed as they close)

These are NOT permission to invent semantics (GOVERNANCE G4). They are tracked
here so they are never lost to a context reset. Each needs a maintainer decision.

OPEN-1 — CLOSED (v3.2). Named struct construction `TPoint(FX := 10, FY := 20)`
  is now canonical law (see CANON-9, Named struct construction).

OPEN-2 — CLOSED (v3.3). In-context: a real literal in a decimal-typed context
  is read directly as that decimal type, exact, excess precision a compile-time
  error (Ada model). Context-free: use the type constructor `Currency(19.99)` /
  `Crypto(0.001)`, coherent with struct construction (CANON-9). See CANON-8,
  "Decimal literal form".

(Reference for the decided items: C# requires the `m` suffix and forbids implicit
float<->decimal conversion; Ada reads decimal literals by context and rejects
excess precision at compile time; Inox follows the Ada model in-context.)

## B-PROPOSALS. AI-SUGGESTED DESIGN CHANGES (NOT law; await approval)
(empty — AIs append PROPOSAL entries here, dated, never inline in Layer A.)


# ============================================================================
# END OF INOX_CANONICAL.md v3.0
# A context-less AI: read HOW TO READ -> GOVERNANCE -> CHANGE LOG -> LAYER A
# (CANONICAL, DECISIONS, DEVELOPMENT, FUTURE, OPEN-QUESTIONS) -> LAYER B.
# Never confuse the layers. The document governs design; the code governs status;
# any mismatch is a bug to report, never to silently resolve.
# ============================================================================
