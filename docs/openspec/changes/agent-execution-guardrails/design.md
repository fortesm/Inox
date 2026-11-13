## Design and invariants

OpenSpec remains the only persistent source of requirements and delta specifications. Superpowers-style test-first and independent review practices are adopted as *execution discipline*, not another artifact store or competing plan. The generated `openspec-apply-change/SKILL.md` remains untouched so `openspec update` can replace it safely.

### Cost model

Claude receives a fixed-size decision summary: PASS/FAIL/SKIP/NOT_RUN per gate, counts, a log file path, and only bounded failure excerpts. Detailed stdout/stderr and evidence live under a gitignored build directory. Tests use ordinary subprocesses: no LLM interpretation of successful test cases.

### Responsibility

1. The maintainer owns G7, architectural decisions and all merges.
2. Claude implements code and focused tests from approved OpenSpec artifacts, respecting phase boundaries.
3. Local scripts and CI run deterministic validation and retain raw evidence; missing prerequisites are NOT_RUN, never PASS.
4. A reviewer other than the author examines the PR, architecture and gate evidence; ChatGPT may review using the GitHub connector, but it is not an unattended background service.
5. GitHub PR checks remain authoritative; a Stop hook is a convenience and cannot guarantee integration safety alone.

### Test-first policy

For a behavior change, reproduce the defect with a targeted diagnostic/semantic/TIR/IR/runtime test and observe RED against the old behavior when feasible; then GREEN and REFACTOR. For behavior-preserving refactors, use a before/after baseline and compare IR/TIR plus full regression. For tooling/doc changes, verify the tool/contract itself. Do not generate an artificial failing test to satisfy a ritual. Never weaken an existing test or broaden scope without approval.

### Phase isolation

Lexer/parser preserve the written AST. Normalization owns desugaring. Semantic analysis resolves meanings, mutability, generic instances and type operations. TIR carries resolved decisions. Code generation emits LLVM IR text without rediscovering types/names. libinoxrt owns runtime memory and console I/O. This design does not introduce `llvm::verifyModule()` or direct libLLVM linkage (E22).

### Hook and local verifier

The Stop hook delegates to a Python script and uses input `stop_hook_active` to avoid continuation loops. It detects tracked working changes, staged edits, newly created files and committed differences from the `main` merge-base; it must not use only `git diff HEAD`. It respects the local `.inox-machine` configuration without copying or committing its contents. The script writes full logs, emits at most a bounded compact summary, and caches a passing result only for the same content fingerprint and effective toolchain identity.

The hook is intentionally not a substitute for CI: if the machine is not configured, it records NOT_RUN rather than assuming another platform. Failure sends a bounded diagnostic and exit code 2 once; no infinite loop. Validation never claims complete E19 coverage when it only ran a quick local profile.

### Open questions excluded from this change

Enforcing `main` branch protection requires repository administration and is not silently enabled by editing developer files. The maintainer controls GitHub rulesets and required checks. Runtime-specific proof procedures remain in E19 and the relevant capability specs.

## Verification

- Static JSON, YAML and Markdown checks where local parsers exist.
- Python self-tests for file-change detection, cache invalidation, untracked files, missing machine and result classification.
- Local compiler build/full gates only on a machine with a configured `.inox-machine`; otherwise report NOT_RUN.
- OpenSpec strict validation in CI for this change; preserve published source of truth.
