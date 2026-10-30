## Why

Test and example programs, and the code illustrations of the documents,
used Portuguese names and texts (`TConta`, `FSaldo`, `Depositar`,
`"saldo nunca negativo"`). Decided by Marcelo Fortes, 2026-10-30, after
the records became English (change `decisions-not-dialogue`): translate
them too.

Risk: normal (names and texts of tests, examples and documents; the
compiler, the runtime and the language do not change).

Status: **implemented** (canon r3.103).

## What Changes

1. **Names.** About 130 Portuguese identifiers in 120 test and example
   programs become English (`TConta` → `TAccount`, `FSaldo` → `FBalance`,
   `TNo` → `TNode`, `Copas` → `Hearts`), avoiding Inox's reserved and
   predefined names (`New`, `Move`, `Value`, `Fill`, `old`), checked file by
   file so that no new name meets an existing one. Every spelling keeps its
   case pattern, so the tests of case-insensitive names still compare
   `Amount` with `amount`.
2. **Texts.** Portuguese strings and comments of the programs, with their
   expected outputs, traps and diagnostics, become English.
3. **Documents.** The code illustrations of the constitution, the specs,
   the changes, the history, the site and the CHANGELOG use the same names.

## Not changed

- Names of people and places (Ana, Maria, José, Recife) and Unicode test
  data (`"água😀"`, `"ção"`, accented input lines) stay: they are data, and
  the accents are what the tests check.
- Every test keeps what it proves; the suite passes unchanged in count.
