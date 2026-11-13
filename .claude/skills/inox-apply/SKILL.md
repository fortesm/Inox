---
name: inox-apply
description: Inox-specific implementation of an approved OpenSpec change, with test-kind evidence, compiler phase rules and minimal Claude token use. Prefer over the generic generated apply skill for Inox compiler work.
---

# Implement an approved Inox change

Do not create a second plan or fork a specification framework. **OpenSpec is the only persistent change specification**. This skill adds the execution discipline of test-first development and independent review without installing Superspec or Superpowers.

1. **Grounding, then scope.** On a local checkout inspect `.inox-machine` restrictions and `git status` before edits; inspect `AGENTS.md`, the applicable locked law, `docs/ARCHITECTURE.md` phase and OpenSpec change. From `docs/` use `openspec status/instructions apply --change <name> --json`. Never create an OpenSpec root at repository top level. Only implement approved tasks. On G4 ambiguity or high-risk design without dated approval, stop; do not guess.
2. **Test-kind evidence.** Behavior change: focused diagnostic/parser/semantic/TIR/LLVM/runtime regression, RED on previous implementation where feasible, then GREEN and REFACTOR. Pure refactor: record baseline and compare LLVM+TIR snapshots; do not manufacture failing tests. Tooling/doc update: validate the changed tooling. Do not edit unrelated files to make a test pass.
3. **Phase fidelity.** Use path-specific `.claude/rules/` and the canonical phase contract; no parser desugaring, no semantic inference inside codegen, no direct libLLVM APIs. Ownership, contracts and cross-platform ABI are never casually simplified.
4. **Low-token verification.** Invoke `python tools/agent_validation.py verify --profile quick` when a configured machine can execute it; full local `--profile full` and applicable E19 gates before a completion claim. Detailed stdout/stderr go to files; read only bounded FAIL excerpts, never one PASS per test. Retain separate PASS/FAIL/SKIP/NOT_RUN evidence and exact platform. CI is definitive for merge, not a substitute for reporting missing local checks.
5. **Review and acceptance.** Match every task to tests/actual evidence before `[x]`; no claim based only on prose, no silent scope shrink. An independently reviewing agent or ChatGPT can inspect PR/checks **after** code is produced; do not spawn token-expensive Claude subagents by default. Only the maintainer merges into `main`.

Return succinctly: change/commit, changed modules, behavioral evidence, gate status, remaining NOT_RUN and explicit approval questions. Do not paste full logs or reiterate the constitution.
