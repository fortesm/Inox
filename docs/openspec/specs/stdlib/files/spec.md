# stdlib/files Specification

## Purpose
Text files in the `Std.Files` (the `File` type) and `Std.IOUtils`
(functions) modules: opening, reading, writing and closing
files, file system functions, and their errors as Inox exceptions. Part of
the Inox law, detailing the constitution (CANON-17).

## Requirements

### Requirement: Write and read text lines
`Create(Path)` SHALL open a new or truncated file for writing; `WriteLn(F, ...)`
SHALL write its arguments and one newline; `OpenRead(Path)` SHALL open a file
for reading; `ReadLn(F)` SHALL return the next line without its line end and
`Eof(F)` SHALL be true after the last line.

#### Scenario: Round trip
- **WHEN** a program creates a file, writes two lines with numbers and text, closes it, reopens it and reads until Eof
- **THEN** it prints the two lines as written (tests/runtime/files-text.inox)

### Requirement: Files close on every exit
A `File` variable SHALL be closed when its routine exits by any path,
including an exception; `Close(F)` SHALL close it earlier and closing twice
SHALL do nothing.

#### Scenario: Exception after writing
- **WHEN** a routine writes a line and then raises an exception caught by its caller, and the caller reads the file
- **THEN** the line is in the file (tests/runtime/files-close-on-exception.inox)

### Requirement: I/O errors are exceptions
Opening a missing file SHALL raise `FileNotFound`; other failures SHALL raise
`PermissionDenied`, `AlreadyExists`, `DiskFull` or `IOError`, handled by
`try`/`except On`/`Else`/`ensure` and `Retry` like any exception.

#### Scenario: Missing file
- **WHEN** a program opens a missing file inside `try ... except On FileNotFound ... ensure ... ;`
- **THEN** the handler runs, then the ensure (tests/runtime/files-not-found.inox)

### Requirement: A File is not copyable
A `File` SHALL NOT be assigned, copied or passed by value; it SHALL be passed
as `ref mut`.

#### Scenario: Copying a File
- **WHEN** a program writes `G := F` with F a File
- **THEN** the compiler rejects it (tests/diagnostics/file-copy.inox, file-assign-copy, file-field, file-parameter-by-value, file-return, file-state, file-array-element, file-compare)

### Requirement: File system functions
`Exists(Path)`, `DeleteFile(Path)` and `RenameFile(From, To)` SHALL test,
delete and rename files, raising the I/O exceptions on failure.

#### Scenario: Create, rename, delete
- **WHEN** a program creates a file, renames it and deletes it, testing Exists after each step
- **THEN** Exists reports true, then true for the new name, then false (tests/runtime/files-system.inox)

### Requirement: The file names exist only with Use
The type File SHALL be visible only in a program that uses Std.Files or
Std.IOUtils, and the file functions only in a program that uses
Std.IOUtils; elsewhere their names SHALL be free for the program's own
routines, and a call to a file function SHALL report that the module is
needed.

#### Scenario: Without Use
- **WHEN** a program without `Use Std.IOUtils` calls OpenRead, or declares a File parameter
- **THEN** the compiler asks for `Use Std.IOUtils` (tests/diagnostics/file-without-use.inox, file-type-without-use.inox)

#### Scenario: Names free without Use
- **WHEN** a program without `Use Std.IOUtils` declares its own Create and Close
- **THEN** it calls them (tests/runtime/file-names-free.inox)

#### Scenario: Names taken with Use
- **WHEN** a program with `Use Std.IOUtils` declares its own Create
- **THEN** the compiler rejects the duplicate name (tests/diagnostics/file-name-taken.inox)

### Requirement: Files are closed when they are released
A File's file SHALL be closed when no variable holds it any more, and the
memory of the handle freed, also through exceptions.

#### Scenario: Leak check
- **WHEN** the file programs run under AddressSanitizer with leak detection
- **THEN** no handle leaks and none is freed twice (tools/string_leak_check.py over tests/runtime/files-*.inox)
