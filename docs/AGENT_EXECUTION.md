<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# AI-assisted compiler development: low-token execution and evidence

This is operational guidance, not an amendment to Inox language law.
**Authoritative:** `docs/openspec/specs/constitution/spec.md`,
`engineering.md`, the approved capability specs, and the change's
`docs/openspec/changes/<name>/` artifacts. Conflicts go to the maintainer
(G1/G4/G7). The existing generated `/opsx:apply` skill remains intact.

## Goals and boundaries

- Minimize Claude Code token use: spend its context on C++20 implementation,
  approved Inox syntax, diagnostics and narrow source-level tests.
- Let deterministic scripts and CI run hundreds of tests without emitting
  one PASS line per test into an LLM context. Never hide failures, skips or
  prerequisites not met.
- Avoid new overlapping workflows: **OpenSpec** owns the change definition,
  the **Inox apply skill** owns execution discipline (test-first when useful),
  **E19/CI** owns verification, and an **independent reviewer** owns review.
- No automatic merge, no independent architectural decision by AI,
  no claim of impossibly perfect or zero-regression code.

## Standard workflow

1. **Local workspace and change.** Check `git status` and machine identity
   in `.inox-machine` (never commit that file). Identify the explicitly
   approved OpenSpec change. From `docs/`, use `openspec instructions apply
   --change NAME --json`. Read relevant approved law and capability
   rules, not unrelated manuals or the complete test output repeatedly.
2. **Reproduction/baseline.** For behavioral bugs, add the minimum focused
   failing test first when possible. Inox test families include
   `tests/diagnostics`, `tests/parser`, `tests/tir`, `tests/runtime`,
   `tests/codegen` and `tests/compiler-unit`. Use only valid Inox
   grammar. For pure refactors, record the initial TIR/LLVM IR snapshot
   before edits instead of manufacturing a RED test.
3. **Implementation.** Follow E11's phase ownership. Keep changes cohesive,
   avoid speculative abstractions, include required C++20 headers, preserve
   RAII, exceptions, ownership, Windows/MSVC ABI and textual IR.
   Use relevant path-specific rules in `.claude/rules/`.
4. **Fast local evidence.** On a configured machine, run:
   `python tools/agent_validation.py verify --profile quick`.
   This builds the configured preset and runs `.inox-machine`'s
   `test_command`, collecting complete output under
   `build/agent-validation/` and returning only an abbreviated summary.
   For additional local validation run `--profile full`, which adds
   constitution, grammar and version checks. **Neither profile equals
   all E19 gates**; sanitizer builds, differential tests, fuzzy seeds,
   backend, exception/owner leak checks, Windows/Linux and snapshot
   comparisons still apply independently through E19/CI.
5. **Review and promotion.** Ensure all applicable tests and full E19/CI
   evidence are available; a second reviewer independently checks the
   PR diff and results. The ChatGPT GitHub connection can inspect PRs,
   check runs and available artifacts on request; it does not monitor
   them in the background automatically. Only the maintainer merges.

## Stop hook behavior

`.claude/settings.json` uses the supported exec-form `Stop` hook. It
runs **quick** verification only when compiler, standard-library, tests
or build files differ from the local `main` merge-base, including
committed, staged, unstaged and untracked changes. It does not re-run
unchanged successful checks: it fingerprints relevant file content and
machine settings. A real failure emits exit code 2 and only a bounded
error excerpt, then writes its full log on disk. `stop_hook_active`
prevents an unrepairable repeated block.

If the checkout has no `main` baseline or no correctly configured
`.inox-machine`, it reports **NOT_RUN**, not PASS. If a runner prints
`[SKIP]`, the result is **PARTIAL**, not complete validation. Hook
execution and return codes are dependent on Claude Code installation
and project trust. A missing Python executable or hook failure cannot be
construed as a green suite.

**A green local hook is not a merge gate.** GitHub Actions and a
protected branch/ruleset should enforce reviewer approvals and checks.
A PR can still need explicit design approval, test evidence and CI.
The maintainer must configure GitHub protection separately; no branch
protection is silently changed by this document.

## Token economy policy

- Successful commands: report one line per *gate*, not per test.
- Failures: show at most eight relevant error lines (bounded characters)
  and a path to full logs. Read only targeted ranges of that log.
- Never ask the agent to summarize thousands of `[PASS]` messages.
- Avoid resending the complete constitution, repeated design proposals
  or compiler source files on every task; consult the specific law anchors
  and symbols needed. The law still controls even if its full text is
  not pasted repeatedly.
- Do not spawn another Claude subagent by default: that consumes additional
  model tokens. Independent review can happen externally via the PR.
- Report `PASS`, `FAIL`, `SKIP`, `PARTIAL` and `NOT_RUN` distinctly.
  G9 permits no implementation-complete claim based only on an LLM report.

## Tests for the local verification helper

Run `python -m unittest discover -s tools/tests -p test_agent_validation.py`.
These tests verify committed/staged/untracked change discovery, missing
machine identity, content-sensitive caching and honest incomplete reporting.
They do **not** build the Inox compiler or replace the real suite.
