# Concurrency, SMP parallelism and data safety — architectural study

Exploration for the maintainer's decision (Marcelo Fortes). Nothing here is
law. Revision 2 (2026-10-31) answers the maintainer's review of revision 1:
the Link-after-Move scenario, reads in `forall`, call effects, nested
tasks on a pool, the scope of determinism, the overflow semantics of
reductions, State, File and transitive transferability. Sections 1.6 and
5-12 are new or rewritten.

Revision 3 (2026-10-31) answers the second review: read-only loans while
a Link still reaches the object, transitive immutability and the creation
of Links, the memory model, the limits of compensation, reductions,
errors and cancellation. New probes are 1.7. **Sections 13-20 supersede**
5.3, the Link part of 5.5, the compensation part of 5.8, 5.9, and the
tables of 7 and 11 where they differ.

Revision 4 (2026-10-31) makes the adjustments of the third review only:
the three forms of transfer, the re-keying as a rule of its own, the
wording of D5, and the measured impact of D8 on the repository. Sections
21-25 are new; section 25 consolidates the study. No new feature. Spellings such as `forall` are working names, not grammar. Each claim
is marked where it matters: **[code]** observed in the repository,
**[law]** written in the constitution or a spec, **[source]** stated by
another language's documentation, **[analysis]** reasoning of this study,
**[hypothesis]** to be validated by a prototype.

## 1. Diagnosis: what Inox has today

### 1.1 Works today (implemented and tested)

| Capability | Where | Relevance |
|---|---|---|
| Structs and arrays are values, copied on assignment | CANON-9, `language/structs`, `language/fixed-arrays` | A copy given to a task shares nothing **[law]** |
| `Own<T>`: one owner per heap object, never copied, `Move(P)`, freed at every exit | `semantic/SemanticPointers.cpp`, `runtime/OwnRuntime.cpp`, `language/pointers` | Transfer of ownership already exists **[code]** |
| Move analysis: use after `Move`/`Free` is a compile error on every path except `leave`/`continue` (run-time backstop, trap 17) | `SemanticPointers.cpp`, `SemanticStatements.cpp`; follow-up 4 | A flow-sensitive dataflow already exists and can be extended **[code]** |
| `Link<T>`: copied freely, checked by a 64-bit generation on every use | `OwnRuntime.cpp` (block header), `FunctionEmitterPointers.cpp` | The **only** stored, shareable, mutable alias in the language **[code]** |
| `ref`/`ref mut` parameters live only during the call; a `ref mut` argument's root variable appears in no other argument (`Swap(A[1], A[2])` is rejected) | `SemanticCalls.cpp` `checkReferenceArguments`, ADR-0016, `language/ref-parameters` | Exclusivity by root variable, conservative and cheap **[code]** |
| Slices are read-only, never stored, returned or reassigned | ADR-0017, `language/slices` | A slice never outlives its array and never writes **[law]** |
| Strings are immutable blocks with a **non-atomic** reference count; literals are immortal | `runtime/StringRuntime.cpp` header, `docs/STATUS.md` | Sharing a String between threads races on the count **[code]** |
| `State`: a module's mutable globals, initialized before `Main` | CANON-5 State, `language/module-state` | The second source of shared mutable memory **[law]** |
| Generics: each use is a checked instance; no constraints | `SemanticGenerics.cpp`, CANON-8 | A property such as "transferable" can be checked per instance, with no trait **[code]** |
| Exceptions: C++ exceptions thrown by libinoxrt, caught by pads (Itanium and MSVC) | `codegen/ExceptionAbi.h`, `runtime/ExceptionRuntime.cpp` | Per-thread by nature; crossing a task needs capture and rethrow **[code]** |
| Traps stop the process with exit status 70 (`_exit`) | `codegen/RuntimeHelpersIr.cpp`, CANON-19 | A trap in any task stops the program: already consistent **[code]** |
| Contracts lowered on the AST (hidden `__Name_body` routines) | `semantic/ContractLowering.h` | Precedent for rewriting a construct into hidden routines before analysis **[code]** |
| No closures, no routine values | `routine-types` (direction only) | A parallel body today can only be a block, never a captured function **[code]** |

### 1.2 Not thread-safe in the runtime today **[code]**

- `OwnRuntime.cpp`: the free lists (a global map of vectors), the queue of
  pending frees and the `freeing` flag are process globals.
- `StringRuntime.cpp`, `FileRuntime.cpp`: reference counts are plain
  integers.
- `ConsoleRuntime.cpp`: the input buffer (`gInput`, `gInputStart`,
  `gInputEnd`), raw mode and colour state are globals.
- `SystemRuntime.cpp`: `gArguments` is written once before `Main` (safe if
  only read afterwards).

### 1.3 Only specified, or partial

- FUTURE Concurrency: `do` is reserved (tests/diagnostics/do-reserved.inox);
  nothing else exists **[law]**.
- `Option<T>` for any T, `Result<T, E>`, `Vector<T>`, methods of generic
  structs: planned, not implemented **[law]**.
- `safe-pointers-followups`: Links to parameters, zero-cost Links, release
  per loop iteration, invariants through pointers (open) **[code]**.

### 1.4 Not designed

`routine-types` and closures, `lifecycle` (constructors, destructors),
`Self owned`, the `| |` capture clause and the `{ }` effect clause
(`delimiters-and-annotations`, brainstorm only), FFI.

### 1.5 The decisive observation **[analysis]**

(Kept from revision 1; section 1.6 corrects it.)

Inox already removed most aliasing. A value reaches another name only
through:

1. `Link<T>` (stored, copied, mutable through it);
2. `State` (global, mutable);
3. the count inside a String or File block (hidden mutation);
4. `ref`/`ref mut` and slices, which cannot outlive a call or a block;
5. owners, which are unique by construction.

Rust needs lifetimes because any reference can be stored; Swift needs
regions because any class instance can be shared. Inox needs neither if
it keeps (4) scoped and treats (1), (2) and (3) explicitly. This is the
basis of the recommendation: **the minimal rule set is small because the
language already is.**

### 1.6 Probes run on the current compiler (revision 2) **[code]**

Small programs compiled with today's compiler (no change to it):

1. **A Link survives the Move of its owner.** `L := Link(P)`, then
   `Q := Move(P)`, `Q.FValue := 7`: `L.FValue` prints 7 and `Alive(L)` is
   true; after the new owner frees the object, `Alive(L)` is false. This is
   CANON-25 as designed (a Link follows the object, not the owner). Under
   concurrency it is a real hole: moving `Q` to another task leaves `L`
   in the parent reaching the same memory. Forbidding Links to cross tasks
   does not close it.
2. **ref mut does not see State.** `Bump(ref mut A)` called as `Bump(S)`
   with S in State, and Bump reads `S[1]` directly: it sees its own write
   through A. ADR-0016 rejects the same variable in two arguments, not an
   argument that is also reachable as State by the callee. Harmless while
   sequential (defined result); an effect analysis must count a write
   through a `ref mut` rooted at State as a write of that State.
3. **A File is unique.** It cannot be assigned, copied, passed by value,
   returned, stored in a field, an array or State (`stdlib/files`); its
   count only follows moves inside the runtime (`FileRuntime.cpp`). One
   variable holds it, and callees reach it only by `ref mut`.
4. **Strings change in place when unique.** `StringFunctions.cpp` appends
   into the block when `count == 1` and there is room. With atomic counts
   this stays correct only if no other task reads the block without
   holding a count: the read-only rule of 5.3 guarantees it.
5. **Floating point is plain IEEE.** The emitted IR carries `fadd`/`fmul`
   without fast-math or contraction flags; programs are built with `-O2`
   by clang from that IR.
