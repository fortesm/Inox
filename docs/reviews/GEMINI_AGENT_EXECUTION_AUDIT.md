<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Review of Gemini's OpenSpec, Superpowers and Claude Code recommendations

**Status:** Engineering research and implementation rationale, **not Inox language law**.
**Date:** 2026-11-13.
**Input:** the 418-line conversation export *Chat aobout Spec and implementation.txt* supplied to the review.
**Repository baseline:** \`main\` at \`82477abb1c1fb2ed018321848bf7876d71e66c8c\`.
**Scope:** every substantive section of the supplied conversation, cross-checked against real repository files. Line ranges are references to the supplied source file; overlapping/adjacent statements are grouped where they have one technical conclusion.

**Important provenance limit:** The source was one conversation export. No separate second uploaded text containing all repository governance files was available to the reviewer. The repository's actual \`AGENTS.md\`, \`docs/openspec/config.yaml\`, constitution, \`engineering.md\`, generated apply skill, \`gates.yml\`, \`ci.yml\`, \`run-tests.ps1\`, build presets and related scripts were read **directly from GitHub**. Claims in the Gemini exchange that were made before that access should not be treated as verified repository findings.

## Criteria

- **ADOPT:** The idea is technically appropriate and implemented or specified in this change.
- **ADAPT:** Useful idea, but correct for real Inox architecture or practical token costs first.
- **REJECT:** False for this compiler, disproportionate cost, or conflicts with the authoritative workflow.
- **CONTEXT:** Conversation context, alternative suggested by another party, or repetition; not a requirement.
- **UNVERIFIED:** An ecosystem/benchmark assertion not established by repository evidence.

## Line-range audit of the entire supplied exchange

| Source lines | Claim or recommendation | Decision | Result for Inox |
|---|---|---|---|
| 1–3 | OpenSpec handles *what* to build, Superpowers handles *how* to execute | ADOPT | Keep OpenSpec as the only persistent specification; integrate its useful execution discipline. |
| 4–13 | Comparison of specification files, skills, TDD and reviews | ADAPT | These are design patterns, not enforcement guarantees. Extra subagents incur Claude token costs. |
| 14–20 | OpenSpec prevents specification drift while Superpowers handles skipped tests | ADOPT | Apply to change type: behavior, refactor, tooling; respect G7 even for apparently small language changes. |
| 21–27 | Use both tools or a bridge without duplicate planning | ADOPT | Do not add another plan, another task list, or a competing constitution. |
| 28–40 | Superspec and other bridges solve integration | UNVERIFIED | Their comparative benefits were not independently established; no dependency is needed for the current gap. |
| 41–56 | Replace or complement with GSD, BMAD, Spec Kit | REJECT | Migration would duplicate existing governance and create unnecessary drift risk. |
| 57–70 | A unique compiler syntax demands explicit grammar and phase-specific tests | ADOPT | Use approved grammar, CANON, TIR, diagnostics, LLVM emission and runtime suite. |
| 71–82 | Introduce syntax/composition/LLVM specification directories | ADAPT | The OpenSpec root is **docs/**; extend existing capability specs and changes, not another top-level tree. |
| 83–106 | C++ compiler should pin libLLVM API, Opaque Pointers, \`llvm::verifyModule()\` | REJECT | E22 currently uses **textual LLVM IR through external Clang**. No direct libLLVM API requirement. |
| 107–118 | Brainstorm, OpenSpec proposal, RED/GREEN/REFACTOR, review | ADAPT | Test-first for observable behavior; refactors need a baseline and \`ir_snapshot.py\` (including TIR), not contrived RED tests. |
| 119–143 | Swap tools to SDD-Superpowers/ECC/EARS/Spec Kit | REJECT | Existing G1–G9, E1–E23 and OpenSpec are more targeted. Do not duplicate frameworks. |
| 144–158 | Tool comparison and migration guidance | ADAPT | Preserve transferable practices only; do not replace repository-controlled OpenSpec. |
| 159–170 | Regressions require unambiguous requirements and diagnostics | ADOPT WITH CORRECTION | The existing SHALL/WHEN/THEN scenarios already express testable requirements; example diagnostic \`E0104\` was hypothetical, not a real Inox error code. |
| 171–172 | Freeze libLLVM C++ snippets | REJECT | Refer to the existing textual emitter, runtime ABI and TIR semantics instead. |
| 173–177 | Add llvm-lit/FileCheck and a PostToolUse full-build gate on each edit | ADAPT | Reuse actual test harnesses and snapshots; full suites after every edit waste compute/context and can cause loops. |
| 178–186 | Sanitizers, ownership checks, verifier calls and global \`-Werror\` | ADAPT | E9/E18/E19 already define required safety evidence; do not silently mandate libLLVM or alter build warnings policy. |
| 187–197 | Install Superpowers and require hook/subagent review | ADAPT | Local rules, dedicated skill and independent reviewer without mandatory extra Claude agents. |
| 198–208 | Project architecture is separated into lexer/parser/normalization/semantic/TIR/codegen | ADOPT | Verified; enforce E11 phase ownership and typed codegen boundary. |
| 209–213 | Complex semantics plus absent local hooks explain some regression risk | ADOPT | \`.claude/\` on the verified main had commands and skills but no project \`settings.json\` or path-scoped rules. |
| 214–220 | Strict phase-by-phase execution | ADAPT | Verify each phase but do not ban implementation of an approved cross-phase feature in one coherent PR. |
| 221–225 | Require negative tests and LLVM IR evidence | ADAPT | Appropriate for new semantic rules; refactors and documentation need different evidence. |
| 226–242 | Initial repository observations before full access | CONTEXT | Gemini subsequently acknowledged incomplete access. Validate every project-specific premise before acting. |
| 243–257 | Path-scoped guidance for frontend, semantics and backend | ADOPT | Three short \`.claude/rules/\` files, referring to CANON/E/ARCHITECTURE, without duplicating law. |
| 258–266 | Four hard phase gates and small tasks | ADAPT | Cohesive change, deterministic tests and no explosion of PRs or microreleases. |
| 267–273 | A Windows-only Stop hook running the entire suite after edits | ADAPT | Cross-platform helper using \`.inox-machine\`, content-aware caching and bounded output. The hook is not equivalent to CI. |
| 274–281 | RAII, explicit includes, no accidental phase coupling, C++ SonarCloud | ADOPT WITH CORRECTION | Those rules are already in E9/E11/E18; SonarCloud is additional evidence, not semantic proof. |
| 282–301 | ZIP file tree and active changes as architectural diagnosis | ADAPT | Listing a file is not evidence of its contents; existing active OpenSpec deltas must not be mistaken for implemented law. |
| 302–343 | Concatenate large files to upload to the agent | CONTEXT | Useful when no connector exists; GitHub access avoids re-uploading the constitution or sending huge files to Claude. |
| 344–354 | Final diagnosis: existing governance is passive and generic apply lacks local gates | ADOPT WITH CORRECTION | Generated apply already includes blockers and scope controls; missing automated execution and bounded evidence remain real. |
| 355–365 | Replace generated apply skill and forbid code edits without RED | ADAPT | Keep generated skill intact, add non-generated \`inox-apply\`; test-first by change type. |
| 366–376 | Enforce E11 boundaries and E19 checks | ADOPT | Explicit rules plus actual gate evidence; no checkbox on unexecuted tests. |
| 377–387 | Three \`paths:\` rules | ADOPT | Added frontend, semantics/TIR, backend/runtime files using official phase contracts. |
| 388–407 | Exact inline \`Stop\` PowerShell snippet | REJECT IMPLEMENTATION | Hardcodes Windows; \`git diff HEAD\` misses untracked and committed changes; \`--target inox\` may omit unit-test executables; results may be incomplete; requires safe loop handling. |
| 408–418 | Independent engineering review with E1–E18/G4 checklist | ADAPT | External PR review is valuable; a mandatory additional Claude reviewer costs tokens and cannot replace deterministic tests. |

## Repository-grounded facts and resulting design

### Governance already present

\`AGENTS.md\` enforces G9 **evidence, not reports**, \`git status\`,
\`.inox-machine\` identity, G7 approval, PR-only workflow, and independent
review. The constitutional \`spec.md\` and \`engineering.md\` define
authority and compiler phase rules; \`constitution.lock\` detects
unapproved changes. None of these should be recreated in shorter
informal pseudo-constitutions.

\`docs/openspec/config.yaml\` already required E19 and G9 under its tasks
rules, but before this change it lacked specific **apply-operation guidance**.
The generated \`.claude/skills/openspec-apply-change/SKILL.md\` had task state,
blockers and scope instructions, yet no actual local E19 enforcement.
It should not be replaced wholesale: it is OpenSpec-generated.

### Verification and backend facts

\`.github/workflows/ci.yml\` runs the compiler and tests on Linux/Windows
and checks OpenSpec, constitution, backend, exception, memory and version.
\`.github/workflows/gates.yml\` supplies sanitizers, mutation fuzzing,
differential arithmetic, IR/TIR snapshots for refactors and stack-budget
checks. \`run-tests.ps1\` may print **[SKIP]** where test executables or
prerequisites are absent; an exit code of zero is not proof every suite
actually ran. E22 deliberately uses textual LLVM IR with external Clang
and does not permit introducing direct libLLVM verification by analogy.

### Why Gemini's inline hook is insufficient

- It misses untracked files and can miss already committed branch edits.
- It compiles only a single target, potentially leaving unit tests absent.
- It assumes the development machine is Windows instead of consulting
  the local \`.inox-machine\` restrictions and build preset.
- It can dump many \`[PASS]\` lines into the model context.
- It does not establish all E19 gates or protect \`main\` at merge time.
- A Stop hook can be bypassed or be unable to execute; it is not a
  substitute for branch protection and required GitHub checks.

## Implemented inside PR #156

- **OpenSpec:** \`docs/openspec/changes/agent-execution-guardrails/\`
  with proposal, design, requirement delta and tasks;
  \`docs/openspec/config.yaml\` extended with compact guidance.
- **Claude Code:** new path-scoped rules, a dedicated non-generated
  \`.claude/skills/inox-apply/SKILL.md\`, and \`.claude/settings.json\`.
- **Verifier:** \`tools/agent_validation.py\`, with full logs outside the
  model, short pass/fail summaries, explicit partial or unavailable
  checks and cache invalidation when source/toolchain inputs change.
- **Self-tests:** \`tools/tests/test_agent_validation.py\` and Linux/Windows
  CI jobs; a test-specific verification result is not a compiler-quality
  gate result.
- **Operator instructions:** \`docs/AGENT_EXECUTION.md\` and a short
  reference from \`AGENTS.md\`.

## Further obligations and known limitations

1. **Do not confuse code creation with validation:** local verifier
   self-tests were reported as 6 PASS. Full Linux/Windows compiler suites,
   strict OpenSpec validation and E19 gates must be judged from the PR's
   actual CI; none are declared complete by this report alone.
2. **The workflow must stay cost-aware:** preserve full logs for auditing,
   send only bounded failures to Claude, and avoid mandatory additional
   Claude subagents.
3. **A Stop hook cannot enforce maintainer authority:** G7 and PR review
   still apply. Protecting \`main\` with required status checks and approved
   reviewers is a separate repository administration action.
4. **Do not infer language design:** G1/G4 apply to every change. Compiler
   semantics, CANON-9 delegation, ownership and E22 backend policy remain
   unchanged by these workflow updates.
5. **Trace source accuracy:** the Gemini conversation contained plausible
   but unverified claims about external tooling. This review does not
   endorse ecosystem rankings or a guarantee of defect-free code.

**Summary:** Adopt Superpowers-style verification *discipline*, not a
second framework, second constitution or second task plan. Use OpenSpec
for authority, Claude for focused implementation, deterministic tools/CI
for evidence, and independent PR review (including ChatGPT on request)
for engineering judgment.
