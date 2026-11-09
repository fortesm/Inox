<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Microcontracts and receiver rebinding: architectural design study

**Status:** Design exploration, **not approved for implementation**. This document
is non-normative: it neither changes CANON-1/CANON-8/CANON-9/CANON-24/CANON-25
nor announces a compiler feature. All syntax below marked *conceptual* is
illustrative, not accepted Inox. A subsequent OpenSpec change and explicit
maintainer G7 decisions are required before changing language semantics.

**Recorded:** 2026-11-09. **Purpose:** Preserve a substantial design direction
and its trade-offs so that later work does not lose the rationale or mistake
an earlier, narrower implementation for a rejection of the larger idea.

## 1. Existing law, implementation, and historical boundaries

Inox is deliberately post-object-oriented. CANON-1 excludes classes,
classical inheritance, Java-style interfaces, mixins, duck typing and class
taxonomies. Structs are nominal data types; associated methods are declared
outside the struct. Reuse, visibility and method calls have the precise
current semantics set out in CANON-3 and CANON-9.

The approved `method-reuse` change (2026-10-13, r3.66) chose:

- `TDerived Struct (TPart)` as composition and reuse of offered members.
- **Delegation:** an inherited/reused method executes on the contained part,
  not on the outer struct. `Self` does **not** rebind.
- `Override` to resolve replacement of offered methods statically.
- Methods of built-in types and ordinary routines with `ref mut` to operate
  on writable fields.
- No subtyping and no automatic dynamic dispatch.

The original archived design explicitly excluded receiver rebinding, member
mapping, protocols/interfaces as nominal entities, and dynamic dispatch.
Those exclusions describe **that implementation**; none of the advanced
mechanisms in this study is already implemented. This study does not silently
reverse that decision. Whether an **additional opt-in** mechanism can coexist
with delegation is a G7 question.

Existing generics (CANON-8) instantiate routine and struct templates once
per list of concrete types, perform checking on each instance and have **no
declared generic constraints**. Current generic methods reuse the receiver's
generic type parameters; methods with their **own** type parameters are
currently rejected. CANON-24 supplies `pre`, `pos`, `invariant` and
`variant` (Design by Contract), distinct from the proposed microcontracts.
CANON-25 supplies owner/link safety, not a trait system.

Primary context:
- `docs/openspec/changes/archive/2026-10-30-method-reuse/{proposal,design}.md`
- `docs/openspec/specs/constitution/spec.md` (CANON-1, -3, -8, -9, -24, -25)
- `docs/openspec/specs/language/generics/spec.md`
- `docs/OOP_COMPARISON.md`, `docs/ROADMAP.md`
- `docs/ARCHITECTURE.md` (semantic phases and TIR boundary)

## 2. The intended capability, not a new spelling of interfaces

**Primary objective:** Reuse the **complete implementation of an associated
method** across nominal, structurally different structs, with explicit
adaptation of its receiver and of the members it uses. Neither the destination
struct's fields nor their names need equal those of the source struct.
No class hierarchy, interface declaration, forwarding method per destination,
or runtime vtable should be necessary for this **static reuse** case.

Three complementary concepts define the direction:

1. **Receiver rebinding:** specialize a method body's `Self` to a distinct
   destination receiver, rather than executing against an embedded source
   part (the existing delegation semantics).
2. **Member mapping:** explicitly associate a member referenced by the
   method with a possibly differently named member/path of the destination.
   This is semantic name binding, not textual search/replace.
3. **Inferred receiver microcontract:** derive the *minimal, local* set of
   fields, callable members, types, access modes and effects demanded by the
   implementation; validate the destination and mapping against that set.

The programmer may request reuse explicitly while the compiler derives the
microcontract. A nominal `Trait`, `Protocol` or `Interface` definition is
not a prerequisite for the reuse mechanism. This does **not** imply that
microcontracts automatically constitute stand-alone types, subtype
relationships, universal structural conformance, or dynamic dispatch.

