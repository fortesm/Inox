<!-- SPDX-License-Identifier: MPL-2.0 -->
# Inox revisions

Language version: Inox 0.4.0 pre-alpha
Canon revision: r3.107
Maintainer and sole design authority: Marcelo Fortes
Repository: github.com/fortesm/Inox — license MPL-2.0

The two lines above are what `inox --version` must print (with its BUILD);
the suite checks them. VERSIONING in `spec.md` defines both numbers. This
page is not locked: it is the living record of the constitution. Revision
lines are append-only (G3); open questions leave the list when the
maintainer answers them, and the answer becomes a revision.

## Open questions awaiting the maintainer

None of these is permission to choose (G4); until answered, the current text
stands.

1. **`binary-streams` design** (Risk: high):
   `docs/openspec/changes/binary-streams/design.md`, six decisions for binary
   files (a `BinaryFile` type, little-endian, `Write`/`Read` sized by the
   type, Strings in binary files, reading past the end, 0-based byte
   offsets); streams come after `generics`.
2. **`| |`, `{ }` and annotations** (brainstorm, 2026-10-16):
   `docs/openspec/changes/delimiters-and-annotations/proposal.md` — `| |` as a
   capture clause (`Name`, `Move(X)`, `Link(X)`), `{ }` as an effect or a
   requirement clause, `@` annotations; no String interpolation and `{ }`
   reserved (decided, r3.74).

## Revision log

Entries v1.0 to r3.60 are in `docs/history/canon-changelog.md` (frozen).
Each new line: revision — date — what changed — approval — OpenSpec change.

- r3.61 — 2026-10-12 — The canon becomes this constitution:
  `docs/INOX_CANONICAL.md` is retired (last text: git tag `canon-final`;
  change log in `docs/history/`), its law minimized here (`spec.md`,
  `engineering.md`), locked by `constitution.lock` and
  `tools/constitution_check.py`; the capability specs become law under it;
  stale status, duplicates and superseded text dropped (list in the
  change). Decided by Marcelo Fortes, 2026-10-12: the canon survives,
  minimized and cleaned, inside the OpenSpec constitution, edited to remove
  every inconsistency, incoherence and needless redundancy. Change
  `constitution`.
- r3.62 — 2026-10-12 — Three decisions. (1) `State` has no `:` and keeps
  its closing `;`; the forms `State :`, `Name : Type` and `Const Name : Type`
  are rejected; the constitution records the idea of the syntax and of `:`
  (decided by Marcelo Fortes on 2026-10-12, and on 2026-10-13 that
  State closes with `;`). Change `state-without-colon`.
  (2) `Length` of a String and `Ord` of a Char are rejected, as the law
  already said (confirmed by Marcelo Fortes). Change `char-not-ordinal`. (3) The roadmap is renumbered for
  the version scheme and E22 names no versions (approved by Marcelo
  Fortes). Change `roadmap-renumber`.
- r3.63 — 2026-10-13 — The three open questions answered by Marcelo Fortes
  (1 and 2 as proposed, alternative A for 3). (1) The driver has
  subcommands: `inox file.inox` and `inox run` build and run, `inox build`,
  `inox check`, `inox emit-llvm`, `inox help`, `inox version`; the `--`
  switches stay as aliases. Change `driver-commands`. (2) An exception
  binding is exempt from the unread-local rule only until exceptions have
  readable members. Change `driver-commands` (law text only). (3)
  `Std.Debug.Assert(Condition)` and `Assert(Condition, Message)` exist; a
  failed Assert is a trap, "assertion failed", exit 70, never compiled out.
  Change `debug-assert`.
- r3.64 — 2026-10-13 — Module pairs: `Std.X` holds a subject's types, data
  and fundamental operations, `Std.XUtils` the conveniences built on them
  (and gives everything of `Std.X`). Std.Files holds the File type and the
  basic operations; Std.IOUtils keeps Exists, DeleteFile, RenameFile.
  Std.Strings decisions: `+` concatenates without repeated copies,
  positions count code points from 1, `Length(S)`, `Substr`, `Trim`,
  `LTrim`, `RTrim`, Delphi-style names; `Length(S)` rather than `CharLength`; the
  same split for files, basics in Std.Files and conveniences in Std.IOUtils
  (approved by Marcelo Fortes). Changes `module-pairs` (implemented) and `std-strings`
  (design awaiting approval).