6. **The call graph is exact.** Calls resolve statically (methods are
   static calls, CANON-1; no routine values yet); every call is resolved to
   a signature during analysis, and State symbols have their own kind
   (`Symbol.h`, `SymbolKind::State`). The analyzer does not keep the call
   graph today; recording its edges is a small addition.
7. **State is module-private.** State is never Public (r3.99), so every
   write to a State variable, every `Link` to it and every `ref mut` of it
   is in its own module.

### 1.7 Probes of revision 3 **[code]**

8. **`ref` is not read-only through owners.** `Poke(ref P Own<TNode>)`
   writes `P.FValue := 1` and the caller sees 1. With
   `Poke(ref H THolder)` and `FChild Own<TNode>`, `H.FChild.FValue := 1`
   is accepted too.
9. **A read-only receiver writes through its owners.** `THolder.Poke(Self)`
   (not `Self mut`) writes `Self.FChild.FValue := 1`: accepted.
10. **Links are made from read-only places.** `Link(H.FChild)` in a routine
    with `ref H`, `Link(Self.FChild)` in a `Self` method and `Link(P)` with
    `ref P Own<TNode>` are accepted, and writing through them changes the
    caller's object. (Only a Link to a *value* parameter is rejected.)
11. **State may hold an owner** (`State G Own<TNode> := New(TNode)`), and
    its object is written as `G.FValue := 4`.

So the read-only of `ref` and `Self` (ADR-0016, CANON-9) is shallow today:
it protects the variable and its plain fields, not the objects its owners
own. Sequentially the results are defined; for concurrency this is the
gap the review predicted.

## 2. What the other languages teach

### Chapel **[source]**
- `forall` runs iterations possibly concurrently, and must be
  serializable; the leading iterator decides the number of tasks
  (`dataParTasksPerLocale`, `dataParMinGranularity`).
- Outer variables enter as shadow variables. Scalars are captured by
  value; since Chapel 2.0 an outer **mutable array defaults to `ref`**
  (an explicit `with (ref A)` is no longer needed). Writing a shared
  scalar or array from several iterations is therefore possible and
  unchecked: safety is partly the programmer's.
- Reduce intents (`with (+ reduce Sum)`) give each task a private
  accumulator combined into the parent.
- Lesson: the expressive core is small (a parallel loop over an index
  set, private copies, reductions, a runtime that partitions). The
  fragility comes from default `ref` access to outer arrays.

### Ada 2022 (added: closest to Inox's spirit) **[source]**
- `parallel for` and `parallel do` blocks, with a Conflict Check Policy.
  The default, `All_Parallel_Conflict_Checks`, rejects known conflicts
  and forbids a parallel construct from reading or updating a variable
  global to it, except synchronized objects and **indexed accesses of
  the loop** (`A(I)` inside `parallel for I`). `Parallel_Calls` marks a
  subprogram safe to call in parallel.
- Lesson: local, conservative, compile-time rules give a real guarantee
  for data parallelism without a general alias analysis. Ada's default is
  stricter than revision 1 assumed: a parallel construct may not even
  **read** a variable global to it. Inox can allow reads of the parent's
  locals only because of the structure argument of 5.2; that argument,
  not Ada, carries the proof.

### Rust **[source]**
- `Send` (may move to another thread) and `Sync` (may be shared by
  reference) are auto traits computed from the structure of a type;
  `Rc` is neither, `Arc` uses atomic counts. Scoped threads
  (`std::thread::scope`) let tasks borrow the parent's data because the
  scope joins them. rayon: `join`, `par_iter`, disjoint `chunks_mut`.
- Lesson: (a) a structural, computed property of types is enough; Inox
  computes it per instance without a trait. (b) Structured scopes make
  read-only borrowing by tasks safe without lifetimes, because the parent
  outlives its tasks.

### Go **[source]**
- Goroutines multiplexed on threads (M:N), channels, `select`; the memory
  model guarantees DRF-SC, but the language does not prevent races (a
  race detector finds them at run time), and races on multiword values
  (slices, strings, maps) can corrupt memory.
- Lesson: the operational model (cheap blocking tasks, channels) serves
  servers well; its safety model is not one to copy.

### Swift **[source]**
- `Sendable` values may cross isolation domains; structured concurrency
  (`async let`, task groups) ties child tasks to a scope; region-based
  isolation (SE-0414) is a flow-sensitive check that lets a non-Sendable
  value move to another domain when its region is disconnected and it is
  not used afterwards; `sending` (SE-0430) marks such parameters and
  results; actors isolate mutable state.
- Lesson: "transfer if never used again" is exactly CANON-25 `Move`.
  Inox has disconnected regions for free: an owner and the objects it
  owns form one, and Links are the only edges out of it.

### Delphi PPL **[source]**
- `TParallel.For`, `TTask`/`ITask`, `IFuture<T>`, `TThreadPool` (sized
  by CPU load), `EAggregateException`; `TThread.Synchronize`/`Queue`
  marshal work to the UI thread, because the VCL is not thread-safe.
- Lesson: the programmer's vocabulary is right (parallel for, task,
  future, a pool, a way back to the UI thread); the safety is absent.

### Zig **[source]**
- `async` was removed and returns (0.16) as a `std.Io` interface chosen
  like an allocator: single-threaded, thread pool or evented (io_uring);
  `io.async` (may run concurrently) differs from `io.concurrent` (must),
  which fails instead of deadlocking.
- Lesson: keep the scheduler an implementation choice behind a stable
  interface; distinguish "may run in parallel" from "must run
  concurrently".

### A unified reading of `forall`, `TParallel.For`, `parallel for` and `par_iter` **[analysis]**
All four are the same construct: an index set, a body run once per index,
a runtime that partitions the set into chunks on a pool, a join at the
end. They differ only in what the body may touch: Chapel and Delphi let
it touch anything; Ada and rayon restrict it (Ada by conflict checks,
rayon by `Send`/`Sync` and disjoint chunks). One Inox construct can be
all four, with Ada's restriction.

## 3. The guarantees to state, and to keep apart **[analysis]**

| Property | Can Inox promise it? | How |
|---|---|---|
| Memory safety | Yes (today, single-threaded) | CANON-25, checked indexing |
| No data races in the safe subset | Yes, if the rules of §5 hold | Static rules; runtime primitives trusted |
| Sequential consistency for race-free programs (DRF-SC) | Yes | Follows from the previous line plus acquire/release at spawn, join and channel operations |
| Determinism of results | Yes, within a stated scope (5.6) | Fixed partition, ordered combine; same binary and inputs |
| No deadlocks | No in general; yes for `forall` and structured tasks without channels | Structure; channels and locks can deadlock |
| Logical correctness (no lost update, right order) | No | Contracts, transactions, design |
| Atomicity, transactional isolation, persisted integrity | No (database and library concerns) | Libraries, DBO |

The promise must be worded as: *a program that uses only safe Inox and the
trusted library cannot have a data race*. FFI calls and any future
unchecked construct are outside it, named as such.

## 4. Alternatives

- **A — Ownership and isolation.** Tasks get values by copy, owners by
  `Move`, and read-only access to the parent's variables while the parent
  waits. No shared mutation at all.
- **B — Isolation by tasks and messages.** Mutable state belongs to one
  task; others send messages (channels, an actor-like loop). The GUI
  thread is such a task.
- **C — Synchronized types.** Shared values change only through types
  whose operations synchronize (counters, a protected value, concurrent
  queues), as Ada protected objects or Rust `Mutex<T>`.
- **D — Minimal hybrid.** A as the default and only language rule; B and C
  as library types, which are the only way to share mutable state.

