## Why

Inox programs cannot read or write files: `Std.IO` is console only, and the
exception types `IOError`, `FileNotFound`, `PermissionDenied`,
`AlreadyExists` and `DiskFull` exist with nothing that raises them. Marcelo
asked for files next (2026-10-12): the design discussion he had with ChatGPT
listed the operations and the safety goals, but no code was written.

Approval (Marcelo Fortes, 2026-10-12, answering decisions F1–F7 sent in
chat): all approved, with the note that I/O errors appear in
`try ... except On <error> ... ensure ... ;`, not only with `Retry`, and
that the basic file functions must exist.

Risk: high (a new runtime ABI, and the first non-copyable owned resource,
closed on every exit including exceptions). Design approval required before
code; implemented after `string-runtime`, which it depends on (paths and
lines are Strings; the close-on-exit path is the String cleanup path).

## What Changes

Decisions (Layer A once implemented; canon entry with the change):
- **F1. A file closes itself.** A `File` variable closes its file when the
  routine that holds it exits by any path (end, `Return`, `Exit`, an
  exception); `Close(F)` closes it earlier, and closing twice is harmless.
- **Module pair (canon r3.58).** `Std.Files` holds the definitions: the
  `File` type (the exception types stay in `Std.Errors`). `Std.IOUtils`
  holds every function below (F2, F4, F7) and uses `Std.Files`. To be
  confirmed with the design approval.
- **F2. Opening.** Free functions of `Std.IOUtils`:
  `OpenRead(Path)`, `Create(Path)` (new or truncated, for writing),
  `OpenAppend(Path)`. No modes enum (Enum is not implemented yet) and no
  static method form (not defined in Inox).
- **F3. Errors are exceptions** of the existing taxonomy: `FileNotFound`,
  `PermissionDenied`, `AlreadyExists`, `DiskFull`, otherwise `IOError`. They
  are ordinary Inox exceptions: `try`, `except On FileNotFound ...`, `Else`,
  `ensure` and `Retry(N)` all apply. `Result<T, E>` is a later option.
- **F4. Text only in this change:** `ReadLn(F) String`, `WriteLn(F, ...)` and
  `Write(F, ...)` with any number of arguments like `PutLn`, `Eof(F) Bool`.
  Binary I/O (Byte buffers) is a later change.
- **F5. UTF-8 text.** Reading accepts `\n` and `\r\n` line ends; writing
  always writes `\n`; invalid UTF-8 read raises `IOError`.
- **F6. A `File` is not copyable.** It cannot be assigned, copied into a
  field, element or State, or passed by value; it is passed as `ref mut`.
  This is the first non-copyable type of Inox.
- **F7. File system functions:** `Exists(Path) Bool`, `DeleteFile(Path)`,
  `RenameFile(From, To)`, raising the same exceptions.

Out of scope: binary I/O, directories, seeking, file positions and sizes,
encodings other than UTF-8, asynchronous I/O.

## Capabilities

### New Capabilities
- `stdlib/files`: the `Std.Files` (the `File` type) and `Std.IOUtils`
  (functions) modules.

### Modified Capabilities
(none)

## Impact

New `stdlib/Std.Files.inox` and `stdlib/Std.IOUtils.inox` (declarations
and documentation), runtime
`runtime/FileRuntime.cpp` (portable C++ over the C runtime, Windows and
Linux), the emitter (the File slot is an owned resource closed by the String
cleanup path), the analyzer (non-copyable rule), canon CANON-17/18, manual,
CHANGELOG.
