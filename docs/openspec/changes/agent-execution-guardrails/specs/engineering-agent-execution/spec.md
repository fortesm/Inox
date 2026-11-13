## ADDED Requirements

### Requirement: Agent implementation is grounded in Inox authority

An implementation agent SHALL treat the locked constitution, capability specs and the explicit OpenSpec change as authoritative; it SHALL report ambiguity and unapproved language behavior rather than inferring semantics from another programming language.

#### Scenario: Missing language decision
- **WHEN** a requested compiler change depends on behavior not defined by the approved OpenSpec change and constitution
- **THEN** the agent records the missing design decision, does not implement a guessed interpretation and reports the blocker (test to add; procedural review).

### Requirement: Verification output is bounded without losing evidence

Local automated verification SHALL execute without sending full successful test logs to an LLM; it SHALL preserve complete logs outside the model context and emit a bounded summary of checks, counts, failures and evidence paths.

#### Scenario: Successful run
- **WHEN** all requested checks execute and pass
- **THEN** the CLI summary reports PASS, the number of checks run and a retained log path, without dumping one PASS line per test (tools/tests/test_agent_validation.py: test_quick_validation_is_cached_and_keeps_full_log; full Inox suite evidence in CI).

#### Scenario: Partial environment
- **WHEN** a prerequisite or configured machine identity is missing
- **THEN** the CLI reports NOT_RUN or SKIP explicitly and never promotes a partial result to complete E19 coverage (tools/tests/test_agent_validation.py: test_machine_missing_and_placeholder_are_not_faked and test_missing_machine_is_not_success_in_explicit_verify).

### Requirement: Implementation evidence matches the change kind

Behavior changes SHALL be verified by focused reproductions and regression coverage. Behavior-preserving refactors SHALL compare a before/after baseline including TIR/LLVM IR where E19 applies. Tooling and documentation changes SHALL validate their own effects. No task is complete on an unverified report alone.

#### Scenario: Refactor without semantic changes
- **WHEN** a change is classified as behavior-preserving
- **THEN** the evidence records an earlier baseline and an exact comparison, rather than fabricating a behavior change to create a RED test (test to add).

### Requirement: Phase-specific guidance preserves compiler boundaries

Path-scoped instructions SHALL reference the existing canonical phase contracts without inventing a second language specification or introducing libLLVM-specific requirements for a textual-IR backend.

#### Scenario: Code generation edit
- **WHEN** the agent edits code generation
- **THEN** it uses pre-resolved TIR information and reports a missing semantic decision upstream, instead of re-resolving type or member names in codegen (test to add; phase-contract review).

### Requirement: Local automation cannot bypass maintainer and CI authority

A local Stop hook SHALL be advisory enforcement for the current machine and shall not declare a change merged, globally verified or approved. The sole maintainer controls merges and architectural decisions; CI and independent PR review remain required.

#### Scenario: Missing local validation
- **WHEN** a machine cannot run the configured test command
- **THEN** the output records NOT_RUN with its reason and requires CI evidence before any implementation-complete claim (test to add).