| Criterion | A | B | C | D |
|---|---|---|---|---|
| Data races prevented | Yes, statically | Yes, if messages are transferable | Yes inside the types; only as good as their API | Yes: A statically, B and C by trusted types |
| Compiler complexity | Low: extend move analysis, a "transferable" property, local rules for `forall` | Low (library) | Low (library), plus a way to pass the update (routine types) | Low to medium |
| Runtime complexity | Pool, join, partition | Queues, blocking, wake-up | Locks or atomics | Sum of the three, all small |
| Language simplicity | High: no new concept besides the construct | Medium | Medium | High: one rule, two library families |
| Ease of use | Excellent for batch and math; insufficient for servers and GUIs | Natural for servers, pipelines, GUI | Natural for counters, caches, totals | Covers the nine scenarios of §6 |
| Performance | Best (no locks); copies where values are large unless owners are moved | Message cost | Contention under load | Choose per case |
| SMP scalability | Excellent | Good | Limited by contention | Good |
| Fit with Inox | Direct (CANON-25, value semantics) | Needs generic struct methods for `Channel<T>` | Needs routine types or a `protected`-like construct | Direct |
| Evolution | Open to B and C later | Open | Open | Open |

## 5. Recommendation: D, built on A (revision 2)

### 5.1 Transferable, transitively **[analysis]**
A value is *transferable* when, once given to another task, no memory it
reaches can be reached from the task that gave it. Per type:

| Type | Transferable | Why |
|---|---|---|
| Integer kinds, Float kinds, Bool, Char, enums, Ranges, Sets, Currency, Crypto | Yes | Plain bits, copied |
| String | Yes, once counts are atomic | Immutable; in-place append only when the count is 1 (1.6.4) |
| Struct, fixed array | Yes if every component is | Values, copied |
| `Own<T>`, `Option<Own<T>>` | Yes if T is, by `Move` only, with the re-keying of 5.2 | Unique owner; outer Links cut at the transfer |
| `Link<T>`, anything holding one | No | A Link is an alias by definition |
| Slice | No | Bound to its block (ADR-0017) |
| File | Lent exclusively (5.7), never shared | Unique by the language (1.6.3) |

"Every component" is structural and recursive, so a transferable value
holds no Link at any depth: no mutable memory is reachable from inside it
except through owners it carries, which 5.2 isolates. Generics check it
per instance (CANON-8 unchanged).

### 5.2 Closing the Link-after-Move hole **[analysis]**
Probe 1.6.1 shows the hole is real. Options:

- **Static only.** Reject the Move of an owner into a task when a Link to
  its object may exist. Links can be stored anywhere (fields, arrays,
  State, other objects), possibly by a caller, so this cannot be proved
  locally; it would need the global alias analysis the study must avoid.
- **Re-keying at the transfer (recommended).** When an owner crosses into
  a task (argument, capture, future result, channel send), the runtime
  increments the generation of its block and of every block it owns. Every
  Link made before the transfer then behaves as a Link to a freed object:
  `Alive` is false and a use traps (`use of a freed object or an empty
  Link`), exactly as CANON-25 already defines it. No lifetimes, no new
  concept. Internal Links of the moved graph would also die, but a
  transferable type has none (5.1), so nothing legitimate breaks.
- **Cost.** Proportional to the number of blocks moved; the walk is the
  same as the drop walk the compiler already generates per type, with
  "increment" in place of "free". It can be skipped, as a whole-program
  fact, for types that are never the target of `Link(...)` anywhere in
  the program.
- **Consequence.** A use of a stale Link in the parent is caught at run
  time, not at compile time. It is never a data race: the generation
  check happens before any access, and the header is only written by the
  transfer, which happens-before the task starts (5.8).

### 5.3 Structured scopes and the read-only rule **[analysis]**
Every task belongs to a scope that waits for it. While a task or a
`forall` body may read a variable of the parent, the parent may not write
it, `Move` it, `Free` it, pass it as `ref mut` or take a Link to it. Sibling
tasks cannot write it either (they see it read-only). So reads of the
parent's variables never meet a write. This, not Ada's policy, is the
argument that allows reads Ada forbids.

### 5.4 `forall` (working name): all accesses, not only the target **[analysis]**
Revision 1 restricted the write target; `A[I] := A[I - 1]` showed that is
not enough. Revision 2 restricts every access to a written array:

1. **Index set.** A `forall` runs over a range `L..H` (with `step`), an
   array's index set, a Range type, an enum or a Set. Each yields distinct
   values by construction; Inox has no user iterators, so a repeated index
   cannot occur. A user iterator, if one ever exists, is not allowed.
2. **Written arrays.** An outer array written anywhere in the body (by
   assignment, `ref mut` argument or `Self mut` call on an element) may be
   accessed in the body **only** at the loop index, read or write: every
   subscript list that reaches it has the loop index, unmodified, at one
   fixed position (`A[I]`, `A[I].FPay`, `M[I, J]` for any J). `A[I - 1]`,
   `A[J]`, `A[F(I)]`, a slice of A, A passed whole to a call and `with A`
   are rejected while A is written. `Low(A)`, `High(A)` and `Length(A)`
   stay allowed: the bounds of an array never change.
3. **Other outer variables** are read-only (5.3) and may be read anywhere.
4. **Body locals and the index** are private to the iteration.
5. **Accumulation** only through declared reductions (5.6).
6. **Calls** follow 5.5: a written array reaches a callee only as the
   element `A[I]`; a whole outer variable reaches it only by value or
   `ref` (read).
7. **Nested loops.** For each `forall`, rule 2 applies with its own index:
   `M[I, J]` written in an inner `forall J` inside `forall I` satisfies
   both. A sequential `for J` inside `forall I` may use any J at the
   non-index positions.
8. **Distinct names are distinct storage.** Arrays are values. Inside a
   routine two names reach one storage only through `ref` parameters
   (two `ref`, both read-only: harmless), State through a `ref mut`
   (1.6.2: handled by 5.5 and D3), or Links (outer Links unusable in a
   body). So rule 2 on names is a rule on memory.

Every rule is local to the loop plus the callees' summaries.
**[hypothesis]** payroll and indicator loops written as "compute row I from
inputs, write row I" fit; loops that read neighbours of a written array
(stencils) must write a second array instead (`B[I] := A[I - 1] + A[I]`),
which rule 2 accepts because A is not written.

### 5.5 Effects of calls, conservative and exact **[analysis]**
Without closures, routine values or FFI, a routine can change only:
its locals; its `ref mut` parameters and `Self mut` receiver; State; the
objects of owners it holds; memory reached through Links; Files (through
`ref mut`); the console; and String counts (runtime). That list is
complete for today's language, so a summary per routine needs only:

| Fact | Computed from |
|---|---|
| reads mutable State / writes State (which variables) | State symbol uses, transitively; a `ref mut` argument rooted at State counts as a write of it (1.6.2) |
| writes through a `ref mut` parameter / `Self mut` | parameter modes (already in the signature) |
| derefs a Link not created in its own call tree | Link uses whose origin is a parameter or State |
| console or file I/O | calls to the built-ins |
| calls FFI | future: declared, never inferred |

The call graph is exact (1.6.6); recursion is a fixed point; generic
instances and contract wrappers are routines like the others. A body may
call a routine whose summary has no State access (except frozen State,
D3) and no Link deref of outer origin; `ref mut` arguments must be body
locals or the iteration's `A[I]`; console output is allowed (the runtime
serializes lines; ordering between iterations is unspecified, a logical
race, not a data race). Routine types, when they come, must carry their
summary in the type, or be refused in parallel code.

### 5.6 Reductions: a new operation, not a faster loop **[analysis]**
A reduction is specified separately from a sequence of checked
additions, so CANON-8/CANON-19 overflow rules of ordinary arithmetic do
not change:

- **Sum of Integer kinds and Currency:** the result is the exact
  mathematical sum, which traps if and only if it does not fit the type.
  Exactness by 128-bit partials holds for Int64 and Currency values when
  the count is below 2^64 (|sum| < 2^63 · 2^64). UInt64 likewise.
