## Context

Depends on `string-runtime`: paths and lines are Strings, and a File slot is
an owned resource released by the same exit and exception cleanup.

## Decisions

- **Runtime.** `runtime/FileRuntime.cpp`, portable C++ over `std::FILE`:
  `__inox_file_open(ptr path, i32 mode) -> ptr`, `__inox_file_close(ptr)`,
  `__inox_file_readln(ptr) -> ptr` (String, +1), `__inox_file_eof(ptr) -> i1`,
  write helpers per argument kind, `__inox_file_exists`, `__inox_file_delete`,
  `__inox_file_rename`. Failures map `errno` (Linux) and the Windows error
  codes to the Inox exception type ids and raise through `__inox_raise`.
  Paths are UTF-8; on Windows they are converted to UTF-16 for `_wfopen`.
- **File value.** A `ptr` to a runtime handle; null after `Close`. The slot is
  owned like a String slot: closed at every routine exit and in the cleanup
  pad. Non-copyable: the analyzer rejects any copy (assignment, by-value
  argument, field, element, State, return).
- **Std.Files.** A source module that documents the API; the functions are
  compiler intrinsics (like `Put`/`PutLn` in `Std.IO`).

## Risks / Trade-offs

- First non-copyable type: every copy site in the analyzer must be checked;
  tests cover each.

## Design approval

Awaiting design approval (Risk: high, AGENTS.md); it will be sent in chat
before implementation.