The term *microcontract* describes a compile-time receiver capability
requirement; it is **not** another name for CANON-24's runtime or static
Design-by-Contract assertions.

### 2.1 Canonical motivation: Person and Audition

The following struct and source-method declarations express the motivation
using current-style Inox constructs:

```inox
Type
    TPerson Struct
        FName String
        FReference Integer
    ;

    TAudition Struct
        FIdentification String
        FFactor Integer
    ;

TPerson.AlterValue(Self mut, S String):
    Self.FName := S
;
```

The source implementation writes a `String` member named `FName`.
`TAudition` has no such name but has a writable `String` member named
`FIdentification`. Current delegation via `Struct (TPerson)` would execute
on the embedded `TPerson` part; it would not make the method update
`TAudition.FIdentification`. Handwritten forwarding or an ordinary routine
with `ref mut` can solve this *single assignment* case, but those do not
offer general whole-method adaptation.

**Conceptual notation only — NOT actual Inox syntax:**

```inox
== Hypothetical opt-in reuse form; NOT implemented
TAudition Reuse TPerson.AlterValue
    Map Self.FName => Self.FIdentification
;

A.AlterValue("Audit-42")
```

The desired specialization binds `Self` to a `TAudition` receiver and
resolves the source field access to `FIdentification`; the entire method
body (control flow, nested calls, checks, contracts and effects) is shared
in source and specialized by the compiler. This example proposes no
particular keywords, placement, syntax or permission rules.

A simpler existing alternative remains valuable:

```inox
AlterValue<T>(ref mut Field T, NewValue T):
    Field := NewValue
;
```

That example is **illustrative and subject to normal generic and ownership
rules**. Passing a field by `ref mut` can avoid many mapping use cases.
Receiver rebinding is justified when **a complete implementation** uses
several fields, helper methods, state transitions, control flow and contracts,
or when retaining an associated-method API matters. It must not merely
duplicate existing free-routine functionality.

### 2.2 Generic capability rather than `String`-specific editing

A method could depend on an abstract field of type `T` rather than on
`String`. Its inferred microcontract would state that the destination
supplies a writable member of the substituted type `T`, along with any
operations used by its body. Destination types could bind different names
and instantiate distinct `T`s, subject to CANON-8 and CANON-25.

**Conceptual only** (this does not assert that independent generic method
type parameters, or the displayed reuse syntax, currently exist):

```text
SourceReceiver<T>.Replace(Self mut, Value T):
    Self.Item := Value

DestinationReceiver<U> reuses SourceReceiver<U>.Replace
    map Item -> Payload
```

For owner-bearing types, `Replace` is not automatically a copy: the same
Move, ownership, drop, alias and exception rules as ordinary Inox assignments
must apply. A mapping cannot manufacture a writable borrow from a read-only
receiver or bypass any visibility boundary.

## 3. Semantic model to evaluate

A possible implementation direction is **explicitly requested, locally
structural reuse**. The compiler analyzes the source method, obtains its
typed receiver requirements, substitutes type parameters, resolves an
explicit member map and verifies the specialized body for the target receiver.
This is *not* global duck typing: unrelated structs acquire no implicit
subtype relationship or shared runtime type merely because they happen to
have matching members.

An inferred requirement set may include:

- Receiver mode (`Self` versus `Self mut`) and exact access paths.
- Required readable/writable fields, field types, nested paths and array
  indexing semantics.
- Required methods/functions, overload resolution, parameter modes,
  result types, visibility and effects (including exceptions).
- Generic type equalities, substitutions and operations actually used,
  including constraints arising transitively from helper calls.
- Ownership and lifetime constraints: Move, Free, read-only owner paths,
  `ref`/`ref mut`, `Own<T>`, `Link<T>`, and invalidation behavior.
- Contract dependencies: `pre`, `pos`, `old`, `invariant`, `variant`.
- Origins of requirements so a failed adaptation reports both the source
  location and the target/mapping responsible.