- **Sum of Crypto (Int128 scaled 10^18):** 128-bit partials are not
  enough; exact needs 192-bit partials (runtime, three 64-bit limbs).
  Alternative: checked 128-bit partials over the fixed tree, which
  reproduces but may trap where the exact total fits. Decision D7b.
- **Product:** an exact product is unbounded; defined as checked
  multiplication over the fixed tree of 5.7 (reproducible, may differ
  from a sequential loop in whether it traps). Min, Max, Count, And, Or:
  exact and order-free.
- **Float sum and product:** IEEE rounding at each step over the fixed
  tree: reproducible within the scope below, different from a
  left-to-right loop, and documented as such.
- **Scope of reproducibility (Float):** the same compiled program with the
  same inputs gives the same reduction result whatever the number of
  cores, the scheduling and the run. Across machines it holds when both
  implement IEEE 754 binary32/binary64 with round-to-nearest for basic
  operations (x86-64 SSE2, AArch64), the generated code has no
  fast-math or contraction (1.6.5; the compiler must keep it so), the
  partition algorithm is the same (same compiler version), and the
  element values themselves are equal: results of library functions
  (`Sin`, `Exp`, ...) may differ between C libraries.
- **Fixed partition:** chunk boundaries depend on the size of the index
  set only; partials combine in a fixed tree in index order. Load
  balancing moves chunks between threads, never changes the tree.

### 5.7 Files and other resources **[analysis]**
A File is already unique (1.6.3). It can be **lent** to one task like a
`ref mut` argument for the task's whole life: the parent may not use it
until the scope joins. A task can also open its own Files. Two tasks never
hold one File. No copy, no sharing, no count race (the count only moves).
Console: one process-wide resource, serialized by the runtime per call.

### 5.8 Runtime and scheduler **[analysis]**
- **Pool with help-while-waiting.** A worker that waits for a `forall`
  to end, a task scope to join or a future it needs runs pending work
  instead of sleeping (work-stealing deques, as Cilk, rayon `join` and
  oneTBB do). Nested `forall`, nested scopes and futures then never
  exhaust the workers. To keep stacks bounded and avoid running unrelated
  work inside a waiting frame, a waiter helps only with work of its own
  scope (oneTBB's task isolation).
- **Blocking operations** (channel receive with nothing to receive,
  blocking file or socket I/O, console input, `Sleep`, a future FFI call)
  cannot help. The runtime that implements them tells the pool, which may
  start a temporary worker while one is blocked (compensation, as Java's
  ForkJoinPool managed blockers, and as Go hands its processor to another
  thread during a system call). No M:N scheduler is needed for this.
- **M:N** stays justified only by servers with many thousands of
  connections in blocking style; a decision for the network library.
- **Thread-safe runtime:** atomic String and File counts (or plain when
  the program starts no task: a whole-program fact); free lists per
  thread with a shared overflow list in `OwnRuntime.cpp`; re-keying walk
  (5.2); a lock per console call; exceptions captured and rethrown across
  tasks (`__inox_exception_capture` exists).
- **Memory model:** DRF-SC for the safe subset; release at task start,
  re-keying, channel send; acquire at join, future read, channel receive.
  Nothing exposed to programs.
- **ThreadSanitizer** checks the runtime and the generated code in tests;
  it finds races that happen in the runs it sees and proves nothing. The
  guarantee rests on the static rules plus a small trusted runtime, which
  must be reviewed as such.

### 5.9 Errors and cancellation (kept open) **[analysis]**
Traps: unchanged, any task's trap stops the program (CANON-19). Exceptions
need a full specification before D5 is decided: when the remaining
chunks stop (at chunk boundaries, cooperatively), whether `ensure` blocks
of cancelled tasks run (they must), which exception the parent sees
(lowest index, first in time, or an aggregate), and what a future read
after a failed task raises.

### 5.10 Lowering (hypothesis) **[hypothesis]**
Unchanged from revision 1: a `forall` body becomes a hidden routine
(as `ContractLowering` does for contracts) taking a context and a chunk;
the loop calls a runtime helper. To verify in a prototype.

## 6. Scenarios **[analysis]**

| Scenario | Model | Shared memory? |
|---|---|---|
| 1. Payroll of millions of records | `forall` over the array, writes `Pay[I]` from read-only inputs, reductions for totals | Read-only tables, disjoint writes |
| 2. Financial indicators on large arrays | `forall` + reductions; stencils write a second array | Read-only |
| 3. Consolidating many accounts | `forall` + exact reductions | Read-only |
| 4. Independent files and batches | one task per file, each opens its own File or gets one lent | No |
| 5. Server with many requests | task group, a task per connection, channels; caches through synchronized types | No |
| 6. Concurrent database access | each task owns its connection (moved in, re-keyed); pool as a synchronized type | Through the pool only |
| 7. GUI updated by background work | UI task owns the widgets (they hold Links: not transferable); futures/channels back | No |
| 8. Math on large collections | `forall` | Read-only |
| 9. Pipelines | tasks connected by channels, values moved (re-keyed) | No |

## 7. Safety matrix (revision 2) **[analysis]**

S = static, R = run time, T = trusted runtime or library.

| Case | Today | Verdict with the new rules | How |
|---|---|---|---|
| Two tasks read one immutable value | n/a | Accept | S: read-only rule 5.3 |
| Two tasks write one variable | n/a | Reject | S: 5.3 |
| One reads while another writes | n/a | Reject | S: 5.3 |
| Independent objects changed | n/a | Accept | S: owners moved in, or `A[I]` |
| Transfer by `Move` | Move checked (CANON-25) | Accept | S + R: re-keying 5.2 |
| Original used after the transfer | Compile error | Reject | S: existing |
| Old Link used after the owner moved to a task | Link stays alive (probe 1.6.1) | Trap, never a race | R: re-keying |
| Struct holding a Link | n/a | Not transferable | S: 5.1 |
| Arrays, distinct regions | n/a | Accept only at the loop index | S: 5.4.2 |
| `A[I] := A[I - 1]` | n/a | Reject | S: 5.4.2 |
| Stencil into a second array | n/a | Accept | S: A not written |
| Repeated index | Cannot occur (no user iterators) | Accept | S: 5.4.1 |
| Overlapping slices | Slices read-only | No issue; slicing a written array rejected | S |
| Capture of mutable variables | n/a | Read-only | S |
| Reference escaping a task | ref and slices scoped; Links not transferable | Impossible | S |
| Objects containing pointers | n/a | Owners by Move and re-keying; Links no | S + R |
| Mutable State | ref mut + State alias exists (probe 1.6.2) | Reject in parallel code unless frozen or synchronized | S: summaries 5.5, D3 |
| `ref mut` argument in a body | n/a | Only body locals or `A[I]` | S |
| FFI | Not in the language | Outside the guarantee unless declared safe | S: trusted declaration |
| Synchronized counter | n/a | Accept | T |
| Reductions | n/a | Accept, exact or reproducible as 5.6 | S + T |
| Nested `forall` and tasks | n/a | Accept | T: help-while-waiting 5.8 |
| Blocking inside a pool task | n/a | Accept | T: compensation 5.8 |
| File used by two tasks | File unique (probe 1.6.3) | Reject; lending to one task accepted | S: 5.7 |
| String appended in place while shared | Count 1 only | Safe with atomic counts and 5.3 | S + T |
| Console from many tasks | Globals unsynchronized | Accept, lines serialized | T |
| Errors and cancellation | n/a | Open (5.9) | R |

## 8. Necessary, desirable, deferred, avoided, open

- **Necessary:** transferable (transitive); structured scopes with the
  read-only rule; `forall` access rules on every access to written
  arrays; call summaries (State, `ref mut` roots, outer Links, I/O);
  re-keying at transfer; a thread-safe runtime; help-while-waiting and
  compensation in the pool; DRF-SC stated for the safe subset.
- **Desirable:** futures; exact reductions; `Channel<T>`; atomic
  counters; File lending.
- **Can wait:** protected values; M:N; writable chunk slices for
  irregular writes; user-defined reductions; cancellation tokens.
- **Avoid:** detached tasks; default `ref` to outer arrays; lifetimes;
  exposed threads, fences or memory orders; atomic Link checks on every
  use; letting the guarantee depend on the scheduler.
- **Open:** errors and cancellation (5.9); 192-bit Crypto partials or
  checked tree (D7b); lowering of bodies; measured cost of atomic counts
  and of re-keying; sample payroll programs against rule 5.4.2.

## 9. Incremental roadmap

1. Decide the direction; then write the constitution text of the
   guarantee and the reduction operation.
2. Runtime groundwork (no language change): per-thread free lists,
   atomic counts behind a whole-program switch, console lock, pool with
   help-while-waiting and compensation, exception transfer, re-keying
   walk. Tested with ThreadSanitizer.
3. Call summaries computed and exposed in a test dump; no language change.
4. `forall` with rules 5.4 and built-in reductions.
5. Structured `do` tasks, futures, File lending.
6. `Channel<T>` and atomic counters (after methods of generic structs).
7. Protected values, M:N, chunk slices: on demand.

## 10. Status of each point (the five questions of the review)

| Point | 1. Real today | 2. Already prevented | 3. Needs new static checks | 4. Runtime only |
|---|---|---|---|---|
| Link reaching an object after its owner moves | Yes (1.6.1), harmless sequentially | — | Not provable statically | Re-keying at transfer |
| `A[I] := A[I - 1]` and indirect reads | No `forall` exists | — | Rule 5.4 on every access, calls 5.5 | — |
| Repeated indices | — | Yes: no user iterators | Forbid them if added | — |
| Effects of calls | `ref mut` + State alias exists (1.6.2) | Exact call graph, no dynamic dispatch | Summaries 5.5 | — |
| State in parallel code | Plain globals | Module-private (r3.99) | Frozen-State check, D3 | — |
| Transitive transferability | — | No stored refs besides Link; values copied | Structural property 5.1 | Atomic String counts |
| Strings changed in place | Count-1 append (1.6.4) | — | Read-only rule 5.3 | Atomic counts, acquire on the check |
| File between tasks | — | Unique File (1.6.3) | Lending rule 5.7 | — |
| Pool exhaustion | No pool | — | — | Help-while-waiting, compensation |
| Float reproducibility | Plain IEEE IR (1.6.5) | No fast-math today | Keep it so (a gate) | Fixed tree |
| Overflow in reductions | Checked arithmetic, traps | Ordinary sums unchanged | New operation with its own rule | Wide partials |
| Runtime globals | Not thread-safe (1.2) | — | — | Locks, per-thread lists |

## 11. Decisions D1-D7 after the review

| Decision | Can be approved now? | Remaining condition |
|---|---|---|
| D1 Reject what cannot be proved | Yes, in principle | The guarantee's text names the safe subset, the trusted runtime and FFI |
| D2 `forall` rules | Yes, as rewritten in 5.4 | Sample programs confirm expressiveness |
| D3 State | Yes: only frozen State (never written, linked or passed `ref mut` after its initializer, checkable inside its module) or synchronized types | — |
| D4 Links and Files | Links: not transferable plus re-keying (5.2). Files: lent to one task (5.7) | Measure re-keying |
| D5 Errors and cancellation | No, keep in study | Specification of 5.9 |
| D6 Shared mutable state | Yes: library types first; no `protected` construct without demonstrated need | — |
| D7 Reductions | As a goal, yes; D7a scope of Float reproducibility as 5.6; D7b exact Crypto (192-bit) or checked tree | Constitution text for the new operation |

## 12. Test specifications (pseudocode, not grammar; no compiler change)

Each item states what the compiler or the runtime must do once the rules
exist. "Today" says what the current compiler does with the sequential
equivalent.

Accepted:
- A1 Payroll: `forall I` over `Employees`, read tables and rates, write
  `Pay[I]` and `Pay[I].FTax`, reduce the total Currency. Output equals the
  sequential loop; total exact.
- A2 Stencil: read A, write `B[I] := A[I - 1] + A[I]`.
- A3 Matrix: `forall I`, inner `for J`, write `M[I, J]`.
- A4 Nested: `forall I` with an inner `forall J` writing `M[I, J]`, on a
  pool of 2 workers (no exhaustion).
- A5 Tasks: two tasks, each given `Move(P)` of its own owner, write their
  objects; the parent reads the results after the scope.
- A6 Future: a task returns an owner; the parent reads it after the
  scope; its Links made inside the task are dead in the parent.
- A7 File lent to one task, which writes lines; the parent closes it
  after the scope.
- A8 Frozen State (a table filled by its initializer) read in a body.
- A9 Console output from many iterations: every line whole.

Rejected at compile time:
- R1 `A[I] := A[I - 1]`; R2 `A[J]` with J not the index; R3 `A[F(I)]`;
  R4 a slice of a written A; R5 A passed whole to a call while written.
- R6 a write to an outer scalar in a body; R7 the parent writes a
  variable a live task reads.
- R8 a body calls a routine that writes State; R9 a routine that writes
  State through a `ref mut` rooted at State (today: accepted, probe
  1.6.2); R10 non-frozen State read in a body.
- R11 an outer Link used in a body; R12 a value holding a Link given to a
  task; R13 a File used by the parent while lent.
- R14 a `ref mut` argument in a body that is neither local nor `A[I]`.

Trapped at run time:
- T1 the parent uses a Link made before `Move(P)` into a task: "use of a
  freed object or an empty Link" (today: the Link stays alive, probe
  1.6.1).
