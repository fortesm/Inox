## Why

The canon recorded a goal for the driver — subcommands `inox run`, `inox
build`, `inox check` instead of `--` switches — and left it open (open
question 1 of `revisions.md`, r3.61). Asked, the maintainer agreed with the
recommendation (Marcelo Fortes, 2026-10-13: 1 and 2 approved): the
subcommands become the user's contract and the switches stay as aliases.
The same answer settles question 2: an exception binding is exempt from the
unread-local rule only until exceptions have readable members.

Risk: normal (driver argument parsing; no language or lowering change).

## What Changes

- `inox file.inox` builds and runs (it only checked before); `inox run`,
  `inox build`, `inox check`, `inox emit-llvm`, `inox help`, `inox version`.
  `--run`, `--build`, `--emit-llvm`, `--help`, `--version` are aliases;
  `--parse-only`, `--dump-tokens`, `--dump-types` stay developer switches.
  A command without a file is a usage error.
- Both runners check frontend tests with `inox check` (they used
  `inox file`, which now runs the program) and test the new commands.
- Constitution CANON-23 (Driver commands) and CANON-5 (Unread locals);
  manual, BUILD_AND_TEST_INSTRUCTIONS, STATUS.

Out of scope: options such as an output path or optimization level.
