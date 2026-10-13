## Why

Marcelo Fortes, 2026-10-13: "teste tudo que pode ser feito com arquivos
texto, binário, stream". Text files exist (CANON-17, `examples/file_io.inox`
tests all of them); CANON-17 leaves "binary I/O, directories, positions and
other encodings" to later changes. This is the change for binary files and
positions, and the direction for streams.

Status: **Awaiting approval** of the decisions in design.md. Risk: high (new
type, new runtime functions, a byte-exact file format). No code before
approval.

## What Changes (proposed)

- Phase 1, after approval: a `BinaryFile` type in Std.Files with the same
  ownership rules as `File` (never copied, `ref mut` parameters, closed on
  every exit) and its own operations: open/create/append, bytes, typed
  values in little-endian, `Position`, `Seek`, `Size`, `Eof`, `Close`.
  Every failure is a CANON-15 IOError, as for text files.
- Std.IOUtils gains `CopyFile` and `FileSize` (conveniences built on it).
- Phase 2, after `generics` and `composition`: streams, a `TStream`
  protocol satisfied by `BinaryFile`, a memory stream (needs `Vector<Byte>`)
  and the console streams, so one routine reads or writes any of them.
- Text files get no `Seek` (positions in a UTF-8 text are bytes, not
  characters); they stay as they are.