Requirements should be represented in typed semantic data and could be
exposed in compiler-produced module/interface metadata, without requiring
a source-level nominal interface declaration. The format, export rules and
stability of such metadata remain to be designed; current
`inox interface` output must not be presumed adequate.

The specialization must preserve the evaluation order and single-evaluation
rules of ordinary Inox; it must not substitute source text, duplicate side
effects, resolve names differently in the backend, or bypass the TIR
boundary. Existing contracts of compiler phases in `docs/ARCHITECTURE.md`
remain binding.

### 3.1 Rebinding versus existing delegation

| Property | Existing CANON-9 reuse | Proposed additional adaptation |
|---|---|---|
| Data relationship | Destination contains a named source part | Target may be an unrelated struct |
| Executing receiver | Source part | Target struct |
| Source field `Self.FName` | Field of embedded source part | Explicitly mapped target field |
| Method call resolution | Static, with current `Override` rules | Requires explicit conflict and self-call rules |
| Subtyping / vtable | Neither | Neither for this static mechanism |
| Implementation status | Implemented | Not implemented, G7 pending |

This is a critical distinction: silently changing `Struct (TPart)` from
delegation to receiver rebinding would break existing programs and violate
the approved semantics. If pursued, the new mechanism should likely be
explicit and separate, unless the maintainer authorizes a breaking change.

### 3.2 Contracts and correctness cannot be reduced to matching types

`TPerson.FReference Integer` and `TAudition.FFactor Integer` have the same
representation but may have unrelated meanings. A matching type cannot prove
semantic equivalence. The source author and reuse site remain responsible
for meaningful mapping, while the compiler enforces all checkable safety
rules.

The design must explicitly decide whether source contracts are copied,
rebound, adapted or rejected; how target invariants are checked; what
happens when a source method calls another source method through `Self`;
and whether recursive/reentrant calls, `Override` and qualified calls are
rebound as well. No contract may silently be dropped just because a mapped
field has the right type.

## 4. What this design buys and what it does not

### Benefits under investigation

- Whole-method reuse between unrelated nominal structs, even with different
  member names and layouts; explicit adaptation instead of forwarder boilerplate.
- Static specialization, potential inlining and constant propagation; no
  mandatory runtime type tag, vtable or class hierarchy for this use case.
- Microcontracts inferred from actual implementations rather than an
  expansive nominal protocol hierarchy.
- Interoperation with current template-based generics while keeping
  ownership safety and predictable runtime behavior.
- A narrower and potentially more maintainable language surface than
  importing full Rust Traits or Swift Protocols.

### Costs, omissions, and risks

1. **No abstract value type:** a receiver microcontract is not automatically
   a type usable for heterogeneous parameters, fields, return values or
   collections.
2. **No dynamic polymorphism:** an unknown-at-compile-time object cannot
   simply dispatch through a microcontract. Plugins, heterogeneous UI
   components and open-world runtime extension need a separate mechanism.
3. **No guaranteed independent API contract:** an inferred contract depends
   on the method body. A change of implementation may change its required
   members, break downstream reuse, or force recompilation.
4. **Separate compilation and ABI stability:** specializing against target
   types may require source or serialized typed bodies and may increase
   dependency coupling across modules and libraries.
5. **Code-size and compile-time costs:** many target/type combinations may
   duplicate specialized code; whole-program optimization may recover
   some but not all overhead.
6. **Associated types and relationships:** representing an element type,
   iterator type and relationships between associated operations is more
   expressive than a lone `T`; not supplied automatically by rebinding.
7. **Conformance discoverability and tooling:** documentation, diagnostics,
   IDE completion and compiler errors need stable descriptions of inferred
   requirements rather than only late instantiation errors.
8. **Coherence/conflicts:** two reuse sites may offer the same name,
   overloads may differ, and transitive mappings may conflict; resolution
   cannot be accidental.
9. **Semantic equivalence:** two equally typed members need not mean the
   same thing; explicit mapping is an obligation, not a proof.