- r3.65 — 2026-10-13 — Two answers and three designs. (1) The `std-strings`
  design is approved by Marcelo Fortes; its implementation may start.
  Change `std-strings`. (2) A function called as a statement (its result
  thrown away) stays as it is: accepted by the analyzer and reported by the
  backend as "not yet implemented" (a backend gap, `docs/BACKEND_GAPS.md`);
  the law adds no rule (decided by Marcelo Fortes). (3) Designs proposed, awaiting
  decisions: changes `generics`, `composition`, `safe-pointers`, and
  `binary-streams` (binary files and streams, which CANON-17 leaves to a
  later change).
- r3.66 — 2026-10-13 — Method reuse. `TName Struct (TPart, ...)` reuses
  structs: each part is a field named after its type and brings its fields
  and methods, at every level; a reused method runs on its part; a name
  reached by two paths, or hidden, is an error at the declaration; a method
  replaces a reused one only with `Override`; no subtyping. Built-in value
  types accept methods (`String.UpdateStr(Self mut, ...)`), and a field of
  `Self mut` may be passed as `ref mut`. CANON-1, CANON-9 and CANON-10 say
  so; FUTURE no longer plans nominal protocols or traits. Conformance fixes:
  `Self` and value parameters cannot be written, `with` follows its target,
  a `Self mut` method needs a writable receiver. The `composition` design
  (embedding, protocols) is replaced: inheritance gives way to the reuse of methods.
  Approved by Marcelo Fortes (decisions 1a, 2a, 3b, 4a+b). Change
  `method-reuse`.
- r3.67 — 2026-10-14 — Design by contract, as in Eiffel (CANON-24): `pre`
  and `pos` on the first lines of a routine (`old X`, `Result`), `invariant`
  in a Struct (with the invariants of the structs it reuses), `pre else` and
  `pos then` in an Override, `invariant` and `variant` in loops. A broken
  contract is a CANON-19 trap; contracts are always checked and only read.
  Differences from Eiffel recorded in the change (trap instead of exception,
  no monitoring levels, invariants after outside writes, messages instead of
  tags). FUTURE now names generics and static proof. Decided by Marcelo
  Fortes: Eiffel's design by contract, with the reserved words `pre` and
  `pos`. Change `design-by-contract`.
- r3.68 — 2026-10-14 — Contracts are checked in every build through 1.0,
  with no switch that turns them off (CANON-24); Eiffel-style monitoring
  levels chosen per module are FUTURE work after 1.0, next to static proof
  (decided by Marcelo Fortes). Change `design-by-contract`.
- r3.69 — 2026-10-14 — Variant structs (a value that is one of several
  structs, examined with `case`) are recorded in FUTURE as the way a
  collection mixes kinds without subtyping; behavior stays written once per
  type through method reuse (`examples/reuse-army.inox`). Recorded at the request of Marcelo Fortes.
  Change `variant-structs` (direction; design after
  `enum-range-set`).
- r3.70 — 2026-10-14 — New FUTURE "Encapsulation, routine types and
  lifecycle": encapsulation from a design close to Object Pascal and Rust,
  still to be designed; routine types (Pascal procedural types) recorded with the
  recommendation of named routines first and closures after safe pointers;
  constructors and destructors left to the maintainer's analysis of
  whether Inox needs them. Recorded at the request of Marcelo Fortes.
  Changes `encapsulation`, `routine-types`,
  `lifecycle` (directions); `docs/OOP_COMPARISON.md`.
