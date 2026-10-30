# ADR-0005 — design record

> Imported verbatim from `docs/INOX_CANONICAL.md` (canon v3.48, lines 1805-1828) on 2026-10-12, approved by Marcelo Fortes, 2026-10-12: "Os ADRs viram mudanças arquivadas no OpenSpec."

## Rationale and consequences

Consequence: agents must update AGENTS.md, canonical docs, grammar, examples,
tests on any change; must not drift toward ObjectPascal/Java/Go/Rust/Python/C++
defaults unless explicitly accepted.

Historical portability note (v3.19; ADR-0005 wording above is unchanged): the
platform architecture now explicitly models Windows, Linux, macOS, the BSDs,
Illumos/Solaris, AIX, HP-UX, UnixWare, Android, and related targets. Support is
claimed only after validation; see E12, SECTION 29, and EH-v3.16a.