10. **Safety/effects:** ownership, aliasing, mutation, exceptions, cleanup,
    old-value snapshots and invariants can interact across the new receiver.
    Rebinding is a semantic transformation, not a name substitution.
11. **Privacy and evolution:** private members of source and target modules
    must not become accessible by reuse as a side effect. Public library
    contracts need an intentional export and compatibility policy.
12. **No automatic runtime reflection or type erasure:** an inferred
    capability set neither yields a runtime capability object nor solves
    existential packaging.

These are not all permanent losses. They are capabilities the *static method
reuse design alone* does not provide; a later, orthogonal mechanism may
address them without turning reuse itself into traditional OO.

## 5. Comparative assessment

| Language | Mechanism | Capabilities not automatically supplied by static rebinding |
|---|---|---|
| Java | Nominal interfaces; default methods; interface-typed references | Public independent interface type, dynamic implementations, compatible library contracts |
| C# | Interfaces, default implementation, generic constraints, static abstract interface members | Strong generic capability declarations (including numeric operators) and runtime interface values |
| Modern Object Pascal / Delphi | Nominal interfaces, interface references and implementation delegation | Interface identity/reference semantics, explicit interface implementation and object delegation |
| Go | Structural/implicit interface satisfaction and interface values | A separately named interface *type* with runtime method dispatch and type assertions |
| Rust | Traits, default methods, bounds, associated types, optional `dyn Trait` | Expressive associated-type requirements, independently checked bounds and dynamic Trait Objects |
| Swift | Protocols, protocol extensions, associated types, `some` and `any` | Protocol existential values, opaque return types, associated-type relations and public constraints |

Our proposed mechanism differs from all six in emphasizing **adapting the
same method implementation to different data shapes** with *explicit field
mapping*. It addresses reuse, not the entirety of what interfaces, traits
and protocols do. It must not be oversold as a drop-in replacement for
heterogeneous runtime polymorphism.

In particular, Java/C# interfaces combine contract typing with dynamic
dispatch; Go keeps implicit implementation but still has interface values;
Rust and Swift distinguish static specialization from existential/dynamic
forms. The design question for Inox is whether abstract *values* will
eventually need a separate, deliberately small facility.

## 6. Candidate mechanisms, deliberately kept distinct

**A. Explicit whole-method reuse (primary goal).** A destination requests
reuse, supplies a semantic member map when necessary and receives a
compile-time specialization. Existing CANON-9 composition/delegation remains
unchanged unless separately approved.

**B. Compiler-inferred microcontract (enabler).** Derived from the exact
requirements of the source method, verified against target/mapping, optionally
emitted as typed metadata for tool support and module boundaries. No
source-level `Trait`/`Interface`/`Protocol` declaration is required to
perform the reuse.

**C. Optional abstract-value facility (future and separate).** If programs
need runtime heterogeneous values or open extension, investigate variant
structs, typed routine values, explicit operation tables/type erasure or
another minimal facility. None is selected by this document, and none should
be smuggled into A or B.

**D. Optional explicitly authored constraints (future and separate).**
Some APIs need a published contract that does not vary with implementation
details. Decide later whether inferred microcontract metadata is sufficient
or if an independent source-level requirement declaration is useful.
Do not assume nominal Traits are mandatory.

## 7. Design decisions requiring G7 approval

No compiler agent is authorized to decide these silently:

1. Exact syntax and declaration location of opt-in reuse and member maps;
   whether mappings include fields, methods and nested paths.
2. Rebinding of `Self` and `Self mut` in nested calls, recursion,
   qualified calls, overloads and `Override`; precedence relative to
   existing offered members.
3. Whether source and target must both be exported/visible across modules;
   how private members and method bodies are handled for specialization.
4. Whether an implementation can be adapted without an associated source
   receiver (e.g. a reusable named method template).
5. When a method's inferred microcontract is computed: before or after
   generic substitution, and whether dependent/cyclic requirements are
   supported.
6. How member mapping interacts with `ref`, `ref mut`, borrowed values,
   owner moves, Links, aliasing, invalidation and exception exits.