- r3.71 — 2026-10-15 — Safe pointers without reference counting, garbage
  collection or arenas: every heap object has one owner (`Own<T>`, created
  by `New`, freed at the end of its block or by `Free`, never copied, given
  away with `Move`), and every other reference is a `Handle<T>` checked by a
  generation on each use (`Alive` tests it; a dead Handle traps; a Handle
  is born empty). Arenas may come later as an optional optimization.
  Decided by Marcelo Fortes: no reference counting (it would repeat
  Delphi's problems), no garbage collection in any form, arenas maybe
  later, and a Handle may be born empty. Change `safe-pointers` (design awaiting approval, no code).
- r3.72 — 2026-10-16 — Safe pointers, completed: a non-owning reference
  may point to any variable, field or array element, not only to an owned
  object; it may go anywhere, and one that outlives its variable is
  detected, never dangling (permissive with detection); it is named
  `Link<T>`, not Handle, to avoid confusion with Delphi's and Windows'
  handles; `Value`, `New` and `Free` keep their names; an absent owner is
  `Option<Own<TNode>>`. Decided by Marcelo Fortes. Change `safe-pointers` (design
  awaiting approval, no code).
- r3.73 — 2026-10-16 — Safe pointers become law (CANON-25: Owners, Moves
  are checked, Links, Access through pointers, Owners in routines and
  contracts; CANON-19 points to the trap categories of CANON-24 and
  CANON-25; FUTURE Ownership updated) and are implemented: `Own<T>`,
  `Option<Own<T>>`, `Link<T>`, `None`, `New`, `Free`, `Move`, `Value`,
  `Link`, `Alive`, `Same`, a definite-move analysis, generation-checked
  Links to owned objects, locals, fields, elements and State, and the
  runtime traps "use of a freed object or an empty Link" and "use of an
  owner after Move or Free". Approved by Marcelo Fortes,
  2026-10-16. Change `safe-pointers`.
- r3.74 — 2026-10-16 — Fewer reserved words (50 → 41): the last arm of a
  `case` is `else`, not `otherwise`; `True`, `False`, `Exit`, `bitand`,
  `bitor`, `bitxor` and `bitnot` are predefined names that keep their
  meaning and can never be declared (free as field and method names);
  `Main` is an ordinary name; `step`, `on` and `mut` stay reserved. `{` and
  `}` are reserved until generics and concurrency show their use. `div` and
  `/` stay as they are; `Float(Sum / N)` as the exact quotient is a FUTURE
  direction. Marcelo Fortes, 2026-10-16. Change `keyword-review`.
- r3.75 — 2026-10-16 — CANON-11 says what `with` does with its target: a
  variable or a value, never a type, evaluated once on entry; `.Member`
  acts on its storage and, through a pointer, is checked at each use. Any
  place is now lowered as a target, and a type as target gets a message
  naming the problem. Approved by Marcelo Fortes, 2026-10-16.
  Change `with-places`.
- r3.76 — 2026-10-16 — Second syntax review: no `Module` header, a file is
  a module named after the file (CANON-3); the conditional expression is
  `IIf(C, A, B)`, only the chosen branch evaluated, replacing `if C then A
  else B` (CANON-12, ADR-0013 superseded); `leave`, `continue` and `IIf` are
  predefined names; `Var`, `Module` and `then` are no longer reserved
  (`then` keeps its meaning in `pos then`); `pre`, `pos`, `old`,
  `invariant`, `variant` and `Override` are reserved (CANON-2). 42 reserved
  words, 10 predefined names. Marcelo Fortes, 2026-10-16. Change
  `syntax-review`.
- r3.77 — 2026-10-17 — Generics in their simplest form (CANON-8, FUTURE):
  a routine or a struct may declare type parameters (`Max<T>(A T, B T) T
  :`, `TPair<K, V> Struct`); each use with its types makes an instance,
  checked and compiled like ordinary code, one copy per list of types. The
  three decisions: no constraints for now (`{ }` stays reserved), type
  arguments inferred from the arguments or the target and never written in
  an expression, and T may be any type, also one that is only moved, with
  the error at the instance ("in Max<TPoint>, used in Main"). Methods of
  generic structs, and invariants and reused parts in them, come later.
  The three decisions approved by Marcelo Fortes, 2026-10-17. Change
  `generics`.
- r3.78 — 2026-10-17 — Std.Strings and Std.StrUtils implemented (CANON-14,
  CANON-18, CANON-19): `+` joins Strings and `+=` appends, in place when
  the String alone holds its block; no implicit conversion to String.
  Std.Strings gives Length, ByteLength, Substr, Position, UpperCase and
  LowerCase; Std.StrUtils adds Trim, LTrim, RTrim, Left, Right, PadLeft,
  PadRight, Replicate, Replace, Contains, StartsWith, EndsWith, SameText and
  CompareText. Positions count code points from 1; case and white space
  follow Unicode. A position outside a String traps with "string index out
  of range" (fault kind 15). `S[I]` and `GraphemeLength` stay reserved.
  `Pos` is a reserved word since r3.76, so the search function is
  `Position(Sub, S)`, approved by Marcelo Fortes, 2026-10-17.
  Design approved in r3.65. Change `std-strings`.
- r3.79 — 2026-10-18 — UInt64 implemented (CANON-8): UInt64/UInteger holds
  0..2^64-1 everywhere an integer lives; arithmetic, power and shifts trap on
  overflow and below 0, `div`, `mod`, `shr` and comparisons are unsigned,
  `case` orders its choices unsigned (with `else`) and `for` reaches 2^64-1;
  conversions into and out of UInt64 are checked (kind 8); Get, PutLn and
  files print and read it unsigned. Q1 to Q3 as decided in r3.50: a literal
  up to 2^64-1 only in a UInt64 context, no unary minus, no mixing without a
  written conversion. A UInt64 Const is written `Const K := UInt64(...)`.
  Decisions 1 to 5 approved by Marcelo Fortes, 2026-10-12. Change `uint64`.
- r3.80 — 2026-10-18 — Enum, Range and Set implemented (CANON-5, CANON-13,
  CANON-19): enums are nominal ordinals with Ord, checked TEnum(I), Low,
  High, checked Succ/Pred, comparison, enum `for` and `case` coverage, and
  print their literal's name; a Range of Integer is a subtype checked when a
  value is stored ("value out of range", kind 11) and computes as Integer;
  Set<T> over an Enum or Range of at most 256 values with `in`, =, #, <=,
  >=, the six functions and the literal `[A, B]`. The decisions of
  2026-10-12 (Q1–Q7, r3.50 and r3.51). Enums in aggregates start at the
  first literal (open question 2). Approved by Marcelo Fortes, 2026-10-12. Change `enum-range-set`.
- r3.81 — 2026-10-18 — Open question 2 answered: the strict rule for every
  array. Every local and State array, whatever its element type, is
  initialized where it is declared; `Fill(Value)` (a predefined name, like
  IIf) is the array its target expects, every element or row a copy of
  Value, evaluated once, following the rules of types (a Range checks it),
  copy and construction; an array of owners is filled with None. An enum
  struct field needs a default; structs keep their field defaults (`P
  TPoint`). The set functions `With`/`Without` become `Include`/`Exclude`
  (the old names are rejected with the new ones); the six set functions
  never change their arguments, so a call as a statement is an error.
  Decided by Marcelo Fortes, 2026-10-18: the strict rule is final, with a
  simple mechanism to initialize arrays; every array, whatever its element
  type, is initialized explicitly where it is declared; `Fill(Value)` fills
  the whole target following the rules of types, copy and construction; the
  existing rule of struct field defaults stays separate; `Include(S, X)` and
  `Exclude(S, X)` replace `With`/`Without`. Array fields of structs stay open
  (question 3). Change `array-fill`.
- r3.82 — 2026-10-18 — Open question 3 answered: initialization is
  transitive over aggregates and type parameters (CANON-5, CANON-9). A
  struct field of an enum, array or type-parameter type has no implicit
  value: it has a default (a literal, or `Fill(Literal)` for an array
  field) or is given in the construction, whose omission is an error. A
  struct takes its type defaults (a declaration without `:=`, State,
  `New(TName)`) only when every component has one, nested structs and
  parts included. A field whose type uses a type parameter never has a
  default, whatever the instance (`D TBox<Integer>` is an error). Scalar,
  String, Set and pointer fields keep their zero, "", empty and None.
  Decided by Marcelo Fortes, 2026-10-18: explicit initialization cannot be
  bypassed because a value sits inside a Struct, an Array or a generic
  type; no generic instance may silently introduce a default the generic
  type does not guarantee; both corrections in one PR, with tests of nested
  structs and generic instances. Change `array-fill`.
- r3.83 — 2026-10-19 — Char values (CANON-14, CANON-5): a Char literal
  takes the escapes of Strings and its quote (`'\n'`, `'\''`), never a
  surrogate; `Char(I)` is checked (kind 8), `Integer(C)` gives the code
  point and `String(C)` the one-character String, with no implicit
  conversion either way; Chars compare by code point and form ranges for
  `for`, `case` and `in`; a Char Range holds Chars (checked stores, kind 11,
  Low and High, sets over it); no arithmetic, `Ord`, `Succ` or `Pred`;
  `CharAt(S, I)` in Std.Strings (kind 15 outside); Put, PutLn and WriteLn
  write the character in UTF-8; a Char field has a default or is given in
  the construction (r3.82); Chars live in locals, parameters, results,
  fields, State, arrays (stored in 32 bits), Consts and generic instances.
  `Get(C)` comes later. The eight recommendations approved by Marcelo Fortes, 2026-10-19.
  Change `char-values`.
- r3.84 — 2026-10-21 — Float conversions and Float Consts (CANON-8, CANON-5):
  `Float(I)` and `Float32(I)` round an integer (a UInt64 read unsigned);
  `Float(G)` widens a Float32 exactly and `Float32(F)` rounds, trapping
  ("value out of range", kind 11) when a finite F is beyond Float32;
  `T(F)` into an integer type truncates toward zero and traps (kind 8) for
  NaN, the infinities and values outside T, as the law already said; a
  conversion into a number takes a number (Bool, String and enum rejected
  with the way to write them); a real literal is a Float32 where a Float32
  is expected; real Consts are folded at compile time, and Std.Math's Pi,
  Tau and E become Consts. Marcelo Fortes approved on 2026-10-19 the plan to do
  the Float conversions and the Float Consts next. The choices marked in the change's proposal await his
  confirmation (open question 3). Change `float-conversions`.
- r3.85 — 2026-10-21 — `case` on a String (CANON-12, ADR-0011): the
  selector may be a String; each choice is a String literal or a String
  Const, compared byte for byte as `=` compares; no ranges; the same String
  twice is an overlap; `else` (Ada's `others`) is always required; a Float
  is never a selector. Approved by Marcelo Fortes, 2026-10-19 (String
  yes with these rules, Float no); `else` is the arm of Ada's `others`
  (r3.74). Change `case-strings`.
- r3.86 — 2026-10-21 — Open question 3 answered, the floating-point policy
  (CANON-8): `Float32(F)` rounds as IEEE 754 (ties to even), a loss of
  precision being no error; a finite F whose rounding overflows stops the
  program (kind 11), never an infinity or a saturation; a constant beyond
  Float32 is a compile error; NaN and the infinities keep their kind, and
  underflow gives a subnormal or a signed zero without error; Float32
  expressions are computed in Float32. A real literal is a Float32 where a
  Float32 is expected, rounded once from its decimal text, never through a
  Float64; a typed value never changes precision implicitly. A Const whose
  value is a real literal (or its negation) is universal, as Ada's named
  numbers and Go's untyped constants; Std.Math's Pi, Tau and E are such
  Consts, written with more digits than either type holds. An unchecked
  conversion to an infinity (`Float32Unchecked`) is an idea, not approved.
  Decided by Marcelo Fortes, 2026-10-21: a checked model for `Float32(F)`,
  inspired by Ada with more deterministic rules; the rule for literals
  confirmed unchanged; the Ada/Go model for constants; no saturation.
  Change `float-policy`.
- r3.87 — 2026-10-21 — `!=` is inequality, the same operator as `#`, both
  valid; a lone `!` is not an operator, negation is `not` (CANON-2).
  Decided by Marcelo Fortes, 2026-10-21. Change `not-equal-operator`.
- r3.88 — 2026-10-21 — The console and Std.Terminal (0.3). E1: the console
  belongs to libinoxrt and every program links it; Put, PutLn, Get, GetLn
  and Std.Terminal reach it only through the runtime; on Windows the
  console is UTF-8 with VT sequences while the program runs (CANON-17,
  CANON-19). E2: a function or method with a result is not a statement, as
  in Ada, with no way to drop a result (CANON-7). F1: `Std.Terminal` —
  RawMode, ReadKey (TKey: Kind, Ch, Ctrl, Alt), ScreenSize, MoveTo,
  ClearScreen, ClearLine, SetColor/ResetColor (16 colors), ShowCursor,
  Flush; ReadKey past the end of input traps "end of input" (CANON-18,
  CANON-19). F2: the terminal is given back however the program ends.
  Approved by Marcelo Fortes, 2026-10-21 (E1, E2 as in Ada, F1 with the
  name Std.Terminal, F2, F3), delivered in separate pull requests. Change
  `console-abi-terminal`.
- r3.89 — 2026-10-21 — Reading characters and reals, Char classes, real
  exponents (0.3). B5: a real literal may end with an exponent after its
  fraction (`1.5e3`, `2.0E-7`); `1e3` is an error; a real literal beyond
  Float64 is a compile error (CANON-2). C1: `Get(C)` reads the next
  character exactly, spaces and line ends included; `EndOfInput`; past the
  end, "end of input" (CANON-17). C2: `Get` reads Float and Float32
  strictly; "invalid number input" is a new trap category (CANON-17,
  CANON-19); Currency and Crypto follow with their values. C3: Std.Strings
  classifies a Char by Unicode (IsLetter, IsDigit, IsSpace, IsUpper,
  IsLower, IsPunctuation) and maps its case (CANON-14). Approved by Marcelo
  Fortes, 2026-10-21. Change `input-chars-exponents`.
- r3.90 — 2026-10-21 — Numbers and text (0.3). B1: `String(N)` and
  `String(F)` write a number as Put does; `FormatFloat(F, Decimals)`
  (Std.Strings). B2: `Integer(S)`, every integer type, `Float(S)` and
  `Float32(S)` read the whole text; other text raises the new exception
  `ConversionError` (CANON-8, CANON-15). B3: a real is written as the
  shortest text that reads back as the same value of its type (`0.1`,
  `2.5`, `1500.0`, `1.0e16`), everywhere a real is written (CANON-17). B4:
  Split and Join move to 0.5 with the collections; `FieldCount(S, Sep)` and
  `Field(S, Sep, I)` (Std.StrUtils) now (CANON-14). Approved by Marcelo
  Fortes, 2026-10-21. Change `numbers-text`.
- r3.91 — 2026-10-22 — Currency and Crypto have values (0.3, CANON-8). A1:
  `*` and `/` of decimals are exact and then rounded half to even to the
  type's places. A2: a literal next to a decimal takes its type (`Price *
  3`); an Integer variable needs `Currency(N)`. A3: `Integer(C)` truncates
  toward zero; Std.Math gives `Round` (half to even) and `Trunc`. A4:
  `Crypto(C)` is exact; `Currency(X)` of a Crypto rounds half to even,
  without error. A5: a decimal is written with at least two decimals and no
  trailing zeros after them. `Get`, `String(C)` and `Currency(S)` read and
  write decimals (B1, B2, C2); "decimal overflow" is a new trap category
  (CANON-19). Approved by Marcelo Fortes, 2026-10-21 and 2026-10-22. Change
  `decimals`.
- r3.92 — 2026-10-22 — Dates, the system and the micro-editor (0.3). D1: a
  TDateTime is the microseconds since 0001-01-01 (proleptic Gregorian),
  with TDate and TDuration. D2: two zones, UTC and the system's. D3:
  Std.DateTime (EncodeDate/EncodeDateTime checked, DecodeDate, Year .. Second,
  DayOfWeek, AddDays, AddSeconds, DaysBetween, IsLeapYear, comparison) and
  Std.DateUtils (ISO 8601, FormatDateTime with Delphi's codes). D4:
  Std.SystemUtils (ParamCount, ParamStr, GetEnv, EnvExists, Halt, Sleep,
  CurrentDir, PlatformName); running other programs after 1.0; `inox run`
  gives the words after the file to the program and keeps its exit status
  (CANON-18, CANON-23). F3: `examples/edit.inox` accepts 0.3. Approved by
  Marcelo Fortes, 2026-10-21 and 2026-10-22 (D1 to D4 with the listed
  Std.SystemUtils functions, F3). Change `datetime-systemutils`.
- r3.93 (2026-10-22) — CANON-7 Explicit discard: `_ := Call` drops the
  result of a function or method call on purpose; E2 and OPEN-4 stay
  compile errors. `_` is a discard target without storage, declaration or
  name; only a call with a result may be discarded, never a literal, a
  variable, a field, an operation, a conversion or a construction. A line at
  module level that is no declaration is now an error (it was ignored).
  Decided by Marcelo Fortes, 2026-10-22: option 2, `_ :=`, at first only for
  calls with a result; E2 and OPEN-4 stay compile errors; a separate PR with
  tests of a lone call, an explicit discard, an unread variable and invalid
  discards. Change `discard-results`.
- r3.94 (2026-10-23) — CANON-4: the migration diagnostic for the old `Var`
  block, `Var X := ...` and a module `Var` section is removed; `Var` is an
  ordinary name and such text fails with the generic message of any
  unexpected text. `mut` stays reserved. Decided by Marcelo Fortes, 2026-10-23: nobody else uses Inox yet, so the
  stable language is born without `Var`; the word behaves as any unknown
  identifier, with the generic message; the rule leaves the canon and the
  OpenSpec documents. Change `remove-var-diagnostic`.
- r3.95 (2026-10-23) — CANON-2, CANON-5, CANON-8, CANON-19, CANON-20,
  CANON-23: `div` is no longer a reserved word; as in Ada, `/` of two
  integers is the integer division, truncated toward zero (`div` before),
  of two reals or two decimals their division, and the operands are never
  mixed without a conversion. `/=` divides an Integer too; a constant zero
  divisor of integer `/` is a compile error; integer `/` lowers to the
  checked sdiv/udiv helpers. `mod` is unchanged. Decided by Marcelo Fortes,
  2026-10-23: `div` is no longer a reserved word, and `/` serves integer
  and real division as in Ada, the operation chosen by the operand types.
  Change `integer-slash`.
- r3.96 (2026-10-23) — CANON-2, CANON-8, CANON-19, CANON-20, CANON-23: as
  in Ada, `mod` has the sign of the divisor (`-7 mod 2` is 1) and the new
  reserved word `rem` the sign of the dividend (`-7 rem 2` is -1), with
  A = (A / B) * B + A rem B; on UInt64 both are the unsigned remainder.
  Decided by Marcelo Fortes, 2026-10-23: `mod` follows Ada, with a new
  `rem`. Change `ada-modulus`.
- r3.97 (2026-10-23) — CANON-8 Checked left shift, CANON-19: `A shl N` is
  the product A * 2^N whenever it fits A's type, on Integer as on UInt64; a
  constant beyond it is a compile error and a run-time one a trap ("integer
  overflow"), never silently lost bits; Int64 `shl` dropped them before.
  A deliberately truncating shift, if it comes, has its own name (such as
  `WrappingShl`). Decided by Marcelo Fortes, 2026-10-23 (option 1): `shl` detects overflow
  on Integer and on UInt64 alike, no left shift ever loses information
  silently, and Integer `shl` is fixed in the same PR as the tests that
  found the problem. Change `expression-checks`.
- r3.98 (2026-10-23) — CANON-23 Optimization, Lowering rules: programs are
  built with clang -O2 by default and `--debug` before the file builds them
  with -O0; optimization never changes what a program does (no nsw/nuw, every
  check an explicit trap). Measured: an arithmetic loop went from 1.99 s to
  0.41 s, the time of C -O2. Optimization chosen in the source for LLVM is a
  later design. Decided by Marcelo Fortes, 2026-10-23: optimization is
  the default; optimizations set in the source for LLVM come later. Change
  `optimize-default`.
- r3.99 — 2026-10-29 — Modules and encapsulation (0.4). Each module has its
  own names: its own first, then the one public name among the modules it
  uses, two of them an ambiguity error (A1); `Bank.Accounts.Fee` reaches a
  public name of a used or re-exported module, and no variable takes a
  module's name (A2); `Use Bank.Accounts as Accounts`, local to its file,
  after which only the alias qualifies (A3); `Use Bank.Accounts.(Deposit,
  Withdraw)`, the list before `as` (A4). Everything is private unless
  `Public`, written last in a header (after the result type and
  `Override`, before `:`) and as the line that opens a struct's public
  fields; `Use` is not transitive; `Export` exists only as `Export Use M`;
  State is never Public (B3); a method is declared in its type's module and
  nothing public exposes something private. Struct encapsulation per
  module (C1), a private field without default makes the type built only by
  its module (C2), reuse across modules sees public members (C3), invariants
  are checked after outside writes to public fields (C4); read-only fields
  later (C5). Search order: the entry directory, `--path`, `INOX_PATH`, the
  standard library (D1); `Std` reserved to the standard library (D2);
  `inox interface` (B4); `examples/bank` is the acceptance program (E2).
  Also: a method argument is analyzed against its parameter type, like a
  routine argument. Decided by Marcelo Fortes on 2026-10-26 (private by
  default with `Public`, `Use` not transitive, `Export` only as `Export
  Use`) and on 2026-10-29 (A1 to A4; `Public` last in a header, after
  `Override`; the header layouts are equivalent, a line break only ending a
  complete one-line statement, as in Ruby; B3, B4; C1 to C4 following Rust
  and Ada/SPARK; C5 deferred; D1, with a future package tool; D2; one PR
  with a commit per stage, E1; the bank of `examples/bank/`, E2). Change
  `modules-encapsulation`.
- r3.100 — 2026-10-29 — The grammar and OpenSpec move under `docs/`:
  `docs/grammar/grammar.ebnf` and `docs/openspec/` (the constitution, the
  capability specs, the changes). `docs/` is OpenSpec's project root, so
  every `openspec` command runs from `docs/`. `config.yaml` becomes valid
  YAML (OpenSpec had been ignoring it). No rule of the language changes.
  Decided by Marcelo Fortes, 2026-10-29 (option A of three). Change
  `docs-layout`.
- r3.101 — 2026-10-29 — Inox becomes 0.4.0 pre-alpha: 0.3 (numbers, text,
  the basic library) was never released apart and ships inside 0.4 with
  modules and encapsulation. Releases follow the version: a push to `main`
  whose version has no tag yet publishes `vMAJOR.MINOR.RELEASE` with the
  Linux and Windows packages, tested as E23 says, as the latest release with
  the stage in its title (the manual's download links follow the latest);
  the build reads its version from Version.h. The
  `v0.1.0-test` release stays as history. 34 completed OpenSpec changes
  are archived (their deltas were already in the specs). Decided by Marcelo Fortes,
  2026-10-29: straight to 0.4, archive the completed changes, publish when
  the version changes on main, keep the old release as history. Change
  `release-0.4`.
- r3.102 — 2026-10-29 — Records state decisions, in English (G2, G3, G7,
  G8): a record says what was decided, who approved it and when; it never
  transcribes a conversation. Every quotation of the maintainer's chat
  messages in the constitution, the specs, the changes, this log, the
  history, the roadmap and code comments becomes the decision it carried,
  in English, with his name and the date; no decision changes. Decided by
  Marcelo Fortes, 2026-10-29. Change `decisions-not-dialogue`.
- r3.103 — 2026-10-30 — Test and example programs and the code
  illustrations of the documents use English names and texts (`TAccount`,
  `FBalance`, `TNode`, `Hearts`); names of people and places and Unicode
  test data stay. No rule changes. Decided by Marcelo Fortes, 2026-10-30.
  Change `english-programs`.
- r3.104 — 2026-10-30 — Repository layout: `tests/ci/`, `src/runtime/`,
  `tools/scripts/`, `docs/BUILD.md`, the legal notices in `licenses/`
  (`LICENSE` stays at the root); `contrib/`, `include/`, `libs/` and
  `utils/` each with a README saying what belongs there; the test runners
  work in a temporary directory. No rule changes. Decided by
  Marcelo Fortes, 2026-10-30. Change `repo-layout`.
- r3.105 — 2026-10-30 — Std.DateTime names its parts in full, in the order
  of ISO 8601: `EncodeDate(Year, Month, Day)`, `EncodeDateTime(Year, Month,
  Day, Hour, Minute, Second)`; the functions that read a part take the
  suffix `Of`, as in Delphi's DateUtils (`YearOf`, `MonthOf`, `DayOf`,
  `HourOf`, `MinuteOf`, `SecondOf`, `MicroSecondOf`), since a parameter
  cannot hide a routine. ISO 8601 is the format of exchange; regional
  formats such as `dd/mm/yyyy` come only from `FormatDateTime`. The open
  question of mixing Range types is recorded. Decided by Marcelo Fortes,
  2026-10-30. Change `datetime-names`.
- r3.106 — 2026-10-30 — Range types (CANON-13), as in Ada: an Integer
  Range is a type of its own (`type T is range`), no longer a subtype. Its
  values combine in arithmetic, comparisons, `in` and IIf with values of the
  same Range and with constants; the result belongs to the Range and is
  checked when stored; `N := M + 1` declares a value of the Range. Two
  Ranges, or a Range and an Integer value, mix only through a written
  conversion (`Integer(X)`, `T(X)`), in expressions, assignments and
  arguments; a Range value indexes an array and is taken as it is by Put,
  PutLn and `String(X)`. Char Ranges are unchanged. Closes open question 3.
  Decided by Marcelo Fortes, 2026-10-30. Change `range-types`.
- r3.107 — 2026-10-30 — The spec deltas of `debug-assert`,
  `design-by-contract`, `method-reuse` and `safe-pointers` are folded into
  the capability specs: new `stdlib/debug`, `language/contracts` and
  `language/pointers`; `language/structs` gains method reuse, `Override`
  and methods of built-in types. Two texts follow later law: `Option<X>` of
  a value waits for `Option<T>` (generics exist since r3.77), and an
  invariant over a public field is checked after a write from another
  module (r3.99). No rule changes. Decided by Marcelo Fortes, 2026-10-30.
  Change `fold-spec-deltas`.
