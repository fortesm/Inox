## Why

Foundation step F1 (ROADMAP; `docs/ARCHITECTURE.md` deviation D7). The
quality gates of E19 were run by the agent for each pull request, on a
temporary machine, so their results left no reproducible record; the
external reviews of 2026-10-31 found that CI did not run the sanitized
suite, the mutation fuzz or the differential test, and that the version
written in the README had drifted from `Version.h`.

Approval: Marcelo Fortes, 2026-10-31 (foundation milestone F0-F7).

Touches: E19 (`engineering.md`, relocked). Compiler phases: none (CI and
tools only).

Risk: normal (workflows and a check tool; no compiler or runtime change).

Status: **implemented** (canon r3.111).

## What Changes

1. `.github/workflows/gates.yml`: the suite on a g++ ASan+UBSan build with
   a 1 MiB stack; `tools/mutation_fuzz.py` on that build, seeds 1 and 7, no
   CRASH, TIMEOUT or CGERR, findings kept; `tools/calc_differential_test.py`
   with seeds 1 and 7; and, on a pull request labelled `refactor`,
   `tools/ir_snapshot.py` recorded with the base and compared with the
   head, the programs the pull request adds allowed. It runs on pull
   requests that touch code, tests or tools, on pushes to main, every night
   at 04:17 UTC and by hand; every job uploads its log.
2. `tools/version_check.py`, run by the CI workflow: the version and the
   stage of `Version.h` are the ones of `revisions.md`, `README.md`, a
   `CHANGELOG.md` section and `docs/STATUS.md`, and the canon revision is
   the one of `revisions.md`.
3. `tools/ir_snapshot.py`: `--allow` also accepts a program that appears or
   disappears.
4. E19 says where each gate runs; D7 is closed in `docs/ARCHITECTURE.md`.

## Out of scope

clang-format, clang-tidy, cppcheck, coverage (E19 "still to come"); any
compiler change.
