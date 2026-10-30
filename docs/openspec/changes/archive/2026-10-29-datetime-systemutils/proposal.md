## Why

Last change of 0.3 (`docs/ROADMAP.md`): dates and times, the program's
system (command line, environment, exit status), and the program that
accepts 0.3 — a micro-editor built with what 0.3 gives.

Request (Marcelo Fortes, 2026-10-21): implement version 0.3: Std.DateTime, Std.SystemUtils, and what a text editor needs

Approval (Marcelo Fortes, 2026-10-21, repeated on 2026-10-22): D1 to D3 approved; D4: `ParamCount`, `ParamStr(I)`, `GetEnv(Name)`, `EnvExists(Name)`, `Halt(Code)` to exit with a code, `Sleep(Ms)`, `CurrentDir` and `PlatformName`; F3 yes; separate PRs, easier to test.

Risk: normal (new modules mostly written in Inox, a few runtime functions;
Main now receives the command line, which changes the signature of every
`main` in the IR).

Status: **implemented** (canon r3.92).

## What Changes

1. **D1, D2 — Std.DateTime.** `TDateTime` (microseconds since 0001-01-01,
   proleptic Gregorian), `TDate` (days), `TDuration` (microseconds) are
   Ranges of Integer, so they compare with the usual operators;
   `TDayOfWeek` (Monday first). `NowUTC` and `UTCOffset(T)` are carried out
   by libinoxrt; `Now`, `ToLocal`, `ToUTC` and the rest are Inox in
   `stdlib/Std.DateTime.inox`.
2. **D3.** `EncodeDate`/`EncodeDateTime` checked by preconditions,
   `DecodeDate`, `Year`, `Month`, `Day`, `Hour`, `Minute`, `Second`,
   `MicroSecond`, `DayOfWeek`, `DateOf`, `DateTimeOf`, `AddDays`,
   `AddSeconds`, `DaysBetween`, `IsLeapYear`, `DaysInMonth`. Std.DateUtils
   (Inox): `ToISO8601`, `FromISO8601` (ConversionError on bad text),
   `FormatDateTime(T, "dd/mm/yyyy hh:nn")` with Delphi's codes (`mm` month,
   `nn` minutes, `zzz` milliseconds, quoted text).
3. **D4 — Std.SystemUtils** (intrinsics): `ParamCount`, `ParamStr(I)`,
   `GetEnv(Name)`, `EnvExists(Name)`, `Halt(Code)` (0..255), `Sleep(Ms)`,
   `CurrentDir`, `PlatformName`; UTF-8 on every system (Windows reads the
   wide command line, environment and directory). Main receives `argc`
   and `argv`; `inox run file.inox A B` gives A and B to the program and
   ends with the program's exit status.
4. **F3 — `examples/edit.inox`**: a micro-editor in the manner of mg, one
   buffer of up to 2000 lines, the file named on the command line, Emacs
   keys (Ctrl+F/B/N/P/A/E/V/D/K, Alt+V, arrows, Home/End, PageUp/PageDown,
   Enter, Backspace, Delete, Tab), Ctrl+X Ctrl+S to save, Ctrl+X Ctrl+C to
   quit (twice when modified); tested with keys from a file.
5. The table of library intrinsics (`StdIntrinsics.h`, formerly
   `StdTerminal.h`) carries the module of each routine.

## Capabilities

### New Capabilities
- `stdlib/datetime`: Std.DateTime and Std.DateUtils.
- `stdlib/systemutils`: Std.SystemUtils.

### Modified Capabilities
- Constitution CANON-18 (library modules; planned modules), CANON-23
  (program arguments and exit status).

## Impact

- stdlib: `Std.DateTime.inox`, `Std.DateUtils.inox` (Inox code),
  `Std.SystemUtils.inox` (documentation of the intrinsics).
- libinoxrt: `runtime/SystemRuntime.cpp`; `__inox_runtime_init(argc,
  argv)`; a fault helper for library faults.
- Compiler: intrinsic table by module; `main(i32, ptr)`; the driver passes
  arguments and keeps the program's exit status.
- Tests and scripts: the IR fragments of `main` and the init call; a
  driver test of arguments; `module-planned-not-found` became
  `module-not-found` (Std.Process).

## Open points for the maintainer

- The names `Year`, `Month`, `Day`, `Hour`, `Minute`, `Second` (D3) and
  the rule that forbids shadowing (CANON-10) mean that a program using
  Std.DateTime cannot name its variables Year or Month. Delphi's DateUtils
  calls these `YearOf`, `MonthOf`, ...; keep D3's names or rename?
- TDateTime, TDate and TDuration are Ranges of Integer (they compare with
  `<` and store checked), so nothing stops `T + D` of a TDateTime and a
  TDate; nominal types with operators would need operator definitions,
  which Inox does not have.
- The language version stays 0.2.0 pre-alpha; with this change the list of
  0.3 is done. Raise it to 0.3.0 when you decide.
