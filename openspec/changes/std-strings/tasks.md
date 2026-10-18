## 0. Decisions

- [x] 0.1 Register the modules (canon v3.57, r3.58)
- [x] 0.2 Names, concatenation, positions and the module rule decided (2026-10-13; change `module-pairs`)
- [x] 0.3 Marcelo approves design.md (Risk: high) — 2026-10-13, canon r3.65

## 1. Runtime

- [x] 1.1 Block header (capacity, characters) for libinoxrt blocks; immortal blocks unchanged
- [x] 1.2 `tools/gen_unicode_tables.py` and `runtime/UnicodeTables.inc`
- [x] 1.3 `runtime/StringFunctions.cpp`; unit tests in `inoxrt-tests`

## 2. Compiler

- [x] 2.1 `+` on Strings: chain flattening, in-place append for `S := S + ...` and `S += ...`
- [x] 2.2 Std.Strings and Std.StrUtils intrinsics with module visibility; trap kind 15; `Position` (r3.76 reserved `Pos`)
- [x] 2.3 Fix found on the way: Strings read from State, `ref` parameters, pointer paths and `with` bindings are held for a statement that calls a routine; lexer: two empty literals

## 3. Tests, documentation, gates

- [x] 3.1 Runtime, trap and diagnostic tests per function; leak check (now instrumenting the generated code); 100 000-append test
- [x] 3.2 Constitution CANON-14, CANON-18, CANON-19; spec `stdlib/strings`; manual; STATUS; CHANGELOG
- [x] 3.3 E19 gates; MSVC IR check; Linux and Windows CI green (PR #66)
