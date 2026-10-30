## Why

Version 0.3 in `docs/ROADMAP.md` asks for the definitive console I/O ABI,
the decision on a function called as a statement, and what a text editor
needs. Until now `Put`/`PutLn` were the program's own `printf` and
`Get`/`GetLn` its `getchar`, while Strings and files lived in libinoxrt:
two C runtimes shared the console on Windows (AGENTS.md pitfall of v3.55),
the console was never set to UTF-8 there, and nothing could read a key or
move the cursor. A function called as a statement compiled and dropped its
result (the `function-result-discarded` gap of `tools/backend_gaps.py`).

Request (Marcelo Fortes, 2026-10-21): implement version 0.3 (numbers, text and the basic library): the definitive console I/O ABI, the decision on a function called as a statement, and what a text editor needs; ask about any decision to take

Approval (Marcelo Fortes, 2026-10-21), answering the decisions proposed to
him (E1, E2, F1, F2 below; the others belong to the next changes of 0.3):
E1 yes, E2 as in Ada, F1 yes with Std.Terminal as the name, F2 and F3 yes; separate PRs, easier to test.

Risk: high (the runtime ABI changes for every program: every program now
links libinoxrt, and console I/O moves into it).

Status: **implemented** (canon r3.88). F3, the micro-editor
`examples/edit.inox` that accepts 0.3, comes with the last change of 0.3,
when Std.SystemUtils gives it the command line.

## What Changes

1. **E1, the console is libinoxrt's.** `Put`, `PutLn`, `Get`, `GetLn` and
   Std.Terminal call libinoxrt (`__inox_con_*`, `__inox_term_*`); a
   program never calls `printf`, `getchar` or `fflush`. Every program links
   libinoxrt (the driver always links with `clang++`). Main first calls
   `__inox_runtime_init`, which on Windows sets the console to UTF-8 (input
   and output code pages) and enables VT processing, and gives both back at
   exit. Output keeps its order whatever prints it; before the program
   waits for input, pending output is flushed, so a prompt written with
   `Put` is seen. Console input on Windows is read as UTF-16 and given to
   the program as UTF-8; "\r\n" is one "\n" outside raw mode.
2. **E2, a function is not a statement (as in Ada).** Calling a function
   or a method with a result as a statement, with or without arguments, is
   a compile error: "the result of Twice is not used: a function is not a
   statement (r3.88); use its value, X := Twice(...)". There is no
   `Ignore`. This also holds for the library's functions (`UpperCase(S)`,
   `Create(Path)`, `ReadKey`).
3. **F1, Std.Terminal** (`Use Std.Terminal`): `RawMode(On)`, `ReadKey`
   (a `TKey`: `Kind` of `TKeyKind` — KeyChar, KeyUp, KeyDown, KeyLeft,
   KeyRight, KeyHome, KeyEnd, KeyPageUp, KeyPageDown, KeyEnter,
   KeyBackspace, KeyDelete, KeyTab, KeyEscape, KeyF1..KeyF12 — `Ch`, `Ctrl`,
   `Alt`), `ScreenSize` (a `TScreenSize`: `Rows`, `Cols`), `MoveTo(Row,
   Col)`, `ClearScreen`, `ClearLine`, `SetColor(Fg, Bg)` with the 16 colors
   of `TColor`, `ResetColor`, `ShowCursor(On)`, `Flush`. The types are
   declared in `stdlib/Std.Terminal.inox`; the routines are intrinsics
   visible only with the `Use`. The key names carry the `Key` prefix
   because `Char`, `Left`, `Right` and `Delete` are already names of Inox.
   When input is not a terminal, RawMode does nothing and ReadKey decodes
   the same bytes, so a program is tested with an input file; ScreenSize
   is 24 x 80 when output is not a terminal. ReadKey past the end of input
   stops the program with the new trap category "end of input" (kind 18);
   `MoveTo` with a row or column below 1 with "value out of range".
4. **F2, the terminal is given back** however the program ends: the end of
   Main, `Halt` (0.3, Std.SystemUtils), a trap, out of memory, an
   exception nobody handles, or SIGINT/SIGTERM/SIGHUP/SIGQUIT in raw mode:
   raw mode off, colors reset, cursor shown, Windows console modes and code
   pages restored. Nothing extra is written when nothing was changed.

## Capabilities

### New Capabilities
- `stdlib/terminal`: Std.Terminal.

### Modified Capabilities
- Constitution CANON-7 (a function is not a statement), CANON-17 (the
  console), CANON-18 (Std.Terminal), CANON-19 (trap "end of input";
  libinoxrt holds the console and every program links it).

## Impact

- libinoxrt: `runtime/ConsoleRuntime.cpp` (console, keys, terminal state),
  `runtime/Console.h`; the out-of-memory path restores the terminal.
- Codegen: `RuntimeAbi` gains the console and terminal functions and drops
  `__inox_str_getln` from the table (it stays in libinoxrt for its unit
  tests); the IR declares no `printf`/`getchar`/`fflush`; the fault
  functions call `__inox_runtime_shutdown`; `FunctionEmitterTerminal.cpp`.
- Analyzer: `SemanticTerminal.cpp`, `StdTerminal.h`; the expression
  statement rule (E2).
- Driver and test scripts: always `clang++` with libinoxrt.
- `tests/runtime/files-system.inox` closes the files it creates;
  `tools/backend_gaps.py` loses the `function-result-discarded` probe (now
  a rule, tested by the diagnostics).
- Every program's IR changes (console calls), so `tools/ir_snapshot.py`
  differs from main in every snapshot, as expected.
- AGENTS.md: the pitfall "Console I/O stays in the program" is replaced.

## Open point for the maintainer

E2 meets the rule that a local must be read (OPEN-4): a function called
only for its effect cannot be called as a statement, and `R := F(X)`
without reading R is "local variable never read". Ada answers with a
warning for an unread variable; Inox makes both errors. Today such a
function must be redesigned as a subroutine, or its result used. This
change leaves it so and asks the maintainer.
