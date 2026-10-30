# stdlib/systemutils Specification

## Purpose
The program and its system in `Std.SystemUtils`: the command line, the
environment, the working directory, the platform, Halt and Sleep. Part of
the Inox law, detailing the constitution (CANON-18).

## Requirements

### Requirement: The command line
`ParamCount` SHALL be the number of arguments after the program's name and
`ParamStr(I)` the I-th (0 the program); outside 0..ParamCount it stops the
program with "value out of range". The arguments are UTF-8 (D4).

#### Scenario: Arguments
- **WHEN** a program is run without arguments, asks ParamStr past the last, and is run with two
- **THEN** ParamCount is 0 (tests/runtime/system-utils.inox), the second traps (tests/runtime/system-paramstr-range.inox), and the third prints both (tests/integration/system/args.inox)

### Requirement: Environment, directory and platform
`GetEnv(Name)` SHALL be a variable's value or `""`, `EnvExists(Name)`
whether it is set, `CurrentDir` the working directory and `PlatformName`
the system's name (`Linux`, `Windows`, `macOS`, `FreeBSD`, ...) (D4).

#### Scenario: The system
- **WHEN** a program reads PATH, a variable that is not set, the directory and the platform
- **THEN** each is as stated (tests/runtime/system-utils.inox)

### Requirement: Halt and Sleep
`Halt(Code)` SHALL end the program at once with exit status Code (0..255,
else "value out of range"), giving the terminal back and flushing output;
`Sleep(Ms)` waits, showing what was written first (D4).

#### Scenario: Ending early
- **WHEN** a program sleeps, halts with 0 and with 256
- **THEN** it stops with status 0 after its output (tests/runtime/system-utils.inox) and the last traps (tests/runtime/system-halt-range.inox)

### Requirement: Visible with Use
The names of Std.SystemUtils SHALL exist only in a program that uses it.

#### Scenario: Without the Use
- **WHEN** a program calls ParamCount or GetEnv(1)
- **THEN** the first is unknown without the Use (tests/diagnostics/system-utils-without-use.inox) and the second rejected (tests/diagnostics/getenv-argument.inox)
