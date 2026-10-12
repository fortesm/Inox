## Context

String literals lower to private constants passed to `printf`; no String value
is ever stored. The exception runtime (libinoxrt) already exists on Linux and
Windows, and the emitter already builds landing pads (Itanium) and funclets
(MSVC) for `try`/`except`/`ensure`, verified by `tools/eh_state_balance.py`.

## Decisions

- **Block layout.** `{ i64 count, i64 byteLength, [byteLength+1 x i8] }`, NUL
  terminated for the C runtime. A String value is `ptr` to the block. Count -1
  marks an immortal block (literals and the shared `""`), which retain and
  release skip with one compare, so literal-only programs pay almost nothing.
- **Ownership rules (compiler-inserted).** Every String expression yields a
  *borrowed* pointer, valid until the end of the statement. A slot (local,
  field, element, State) owns one reference: a store retains the new value,
  then releases the old one (safe when they are equal). Parameters are
  borrowed: the caller's reference outlives the call (parameters are
  immutable), so passing costs nothing. A result is returned owned (+1): the
  Return retains its value before the routine releases its slots. A call that
  returns a String gives +1, kept in a statement temporary that is released at
  the end of the statement.
- **When a String is released (changed from the first draft).** Owned slots
  live in the entry block, start as `""`, and are released at every exit of
  the routine (`Return`, `Exit`, the end of the body) and when they are
  overwritten. A declaration inside a loop releases the previous iteration's
  value when it stores the new one. Releasing at the end of every inner block
  as well was dropped: it multiplies the cleanup paths for `leave`,
  `continue`, `until` and `ensure` without freeing anything that is not freed
  at the routine's exit or at the next store.
- **Exceptions (changed from the first draft).** A routine that owns Strings
  gets one cleanup pad built with the exception capture and rethrow already
  used by `try`/`ensure` on both ABIs (Itanium `landingpad`, MSVC
  `catchswitch`/`catchpad`/`catchret`): capture, release the owned slots,
  rethrow. This reuses the lowering validated on the Windows CI (EH-v3.16a)
  instead of adding `cleanuppad` funclets, which no Inox code emits today.
- **State.** String State globals are released at the end of `main`.
- **Runtime API (as built).** `__inox_str_empty() -> ptr`,
  `__inox_str_retain(ptr)`, `__inox_str_release(ptr)` (both skip null and
  immortal blocks), `__inox_str_equal(ptr, ptr) -> i1`, and
  `__inox_str_getln(readByte, status*) -> ptr` (Q1; +1, or the immortal `""`).
  There is no `__inox_str_put`: on Windows the program and libinoxrt may use
  different C runtimes, so console output stays in the program (`printf` with
  `"%.*s"`, length at offset 8, bytes at offset 16) and line input reads
  through the program's own `getchar`, passed as `readByte`. Memory is
  allocated and freed only in libinoxrt. `getln` reports status 0 (ok), 1
  (invalid UTF-8) or 2 (out of memory); the program turns 1 and 2 into
  runtime faults.
- **Statement temporaries.** A call that returns a String (+1) is held in a
  temporary slot of the routine and released at the end of its statement; the
  slot is also an owned slot, so an exception that abandons the statement
  releases it in the cleanup pad, and the next store releases what an
  abandoned statement left behind.
- **Driver.** The program is linked with libinoxrt whenever its IR uses the
  String runtime, not only when it raises (a program comparing two literals
  calls `__inox_str_equal`).
- **Counts.** Non-atomic (Q2).

## Risks / Trade-offs

- Cleanup pads add code to frames with String locals and interact with the
  EH lowering that was hard to get right on MSVC. Mitigation: reuse the
  existing capture/rethrow lowering, and add tests that throw through frames
  holding Strings, run under ASan with leak detection on Linux and natively on
  the Windows CI.
- A String held by a routine that runs long after its last use stays alive
  until the routine exits or the slot is overwritten.
- Reference counting has a cost on every copy; acceptable for immutable values
  and far cheaper than copying bytes. Cycles are impossible (a String holds no
  references).

## Runtime faults

Kind 12 "out of memory" (as planned). Invalid UTF-8 input gets its own kind,
13 "invalid UTF-8 input", instead of kind 6 "invalid integer input" planned
here: the Integer message would mislead. Kind 11 stays reserved for the Range
check of `enum-range-set`. Reported to the maintainer in the pull request.

## Design approval

Approved by Marcelo Fortes on 2026-10-12 ("Então tudo aprovado.", answering
the design summary sent in chat). The two simplifications above ("When a
String is released", "Exceptions") were made after that approval while
studying the existing exception lowering; they are reported to him in the
pull request.
