## Why

Inox's last published release was `v0.1.0-test` (2026-06-02, made by
hand); 0.2 was never tagged, and 0.3 and 0.4 were merged without a version
number. Marcelo Fortes decided, on 2026-10-29:

- the version goes straight to 0.4.0; 0.3 is not released apart;
- the completed OpenSpec changes are archived;
- tags and releases follow the compiler by themselves, published when the
  version changes on main;
- the old `v0.1.0-test` release stays as history.

Risk: normal (CI, packaging and the version number; the language does not
change).

Status: **implemented** (canon r3.101).

## What Changes

1. **Version 0.4.0 pre-alpha.** Version.h and the top of revisions.md; the
   CMake project reads its version from Version.h. The CHANGELOG's 0.3
   section moves into 0.4.0, which is one section.
2. **Release workflow** (`.github/workflows/release.yml`). On every push to
   main it reads the version, and when `vMAJOR.MINOR.RELEASE` has no tag:
   - builds Linux and Windows in Release and runs the suite (Windows
     against its known-failure list);
   - packages, extracts each package into a clean directory, and runs
     `hello.inox` with the package's environment and `math-showcase.inox`
     with only PATH (E23);
   - creates the tag and the release, with the version's CHANGELOG section
     as notes and the stage in the title, as the latest release, which the
     manual's download links follow.

   A push that keeps the version publishes nothing; `v0.1.0-test` stays.
3. **Archive.** 34 completed changes archived with `--skip-specs`: their
   deltas were already in the specs, and re-applying an old MODIFIED delta
   would undo later changes. Their unticked tasks were "CI green" (all
   merged; main green) and the `canon-final` tag (present on GitHub).
4. **Fixes found by the release tests.** The packaged compiler run through
   PATH took the directory it was run from for its own and missed
   libinoxrt; it now asks the operating system. `package-release.ps1`
   still copied the law from `openspec\`, and takes `-CompilerExe`.
5. **Law and docs.** VERSIONING gains "Releases follow the version"
   (relocked); revisions.md r3.101; STATUS, ROADMAP, AGENTS.md, site.

## Not changed

Four changes keep their deltas, never folded into the specs:
`design-by-contract`, `method-reuse`, `safe-pointers` and `debug-assert`.
Folding them needs a review against the constitution (CANON-9, 24, 25 and
18), which states their law today; this is a later task.
