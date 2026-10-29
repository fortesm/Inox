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
- **File value.** A `ptr` to a reference-counted runtime handle
  `{count, FILE*, mode}`; its stream is null after `Close`. The language
  forbids copies, so the count only follows the moves the compiler makes:
  the open call's result is held for its statement (+1), the store into the
  variable retains, the end of the statement releases. This lets every
  String ownership path apply unchanged (`typeOwnsReferences`): File locals
  are owned slots, released at every routine exit and in the cleanup pad,
  and a store releases (closes) the previous file. Releasing the last
  reference closes the stream. Non-copyable: the analyzer rejects any copy
  (an initializer or assignment that is not an open call, a by-value or
  `ref` argument, field, element, State, result, comparison).
- **Visibility.** The driver records the loaded modules on the program
  node; the analyzer declares the File type when Std.Files or Std.IOUtils
  is loaded and the functions when Std.IOUtils is. Without the module the
  names stay free, and a call or type gets a hint to add `Use Std.IOUtils`.
- **Calls.** `FunctionEmitter::emitRuntimeCall` uses `invoke` inside a
  protected region (try body or cleanup pad), `call` elsewhere; the
  functions that can raise are declared without `nounwind`.
- **Portability.** Binary mode always; UTF-16 paths on Windows (`_wfopen`,
  `_wremove`, `_wrename`, `_wstat64`); errno mapped to the exceptions
  (ENOENT/ENOTDIR → FileNotFound, EACCES/EPERM/EROFS → PermissionDenied,
  EEXIST → AlreadyExists, ENOSPC/EDQUOT → DiskFull, otherwise IOError).
  RenameFile checks the target first, so AlreadyExists is the same on every
  system; DeleteFile uses `unlink` on POSIX (never a directory).
- **Std.Files / Std.IOUtils (canon r3.58).** Source modules that document
  the API: `Std.Files` the `File` type, `Std.IOUtils` the functions, which
  are compiler intrinsics (like `Put`/`PutLn` in `Std.IO`).

## Risks / Trade-offs

- First non-copyable type: every copy site in the analyzer must be checked;
  tests cover each.

## Design approval

Approved by Marcelo Fortes on 2026-10-12: "100% aprovado!", answering the
design summary sent in chat with the module pair Std.Files (the File type)
/ Std.IOUtils (the functions).
