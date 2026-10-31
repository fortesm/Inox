## Why

Three external reviews of the repository (2026-10-31) found the compiler's
largest risks in its structure, not in missing features: a parser that
desugars, routine headers and types passed as text, an AST without source
positions, an emitter that reads the raw AST beside many side tables, and
quality gates not all run by CI. Measured against the code, most of these
are deviations from the existing law (E11 phase boundaries, E14
diagnostics, E19 gates), not new questions. The maintainer also asked that
the responsibilities of each part of the compiler be written down so that
no agent, without the history of a conversation, gets lost.

Approval: Marcelo Fortes, 2026-10-31: the foundation milestone F0-F7 in
this order, the contract of the phases in E11, and F4 and F6 as high-risk
steps that wait for a dated approval of their design.

Touches: E11, E21 (`engineering.md`, relocked); compiler phases: none (no
code change).

Risk: normal (documents and law text; no code).

Status: **implemented** (canon r3.110).

## What Changes

1. E11 gains the contract of the phases: what each produces and must never
   do; only the lowering phases and two named rewrites change the AST.
2. `docs/ARCHITECTURE.md` (status layer): the pipeline, the detailed
   contract of each phase (in, out, guarantees, never, owner), who may
   rewrite the AST, the deviations D1-D7 with the step that removes each,
   and the source map moved from `AGENTS.md`.
3. `AGENTS.md` "Start here" and E21 point to it; `config.yaml`: a compiler
   proposal names the phases it touches and updates the file.
4. ROADMAP: the foundation milestone before 0.5 (F0-F7).

## Out of scope

Every code change: F1-F7 are separate changes, in that order.
