## Why

The Inox constitution, E19 gates and G9 evidence rules govern compiler changes, but the generated OpenSpec apply skill does not enforce phase-aware execution, bounded test-output collection, or the distinction between regression evidence and an agent assertion. The missing integration encourages verbose logs, premature checkboxes, and unverified completion claims.

## What Changes

- Add a lightweight Inox-specific apply workflow alongside, rather than inside, the generated OpenSpec skill.
- Define path-scoped Claude Code rules referencing existing law, not restating language semantics.
- Extend project OpenSpec planning and apply guidance with precise regression evidence and explicit limits on runtime logs sent to the model.
- Provide a cross-platform, non-LLM verification command and a bounded-output Stop hook where a configured local toolchain exists; record full logs outside the model context.
- Document a review protocol in which another reviewer (including ChatGPT using GitHub PR/check data) independently checks diffs and gate evidence.
- Add deterministic self-tests for the verifier and explicitly record NOT_RUN and SKIP.

## Non-goals

No change to Inox syntax, ABI, ownership, compiler phase contracts or constitutional law. No new external specification framework, no automatic PR merge, no mandatory Claude subagent consuming extra tokens, no direct use of libLLVM C++ APIs, and no replacement of CI as the authoritative merge gate.

**Risk: normal** (developer workflow automation; no compiler or runtime modification).
**Status:** Implementation of a governance/tooling improvement requested by the maintainer; no G7 language-design approval is inferred or recorded.
