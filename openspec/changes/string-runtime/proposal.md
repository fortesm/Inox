## Why

`String` is CANON-14 law (UTF-8, immutable, non-null, default `""`), yet the
backend handles only literals passed straight to `PutLn`: a `String` local, a
String field, element or State variable, and String parameters stored in a
variable are all gaps (`string-local` GAP). Every later feature that carries
text — exception messages, file I/O, string input — needs a settled runtime
representation first (CANON-17 defers String input "until the runtime ABI is
settled"). This is item 5 of the order approved on 2026-10-12.

Approval (Marcelo Fortes, 2026-10-12): "Todas as tomadas de decisão dos pontos
1 a 5 foram verificadas e eleitas para serem acatadas." Item 5: String as an
immutable, reference-counted value (Claude's proposal, accepted).

Risk: high (exception cleanup in Itanium landing pads and MSVC funclets, a new runtime ABI, memory ownership). Implementation waits for Marcelo's approval of design.md.

## What Changes

- Runtime representation: a String is a non-null pointer to an immutable,
  reference-counted UTF-8 block in libinoxrt. Literals are static and immortal;
  copies share the block; the block is freed when the last owner releases it.
  The representation is invisible to Inox programs (value semantics).
- The CANON-14 0.1 operations everywhere a value can live: String locals,
  parameters, results, struct fields, array elements, State,
  `ref` parameters, `=`/`#` (byte-by-byte), Put/PutLn.
- Deterministic release on every exit of a scope: normal end, `Return`,
  `Exit`, `leave`, and exception unwinding (cleanup in the landing pads on
  Linux and in the funclets on Windows/MSVC).

Decisions (Marcelo Fortes answered on 2026-10-12, item by item, in his message "Decisões que dependem de mim"); they are now Layer A (canon v3.50):
- **Q1. String input: `GetLn(S)` reads a line into a String variable: yes.**
  The line excludes the newline; invalid UTF-8 is invalid input (kind 6).
- **Q2. Reference counts are non-atomic in 0.1.** The concurrency ADR decides
  atomic counts or per-thread ownership before threads exist.
- **Q3. Concatenation, `ByteLength`, `CharLength` and `GraphemeLength` stay
  reserved**, for a separate proposal with their operator and cost model.
- **Q4. `S String` without `:=` stays an error for locals and State**
  (CANON-5 rule 3); String struct fields without default and String array
  elements start as `""` (rule 4 type defaults).

Out of scope: indexing `S[I]` (reserved), concatenation and lengths (Q3),
Char output and Char-String conversions, `Option<String>`, file I/O, exception
messages.

Behavior that must not change: `PutLn("literal")` output; string equality
semantics; no null string anywhere.

## Capabilities

### New Capabilities
- `language/strings`: the String value type and its 0.1 operations.

### Modified Capabilities
(none)

## Impact

- libinoxrt: `runtime/StringRuntime.cpp` (retain, release, equality, output,
  line input) with C++ unit tests; portable C++, no GC (AGENTS.md stdlib rule
  applies to the runtime too).
- Emitter: String slots, retain/release at copies and scope exits, cleanup in
  the exception paths, destruction of String fields, elements and State at
  program end.
- `tools/eh_state_balance.py` extended to cleanups; ASan leak checking
  (`detect_leaks=1`) added to the sanitizer gate for String tests.
- Canon: CANON-14/17/19 Layer B, change log; docs/site/index.html,
  CHANGELOG.md, docs/BACKEND_GAPS.md.

Depends on `struct-values` and `declaration-sections` for String fields and
String State.
