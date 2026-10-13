## 0. Decisions

- [x] 0.1 Register the modules (canon v3.57, r3.58)
- [x] 0.2 Names, concatenation, positions and the module rule decided (2026-10-13; change `module-pairs`)
- [ ] 0.3 Marcelo approves design.md (Risk: high)

## 1. Runtime

- [ ] 1.1 Block header (capacity, characters) for libinoxrt blocks; immortal blocks unchanged
- [ ] 1.2 `tools/gen_unicode_tables.py` and `runtime/UnicodeTables.inc`
- [ ] 1.3 `runtime/StringFunctions.cpp`; unit tests in `inoxrt-tests`

## 2. Compiler

- [ ] 2.1 `+` on Strings: chain flattening, in-place append for `S := S + ...` and `S += ...`
- [ ] 2.2 Std.Strings and Std.StrUtils intrinsics with module visibility; trap kind 15

## 3. Tests, documentation, gates

- [ ] 3.1 Runtime, trap and diagnostic tests per function; leak check; 100 000-append test
- [ ] 3.2 Constitution CANON-14, CANON-18, CANON-19; spec `stdlib/strings`; manual; STATUS; CHANGELOG
- [ ] 3.3 E19 gates; MSVC IR check; Linux and Windows CI green
