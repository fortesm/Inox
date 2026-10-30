## Why

The repository root mixed the build entry points with folders that belong
elsewhere: a one-file `ci/`, the runtime sources outside `src/`, test
runners in `scripts/` next to the checkers in `tools/`, four empty
placeholder folders, the build guide and the legal notices. Decided by
Marcelo Fortes, 2026-10-30, choosing among the options offered.

Risk: normal (paths only; the compiler, the runtime and the language do
not change).

Status: **implemented** (canon r3.104).

## What Changes

1. `ci/windows-known-failures.txt` → `tests/ci/`.
2. `runtime/` → `src/runtime/`, next to `src/compiler/`.
3. `scripts/` → `tools/scripts/`; `tools/` holds every development tool.
4. `BUILD_AND_TEST_INSTRUCTIONS.md` → `docs/BUILD.md`.
5. `AUTHORS.md`, `NOTICE.md`, `TRADEMARK.md` and `LICENSE.md` →
   `licenses/`; `LICENSE` stays at the root, where GitHub and the packages
   look for it.
6. `contrib/`, `include/`, `libs/` and `utils/` stay, each with a README
   saying what belongs there (a later decision of the same day: they are
   common in projects and may become useful); `tools/.gitkeep` and
   `.aiderignore` are removed.
7. The test runners run every program from a temporary directory, so the
   files tests create never land in the repository; build artifacts still
   go to `build/inox-artifacts`, and `INOX_STDLIB` points to the
   repository's library (from elsewhere, `tests/integration/stdlib` would
   be found first).

`.claude/` and `.github/` stay at the root: Claude Code and GitHub only
look for them there.