- T2 an Integer reduction whose exact total does not fit: overflow trap,
  same on every run and core count.
- T3 a trap in one iteration stops the program with exit status 70.

Runtime under ThreadSanitizer (no report expected):
- S1 many tasks sharing immutable Strings (counts); S2 owners allocated
  and freed in many tasks (free lists); S3 console from many tasks;
  S4 re-keying racing with nothing (the walk happens-before the task);
  S5 compensation when tasks block on a channel.

Open (with D5): exceptions in iterations, `ensure` in cancelled tasks,
future read after a failed task.

## 13. Revision 3: loans, not borrowing **[analysis]**

### 13.1 The case of the review
A task reads P (an owner) while the parent writes the same object through
a Link L made earlier. 5.3 forbade writing, moving, freeing and linking
the *variable* P, not writing the *object* through an older Link; and
re-keying runs only on a transfer. The case is a real race under
revision 2.

### 13.2 What is provable locally, and what is not
- A Link to a **local variable** can only be made by `Link(X)` inside the
  routine that declares X (a Link to a value parameter is rejected; a
  `ref` parameter's storage is the caller's). Whether X has been linked is
  therefore a local fact.
- A Link to an **owned object** can be made anywhere the object is
  reachable: callers, callees through `ref` (probe 10), objects that hold
  Links, State. Whether one exists is not a local fact.

### 13.3 Rule for `do` tasks: no loans at all (first version)
A task receives only:
- copies of values of a transferable type (5.1, which excludes owners
  reachable through Links and every Link);
- owners by `Move`, re-keyed (5.2);
- one File lent exclusively (5.7).

A task never reads the parent's storage. The race of 13.1 cannot occur: the
task reads its own copy, or an object no older Link can reach. The cost is
copying; a large value is moved instead (an owner of the array), and
read-only data shared by many tasks comes from frozen State (D3) or, later,
an immutable shared library type (13.6).