7. Source `pre`/`pos`/`old`, target `invariant`, and contracts invoked
   by adapted helper calls; whether cross-type contract substitution is
   valid and what must be rejected.
8. Separate compilation, exportable specialization metadata, ABI stability,
   incremental compilation and code-size control.
9. Conflict resolution when more than one reused method has the same name,
   when mappings overlap, or when reuse is transitive.
10. Whether explicit semantic adaptation qualifies as *non-duck-typing*
    under CANON-1 as written, or requires an approved constitutional
    clarification. No blanket structural subtyping may be inferred.
11. Whether independently published capability constraints or abstract
    runtime values are needed at all; keep these independent of static reuse.
12. Whether this work belongs with post-0.5 expressiveness and exactly how
    it is sequenced relative to the approved roadmap. No artificial
    subrelease or premature version increment.

A recommended starting position, **not an approved rule**, is: opt-in reuse
with explicit mapping; static receiver specialization; implicit inferred
requirements recorded in typed semantic data; no automatic subtype, Trait
object or dynamic dispatch; no changes to existing delegation; strict
CANON-3/-24/-25 conformance.

## 8. Compiler impact and evidence needed if approved

A real implementation would touch at least:

- Parsing/AST for the chosen reuse syntax (only after syntax approval).
- Module resolution for source methods, target members and visibility.
- Semantic analysis for receiver requirements, mappings, generic
  substitution, diagnostic locations and ownership checks.
- Contract lowering for rebinding `pre`/`pos` and invariants safely.
- `SemanticResult` and typed IR (TIR) for explicit resolved call and
  member identities; codegen must not rediscover names or infer types.
- LLVM lowering for ordinary statically specialized methods, with
  cross-platform ABI preservation, exception handling and cleanup.
- Module metadata / `inox interface` only if an export decision demands it.

Evidence, not reports (G9): tests must exercise the full method body and
demonstrate the correct target state after mapping. Include positive and
negative examples for unrelated structs, different field names, nested
field paths, generic types, overloaded helper methods, visibility,
conflicts, read-only receivers, invalid ownership transfers, Link lifetime,
exceptions/ensure, `pre`/`pos`/invariants, recursion, across-module
reuse and same-name member resolution. Inspect TIR and LLVM IR to
demonstrate actual receiver specialization with no unintended dynamic
dispatch; cover Linux and Windows/MSVC ABI, ASan/UBSan where available,
regression suite and E19 gates. Record test commands, environments and
results; do not count a design document or an unmerged PR as implementation.

## 9. Planning and preservation

This document deliberately does **not** amend `docs/ROADMAP.md` or declare
the feature part of 0.5. The approved near-term sequence remains safe-pointer
follow-ups, generic containers, `Option<T>`, `Result<T,E>`,
`Vector<T>` and related ownership work. Generic constraints and other
expressiveness questions currently appear in the 0.6 discussion.

The interaction is important: the compiler should avoid designing advanced
generic capabilities in a way that unnecessarily blocks a future, explicit,
statically checked receiver-adaptation facility. This is an architectural
compatibility concern, **not** permission to implement microcontracts
incidentally in a Safe Pointers or Vector PR.

To advance the design, produce an OpenSpec proposal and risk assessment,
present one consolidated set of G7 decisions to the maintainer, approve
semantics and syntax before writing implementation, and then implement
an integrated, test-backed capability rather than fragmenting the goal
into documentation-only milestones. Preserve the compiler version policy:
CANON revision numbers are not compiler subreleases.

### Status legend

- **Existing:** CANON-9 composition/delegation, static `Override`, generic
  routine/struct specialization, Design by Contract, safe pointers.
- **Direction to investigate:** opt-in full-method receiver rebinding,
  explicit member mapping, implicit per-method microcontracts.
- **Still open:** source syntax, precise semantics, cross-module metadata,
  inferred-versus-explicit constraints, abstract-value/dynamic facilities.
- **Not implemented:** the new rebinding/mapping/microcontract feature.
