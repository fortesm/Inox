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
- **Ownership rules (compiler-inserted).** A slot owns one reference. Storing
  into a slot retains the new value, then releases the old one (safe when they
  are equal). Parameters are borrowed: the caller's reference outlives the call
  (parameters are immutable), so passing costs nothing. A result is returned
  owned (+1). A temporary that is not stored is released after its last use.
- **Scope exit.** Each block keeps the list of its String slots (including
  struct fields and array elements, released by element loops). Normal exits
  release them in reverse order. For exceptions, a frame with live String slots
  gets a cleanup: an Itanium landing pad with `cleanup` that releases and
  `resume`s, or an MSVC `cleanuppad` that releases and `cleanupret`s. The
  release order is the same as for normal exits.
- **State.** String State globals are released by an exit routine registered
  at the start of `main`, so the leak checker sees a clean exit.
- **Runtime API.** `__inox_str_retain(ptr)`, `__inox_str_release(ptr)`,
  `__inox_str_equal(ptr, ptr) -> i1`, `__inox_str_put(ptr)`,
  `__inox_str_getln(ptr*) -> i32` (Q1). Allocation failure stops the program
  with a new fault kind 11 or 12, whichever is free after `enum-range-set`
  ("out of memory").
- **Counts.** Non-atomic (Q2).

## Risks / Trade-offs

- Cleanup pads add code to frames with String locals and interact with the
  EH lowering that was hard to get right on MSVC. Mitigation: extend
  `eh_state_balance` to count cleanups, and add Windows CI tests that throw
  through frames holding Strings.
- Reference counting has a cost on every copy; acceptable for immutable values
  and far cheaper than copying bytes. Cycles are impossible (a String holds no
  references).

## Runtime faults

Kind 6 for invalid String input (Q1); one new kind for out of memory.
