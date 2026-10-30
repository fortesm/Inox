# stdlib/terminal Specification

## Purpose
The screen and the keyboard of a text program, such as an editor, in the
`Std.Terminal` module: raw keys, cursor, colors, screen size, and the
terminal given back at exit. Part of the Inox law, detailing the
constitution (CANON-17, CANON-18).

## Requirements

### Requirement: Std.Terminal is visible only with Use
`Use Std.Terminal` SHALL make visible the types `TKey`, `TKeyKind`,
`TScreenSize` and `TColor` (declared in `stdlib/Std.Terminal.inox`) and the
routines `RawMode`, `ReadKey`, `ScreenSize`, `MoveTo`, `ClearScreen`,
`ClearLine`, `SetColor`, `ResetColor`, `ShowCursor` and `Flush`. Without the
`Use` the names are free; with it, a routine of the program with one of
these names is a duplicate.

#### Scenario: Names
- **WHEN** a program calls ClearScreen without the Use, declares its own Flush without it, or declares Flush with it
- **THEN** the first is rejected (tests/diagnostics/terminal-without-use.inox), the second runs (tests/runtime/terminal-names-free.inox) and the third is rejected (tests/diagnostics/terminal-name-clash.inox)

### Requirement: Calls are checked
A routine without arguments SHALL be called without parentheses
(`ClearScreen`, `K := ReadKey`); `MoveTo` takes two Integers, `SetColor`
two TColor values, `RawMode` and `ShowCursor` one Bool. `ReadKey` and
`ScreenSize` are functions, so their result is used (CANON-7).

#### Scenario: Wrong calls
- **WHEN** a program writes `MoveTo(1)`, `SetColor(1, 2)`, `ClearLine(1)` or `ReadKey` alone
- **THEN** each is rejected (tests/diagnostics/terminal-arguments.inox, tests/diagnostics/terminal-argument-type.inox, tests/diagnostics/terminal-no-arguments.inox, tests/diagnostics/terminal-readkey-discarded.inox)

### Requirement: Keys
`ReadKey` SHALL wait for the next key and return a `TKey`: `Kind` (KeyChar,
the arrows, Home, End, PageUp, PageDown, Enter, Backspace, Delete, Tab,
Escape, F1..F12), `Ch` (the character for KeyChar, else `'\0'`), `Ctrl`
and `Alt`. Ctrl+A..Ctrl+Z are 'a'..'z' with Ctrl, Ctrl+Space is ' ' with
Ctrl, Escape then a key is that key with Alt.

#### Scenario: Keys from bytes
- **WHEN** the input holds a letter, Ctrl+A, arrows, PageUp, F1, Alt+x, "é", Enter, Tab, Backspace, Delete, Ctrl+Right, F12, a space, Ctrl+Space and a lone Escape
- **THEN** ReadKey returns each key with its modifiers (tests/runtime/terminal-keys.inox)

### Requirement: Raw mode
`RawMode(True)` SHALL give the keys one at a time, not echoed, with Ctrl+C
and Ctrl+Z as keys and output line ends unchanged; `RawMode(False)` SHALL
give back the terminal's mode. When standard input is not a terminal
RawMode does nothing and ReadKey decodes the bytes of the input.

#### Scenario: A pipe
- **WHEN** a program in raw mode reads keys from a file
- **THEN** it reads them as a terminal would send them (tests/runtime/terminal-keys.inox)

### Requirement: End of input
`ReadKey` SHALL stop the program with "Inox runtime error: end of input"
(CANON-19) when the input has ended.

#### Scenario: Past the end
- **WHEN** a program reads two keys from an input of one
- **THEN** it prints the first and stops with "end of input" (tests/runtime/terminal-end-of-input.inox)

### Requirement: The screen
`ScreenSize` SHALL be the window's rows and columns (24 x 80 when output is
not a terminal). `MoveTo(Row, Col)` moves the cursor, counted from 1 (below
1 is "value out of range"); `ClearScreen` clears and homes the cursor;
`ClearLine` clears the cursor's line; `SetColor`/`ResetColor` and
`ShowCursor` write ANSI sequences in order with `Put`; `Flush` shows what
waits.

#### Scenario: Output
- **WHEN** a program clears, moves, colors, hides the cursor and flushes
- **THEN** the sequences come out in order with its text (tests/runtime/terminal-output.inox)

### Requirement: The terminal is given back
The terminal SHALL be given back as found however the program ends (the
end of Main, a trap, out of memory, an exception nobody handles, a signal):
raw mode off, colors reset and cursor shown when they were changed, and on
Windows the console modes and code pages restored.

#### Scenario: A trap
- **WHEN** a program hides the cursor, sets a color and traps
- **THEN** the reset and the shown cursor are written before the runtime error (tests/runtime/terminal-restore-on-trap.inox), and colors left set at the end of Main are reset (tests/runtime/terminal-output.inox)
