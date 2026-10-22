## 1. Decisions

- [x] 1.1 Record Marcelo's answers of 2026-10-21 (E1, E2, F1, F2) as r3.88

## 2. Runtime and compiler

- [x] 2.1 libinoxrt console: output, input buffer, GetLn, init (Windows UTF-8 and VT), shutdown and terminal restore (atexit, terminate, signals, traps, out of memory)
- [x] 2.2 Std.Terminal in libinoxrt: raw mode (termios, Windows VT input), key decoding (UTF-8, Ctrl, CSI and SS3 sequences, xterm modifiers, Alt), screen size, ANSI output
- [x] 2.3 RuntimeAbi: console and terminal functions; every module declares the console; Main calls `__inox_runtime_init`; the fault functions call `__inox_runtime_shutdown`; no printf/getchar/fflush in the IR
- [x] 2.4 Driver and scripts link every program with libinoxrt (`clang++`)
- [x] 2.5 Analyzer E2: a function or method with a result as a statement is an error naming it
- [x] 2.6 Std.Terminal: stdlib/Std.Terminal.inox (types), analyzer (visible with Use, arguments checked), emitter (structs built from the packed key and size), trap kind 18

## 3. Tests

- [x] 3.1 Runtime: terminal-keys (with .in), terminal-output, terminal-restore-on-trap (trap, exact output), terminal-end-of-input (trap), terminal-names-free
- [x] 3.2 Diagnostics: function-result-discarded, function-result-discarded-bare, function-result-discarded-method, function-result-discarded-builtin, terminal-without-use, terminal-arguments, terminal-argument-type, terminal-no-arguments, terminal-readkey-discarded, terminal-name-clash
- [x] 3.3 Raw mode and restore checked in a pseudo-terminal by hand (stty before and after a trap are equal; keys are not echoed)

## 4. Documentation

- [x] 4.1 Constitution CANON-7, CANON-17, CANON-18, CANON-19, relocked; capability spec stdlib/terminal; revisions.md r3.88
- [x] 4.2 grammar.ebnf (unchanged syntax, noted); docs/site/index.html, CHANGELOG.md, docs/STATUS.md, docs/ROADMAP.md, AGENTS.md

## 5. Gates (E19)

- [x] 5.1 `bash scripts/run-tests.sh` passes
- [x] 5.2 ASan+UBSan build passes the suite with `ulimit -s 1024`
- [x] 5.3 `tools/mutation_fuzz.py` seeds 1 and 7 are clean
- [x] 5.4 `tools/backend_gaps.py` reports 0 BUG; `tools/string_leak_check.py` passes
- [x] 5.5 `tools/eh_state_balance.py`, `tools/calc_differential_test.py`, `tools/grammar_consistency.py`, `openspec validate --all --strict` and `tools/constitution_check.py check` pass
- [ ] 5.6 Linux and Windows CI green