### 13.4 Rule for `forall`: the parent is suspended
During a `forall` no code of the parent runs, siblings hold no Link to the
parent's objects (13.3), and the body obeys 5.4. The only code that could
write an outer object through a Link is the body itself or a callee, which
14 closes. So a `forall` body may read outer storage, owners included,
without copying, provided:
1. outer values used in the body are of a transferable type (no Link at
   any depth: an outer value holding a Link is unusable in the body);
2. outer places are deeply read-only (14);
3. written arrays follow 5.4 and are of a transferable element type.

### 13.5 Pre-existing Links to the parent's locals
`Link(X)` of a local X made before a `forall` that reads X: the body cannot
use that Link (it is an outer value of type Link, rule 13.4.1), and the
parent does not run during the loop. Before a `do` task: irrelevant, the
task has a copy. No new analysis.

### 13.6 Later, when real programs ask
An immutable shared value (`Shared<T>`-like, library): built from a value,
never changed again, copied by an atomic count, readable by many tasks.
It is a synchronized type of D6 for the read-only case. Not needed for the
first version.

## 14. Transitive immutability and the origin of a write **[analysis]**

### 14.1 Deep read-only through owners (proposed language change, D8)
Probes 8-11 show that `ref`, `Self` and other read-only places do not
protect the objects their owners own. Proposal: an owned object is part of
its owner's value, as a field is.
- Through a read-only place (a `ref` parameter, a `Self` receiver, a
  `forall` body's outer variable, frozen State), the objects reached
  through `Own` and `Option<Own>` are read-only too: no assignment, no
  `ref mut` argument, no `Self mut` call, no `Move`, no `Free`.
- `Link(...)` of a place reached through a read-only place is an error: a
  Link is a capability to write, and making one from a read-only place
  would launder it.
- Unchanged: a Link already held stays a Link (5.1 keeps Link-holding
  values out of parallel code, so this matters only sequentially, where
  Links are shared mutable references by design).

This is a sequential rule, checked by the analyzer where it already checks
writes to `ref` parameters and `Self` (it extends "the root is read-only"
from the variable to every path below it). It changes the meaning of
existing programs (probes 8-10 become errors): a breaking change that needs
the maintainer's approval on its own, independent of concurrency.

### 14.2 Origin of a write: what is left to the call summaries
With 14.1, the only ways code can write memory that is not its own locals:
1. a `ref mut` parameter or `Self mut` receiver (visible in the signature);
2. State (visible as a symbol of kind State, transitively; a `ref mut`
   rooted at State counts as a write of that State, probe 2);
3. through a Link;
4. I/O and, later, FFI.

In parallel code, (1) is limited to body locals and `A[I]`, (2) to frozen
State, (3) to Links of iteration-local origin, which 13.4.1 and 14.1
guarantee structurally: an outer value holding a Link is unusable, and no
Link can be made from an outer place. A Link inside the body therefore
reaches only storage created in the iteration. No provenance tracking of
Links is needed. The call summary shrinks to: writes or reads non-frozen
State; I/O; FFI.

### 14.3 Nested owners, `ref`, `Self mut`, frozen State
- Nested owners: covered by 14.1 (deep through every `Own` level).
- `ref` parameters in a callee of a body: deep read-only by 14.1.
- `Self mut` on an outer value in a body: rejected (not `A[I]`); on
  `A[I]`: allowed, exclusive.
- Frozen State: a State variable is frozen when its type is transferable
  and, in its module, nothing assigns to any path rooted at it (through
  owners included), passes it or a part of it as `ref mut`, calls a
  `Self mut` method on it or a part of it, takes `Link(...)` of it or a
  part of it, moves or frees it. State is module-private (r3.99), so the
  check is local to the module.

## 15. Memory model **[analysis]**

### 15.1 Happens-before edges
Within a task, program order. Between tasks:

| Operation | Edge |
|---|---|
| Start of a task (and of each `forall` chunk) | Every action of the parent before the start, including the re-keying of moved owners, happens-before every action of the task |
| End of a task or chunk | Every action of the task happens-before every action of the parent after the join of its scope (end of the `forall`, end of the scope, read of its future) |
| Iterations of one `forall` | No order between iterations |
| Transfer of an owner (re-keying) | Done by the giving side before the start or the send: ordered by the edge of that start or send |
| Channel send/receive (library) | A send happens-before the completion of the receive that takes its value; for a channel of capacity C, the k-th receive happens-before the (k+C)-th send completes (the rule of Go's memory model) |
| Library atomic operations | Sequentially consistent (one total order over all of them, consistent with program order) |
| Lent File | The lending happens-before the task's first use; the task's last use happens-before the join |

The runtime realizes start/end with release/acquire on its queues and
joins; that is enough for these edges.

### 15.2 Two guarantees, kept apart
- **No data races (static):** a program of the safe subset (D1) has no two
  conflicting accesses to ordinary data unordered by the edges of 15.1.
  This is what the rules of 5, 13 and 14 prove.
- **Sequential consistency (DRF-SC):** such a program behaves as some
  interleaving of its tasks, consistent with program order, *provided*
  every synchronization the library offers is sequentially consistent: the
  atomic types use only sequentially consistent operations (no weaker
  orders exposed), and channels and other synchronized types are
  linearizable. Under the C++ model this holds for programs whose only
  atomics are `seq_cst`; release/acquire alone would not give a total order
  over the atomics themselves. Weaker orders, if ever offered, would be a
  separate, explicitly unsafe-for-SC library.

## 16. Scheduler limits **[analysis]**

| Goal | First strategy | Limits |
|---|---|---|
| CPU-bound parallelism (`forall`, compute tasks) | Pool sized to the cores, work-stealing, help-while-waiting within the waiter's own scope | None needed beyond the pool |
| Occasional blocking inside pool tasks (channel receive, file I/O, `Sleep`) | Compensation: the runtime primitive that blocks tells the pool, which may start a temporary worker | A fixed ceiling of extra workers (configurable, small multiple of the cores); beyond it, the blocked task waits; unmanaged blocking (FFI, unless declared) is not compensated |
| Thousands of connections or I/O operations | A separate, event-driven I/O subsystem in the network library (epoll/kqueue/IOCP or io_uring), designed on its own | Not solved by compensation; may later need lightweight tasks (M:N), a decision of that design |

Compensation is a safety valve for occasional blocking, as in Java's
ForkJoinPool, whose documentation also bounds it and does not promise it
for unmanaged blocking. A program whose tasks wait for each other through
channels can still deadlock (a logical deadlock, not a race); the
structured scopes rule out the other kind (waiting on work that can never
run), because a waiter runs the work it waits for.

## 17. Reductions (revision 3) **[analysis]**

- **Integer kinds and Currency:** exact sum, trap if and only if the exact
  total does not fit; 128-bit partials (count below 2^64).
- **Crypto:** exact sum with 192-bit partials as three 64-bit limbs in
  libinoxrt, portable C++ without a native 192-bit (or 128-bit division)
  type; trap iff the exact total does not fit. (The maintainer's review
  prefers exactness.)
- **Product:** checked over the fixed tree (reproducible; may trap where a
  left-to-right loop would not, or the reverse). Min, Max, Count, And, Or:
  exact.
- **Ordinary arithmetic does not change:** a reduction is its own operation
  with its own contract in the law; `Sum := Sum + X` in a loop keeps
  today's checked semantics.
- **Float, scope of reproducibility.** For the same compiled program and
  inputs, the result is identical bit for bit across runs, core counts and
  scheduling. Across machines, identical when:
  - both implement IEEE 754 binary32/binary64 basic operations;
  - rounding is round-to-nearest-even: Inox never changes the rounding
    mode, and neither may libinoxrt; FFI code that does is outside;
  - subnormals are kept: no flush-to-zero or denormals-are-zero (the
    default on x86-64 SSE2 and AArch64; the build must not enable
    fast-math, which may set them);
  - the generated code has no fast-math, contraction or reassociation
    flags (true today, probe 5; to keep as a gate);
  - the partition and tree come from the same compiler version;
  - the elements are equal: values from `Sin`, `Exp` and other library
    functions may differ between C libraries.
  Within that scope: signed zeros follow IEEE and are reproducible; NaN
  results are NaN on every machine, but the **sign and payload of a NaN
  are not** guaranteed (x86-64 and AArch64 produce different default
  NaNs). Min and Max are defined order-free: `-0.0` below `+0.0`, and NaN
  if any element is NaN.

## 18. Errors and cancellation **[analysis]**

A small model, no exception aggregation:
1. **Traps** stop the program (CANON-19), unchanged.
2. **Cancellation is cooperative:** checked at chunk boundaries, at task
   starts and in blocking runtime primitives. Work not yet started is
   skipped; nothing is interrupted in the middle of a statement.
3. **Cleanup always runs for started work:** an exception or a
   cancellation unwinds a started iteration or task normally, so its
   `ensure` blocks run and its owners are freed. Work cancelled before it
   started ran nothing and has nothing to clean.
4. **The scope always waits** for every started iteration or task before
   it raises.
5. **Which exception, deterministically:** in a `forall`, when iteration k
   raises, only iterations above the lowest index known to have raised are
   cancelled; iterations below it keep running. When all have finished,
   the exception of the lowest raising index is the one raised in the
   parent. That is the same exception a sequential loop would raise first,
   whatever the timing. For tasks of one scope, the same rule in the order
   the parent started them. The other exceptions are discarded.
6. **No lost failure:** a task's exception is raised by the read of its
   future or, if nobody reads it, by the end of its scope (rule 5).
7. **State after a failed `forall`:** elements written by iterations below
   k are written; above k, unspecified (some ran, some did not). Reductions
   of a failed loop give no result.

## 19. Updated decisions **[analysis]**

| Decision | Revision 3 position |
|---|---|
| D1 | Approve as a requirement; the guarantee names the safe subset, the trusted runtime and FFI |
| D2 | Approve the direction with 5.4 (all accesses), 13.4 (transferable outer values) and 14.1 |
| D3 | Approve with the frozen-State definition of 14.3 (deep, module-local) |
| D4 | Approve File lending; Links never cross tasks; owners cross only by `Move` with re-keying; **no read loans to `do` tasks in the first version** (13.3) |
| D5 | Proposal ready for decision: section 18 |
| D6 | Approve: library types first; 13.6 later |
| D7 | Approve the direction: exact 128/192-bit sums, checked tree for products, Float scope of 17 |
| **D8 (new)** | Deep read-only through owners, and no `Link(...)` from a read-only place: a sequential, breaking change, needed by D2 |

## 20. The question of the review

*Can the no-data-race guarantee hold without a general alias analysis or a
borrow system like Rust's?* **Yes, under the rules of revision 3**, for
these reasons:
- Aliases of Inox are few and named: Links, State, `ref`/slices (scoped)
  and owners (unique). Each is handled by one local rule: Links never enter
  parallel code from outside (5.1, 13.4.1) and cannot be made from
  read-only places (14.1); State only frozen (14.3); `ref` is deep
  read-only (14.1); owners cross by `Move` with re-keying (5.2).
- `do` tasks borrow nothing (13.3): no loan, no loan checking.
- `forall` borrows read-only while its parent is suspended (13.4); every
  check is on the loop body and on per-routine summaries over an exact call
  graph (State, I/O, FFI).
- What the compiler needs is a few new facts on types (transferable),
  places (root and path, as `checkReferenceArguments` already computes),
  routines (summaries) and one sequential tightening (D8). None of them is
  a lifetime, a region or a borrow.

What it costs: tasks copy or move instead of borrowing; outer Link-holding
values are unusable in parallel code; probes 8-10 become errors; a stale
Link after a transfer is a run-time trap, not a compile error. What
remains unproved until a prototype: the expressiveness of 5.4 on real
programs, the cost of re-keying and atomic counts, the breakage of D8 in
the existing suite.

### Tests added by revision 3 (pseudocode)
- R15 the parent writes an object through an older Link while a `do` task
  would read it: rejected because a task cannot read the parent's object
  (13.3); the accepted form copies or moves.
- R16 a body reads an outer struct holding a Link: rejected (13.4.1).
- R17 `Link(H.FChild)` with `ref H`, and `Link(Self.FChild)` in a `Self`
  method: rejected by D8 (today accepted, probe 10).
- R18 `H.FChild.FValue := 1` with `ref H`: rejected by D8 (today accepted,
  probe 8).
- R19 a frozen-State candidate whose owner is written in a routine of its
  module: not frozen, so rejected in a body.
- A10 a `forall` body reads an outer owner's object (transferable type):
  accepted, no copy.
- T4 iterations 10 and 500 both raise: the parent always sees the
  exception of 10, and `ensure` blocks of started iterations ran.
- T5 a failed task's future is never read: its exception is raised at the
  end of the scope.

## 21. Revision 4: three forms of transfer, one property **[analysis]**

"Transferable" stays one internal property of types, defined as the union
of three forms, each reusing an existing rule of the language:

| Form | Types | Rule reused | What the receiving task gets | What the giving task keeps |
|---|---|---|---|---|
| Independent copy | Scalars, enums, Ranges, Sets, Currency, Crypto; structs and fixed arrays of such components | Value semantics (CANON-9, fixed arrays): assignment copies | Its own storage | Its own storage, unchanged and independent |
| Exclusive transfer | `Own<T>`, `Option<Own<T>>`; any struct or array that holds an owner at any depth (already move-only today: never copied, CANON-25) | `Move` (CANON-25) plus re-keying (22) | The owner and every object it owns | Nothing: the variable is moved-from (compile-time), and its older Links are dead (run time) |
| Immutable sharing | `String` (and a component String of a copied struct) | Immutability (CANON-14) plus atomic counts in the runtime | A reference to the same immutable bytes | The same reference; neither side can change the bytes (in-place append happens only with count 1, which a shared block never has) |

A composite type takes the strictest form of its components: a struct with
a String and an Integer is copied, its String part shared; a struct with an
`Own` field is transferred exclusively (it is move-only, so it can never be
copied by accident). A type with a `Link<T>`, a slice or a `File` at any
depth is not transferable; a File alone is lent (5.7), which is not a
transfer. No new reserved word, no new type, no new kind of
declaration: the compiler derives the form from the type, as it already
derives "move-only".

## 22. Re-keying is a rule of its own, not `Move` **[analysis]**

The probe 1.6.1 shows the sequential rule: after `Q := Move(P)`, a Link
made from P still reaches the object, as long as it exists. The proposal
does **not** change that. It adds a different, observable rule:

> **Transfer between concurrent contexts.** When an owner leaves its task
> for another task (as an argument or capture of a `do` task, as the
> result of a future, through a channel send, or by any later library
> transfer), every Link made before the transfer to the transferred object
> or to any object it owns stops reaching it: `Alive(L)` is false and a use
> traps with `use of a freed object or an empty Link`, while the object
> lives on in the receiving task. A `Move` inside one task keeps today's
> meaning.

Consistency with `Alive`: CANON-25 says a Link is alive while its object
exists; the new rule says a Link is alive while its object exists **in the
task that holds the Link**. In a program with no tasks the two are the
same, so no existing program changes behaviour. The text of CANON-25 needs
that amendment (section 25.3).

The generation increment is one implementation; its cost (a walk over the
transferred blocks, skipped for types never linked anywhere) is measured
by the prototype before the rule is final.

## 23. D5, worded without overpromising **[analysis]**

Proposed wording:

> When iterations of a `forall` raise exceptions, the parent receives the
> exception of the lowest index among the iterations that actually raised.
> No iteration of a lower index than one that has raised is cancelled, so
> every lower iteration runs to its end. Iterations of higher index may be
> cancelled before they start. Every started iteration completes its
> cleanup (`ensure` blocks, release of owners) before the exception reaches
> the parent. The exceptions of the other iterations are discarded. Tasks
> of one scope follow the same rule in the order the parent started them.

When this equals the first exception of a sequential loop: only if whether
each iteration raises, and which exception it raises, depends on nothing
but its own index and the read-only inputs (no I/O, no clock, no
dependence on other iterations' side effects). With I/O inside the body
(a full disk, a file another iteration created), the selected exception is
still well defined but may differ from a sequential run. The discarded
exceptions are documented as discarded; a later version may report them
(an aggregate, or a diagnostic count) without changing which one is
raised.

## 24. Impact of D8 on the repository **[code]**

Method: every program that can hold an owner was inspected (an owner can
only appear in a type that writes `Own<` or `Option<Own`, directly or as a
generic argument): 28 files, 20 of them diagnostics, 7 runtime tests and
one example; the standard library has none. In those files, every routine
with a `ref` (not `ref mut`) parameter or a `Self` (not `Self mut`)
receiver was checked for an assignment, `ref mut` argument, `Self mut`
call, `Move`, `Free` or `Link(...)` on a path that goes through an owner.

Result:
- Read-only receivers or parameters in those files: `TPoint.Sum(Self)`
  (tests/runtime/pointers.inox), which reads two Integer fields, and
  `Integer.Twice(Self)` (tests/runtime/pointers-more.inox), on an Integer.
  Neither goes through an owner.
- `Link(...)` uses in those files are on locals, on owners held in local
  variables, and on paths through local Links
  (`Current := Link(Current.FNext)`): none starts at a read-only place.
- Generic code that may be instantiated with an owner
  (tests/runtime/generics-structs.inox, `Keep<T>(P Own<T>)`) takes owners
  by value (they belong to the routine) and writes nothing through a `ref`.
- **No existing test, example or library module would be affected.** Only
  the probes of 1.7 (scratch programs outside the repository) break.

Migrations, for the record (none needed today): a routine that changes an
owned object through a parameter declares it `ref mut`
(`Poke(ref mut H THolder)`); a method that changes an object its receiver
owns declares `Self mut`; a routine that only needs a reference it can
write through takes a `Link<T>` argument made by the caller from a
writable place.

The inspection is not a proof; the prototype of D8 must run the whole
suite (and the fuzz corpus, whose mutants may combine `ref` and owners) to
confirm it. Where the rule stops: a path through a `Link` is not
read-only, because a Link is a write capability by design (CANON-25); only
owners, fields and elements carry the read-only of their root.

## 25. Consolidation **[analysis]**

### 25.1 Decisions approvable now (direction; no code)
- **D1** the safe subset is statically free of data races; FFI and the
  trusted runtime are named outside it.
- **D2** `forall`: written arrays accessed only at the loop index; outer
  values transferable and deeply read-only; reductions declared.
- **D3** only frozen State (deep, module-local check) in concurrent code.
- **D4** Files lent to one task; owners transferred by `Move` with the rule
  of 22; Links never cross; `do` tasks borrow nothing.
- **D5** as worded in 23.
- **D6** shared mutable state only through library synchronized types.
- **D7** exact sums (128-bit partials for Integer kinds and Currency,
  192-bit for Crypto), checked fixed tree for products, order-free Min and
  Max, Float reproducibility within the scope of 17.
- **D8** deep read-only through owners for `ref` and `Self`; no
  `Link(...)` from a read-only place.

### 25.2 Questions that depend on a prototype
1. D8 on the whole suite and the fuzz corpus (24).
2. Whether the access rule of `forall` expresses real payroll, indicator
   and consolidation programs (write them first).
3. The cost of re-keying and of atomic String counts, measured, and the
   whole-program switches that avoid them.
4. Outlining a body into a hidden routine reading the parent's frame.
5. The pool: help-while-waiting within a scope, the ceiling of
   compensation, behaviour under ThreadSanitizer.
6. The surface syntax: the name of the data-parallel loop, the form of
   `do` and of a future, the capture clause `| |`
   (`delimiters-and-annotations`), how a reduction is declared.

### 25.3 Constitutional changes that will be needed (each needs approval)
| Where | Change |
|---|---|
| CANON-1 Identity | The target domains described on 2026-10-31 (pending separately) |
| CANON-1 Safety defaults | "No data races in the safe subset" joins the list |
| FUTURE Concurrency | Becomes a CANON requirement: structured tasks and data parallelism, the guarantee of D1, the scope of DRF-SC (15.2), `do` |
| CANON-9 and ADR-0016 (`ref`, `Self`) | D8: read-only through owners; no Link from a read-only place |
| CANON-25 Links | The rule of 22: a transfer between tasks ends older Links; Links are not transferable |
| CANON-5 State | Frozen State; State in concurrent code |
| CANON-14 Strings | Immutable sharing between tasks (the counts are status, not law) |
| CANON-8 / arithmetic | Reductions as an operation of their own (D7), ordinary arithmetic unchanged |
| CANON-15 Exceptions | D5 in parallel constructs; cleanup of started work |
| CANON-19 Traps | A trap in any task stops the program (already true; to state) |
| CANON-23 Code generation | No fast-math, contraction or reassociation, as a requirement of reproducibility |
| `stdlib/files` | Lending a File to one task |
| engineering.md | A ThreadSanitizer gate for runtime changes and concurrency tests |

### 25.4 What does not change
Ordinary arithmetic and its traps; `Move` within a task; value semantics;
generics without constraints; modules and visibility; the behaviour of
every existing program except those of the D8 probes.

## 26. Closing of the exploration (2026-10-31)

Marcelo Fortes approved revision 4 on 2026-10-31 as the consolidation of
the exploration: D1-D8 are the **technical direction** of Inox
concurrency. That approval authorizes no change to the constitution and
no implementation; each of those comes through its own change. Three
precisions are part of the approved text:

1. **D1, the basis of the guarantee.** Programs written in the safe subset
   of Inox, using only operations and libraries that respect their
   concurrency contracts, are free of data races through the combination
   of static checks and trusted mechanisms of the runtime (the re-keying
   of 22, the synchronization of 15.1). The runtime is part of the trusted
   base that carries the guarantee, not outside it; FFI is outside unless
   declared.
2. **CANON-25, validity by event.** A Link stays valid while its object
   exists and no transfer between concurrent contexts made after the
   Link was created has ended its access. Validity is defined by that
   event, never by where the object physically is or by which thread runs
   a task; a `Move` within one task keeps today's meaning. (This replaces
   the wording "in the task that holds the Link" of 22.)
3. **D5, one propagation per failure.** Reading a future whose task failed
   raises its exception. A failure already observed this way is not
   raised again when the scope ends normally. Failures nobody observed are
   raised when the scope ends, under the selection rule of 23. Cleanup of
   started work stays mandatory.

Also recorded: **no optimization at first.** The first implementation
keeps atomic String counts and the re-keying walk uniform for every
program; whole-program analyses that would skip them (programs without
tasks, types never linked) come only after the cost is measured, and only
if the measurement justifies them.

Next, in order: (1) this exploration stays as the record; (2) an isolated
change for D8 (`read-only-through-owners`), prepared as artifacts only;
(3) its implementation after a specific authorization; (4) CANON-9 and
ADR-0016 updated if the results confirm the semantics; (5) `forall`
resumed with real payroll and financial programs before any parallel
runtime.
