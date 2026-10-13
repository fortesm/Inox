## Purpose
Text files in the `Std.Files` (the `File` type) and `Std.IOUtils`
(functions) modules: opening, reading, writing and closing
files, file system functions, and their errors as Inox exceptions.

## ADDED Requirements

### Requirement: Write and read text lines
`Create(Path)` SHALL open a new or truncated file for writing; `WriteLn(F, ...)`
SHALL write its arguments and one newline; `OpenRead(Path)` SHALL open a file
for reading; `ReadLn(F)` SHALL return the next line without its line end and
`Eof(F)` SHALL be true after the last line.

#### Scenario: Round trip
- **WHEN** a program creates a file, writes two lines with numbers and text, closes it, reopens it and reads until Eof
- **THEN** it prints the two lines as written (test to add: tests/runtime/files-text.inox)

### Requirement: Files close on every exit
A `File` variable SHALL be closed when its routine exits by any path,
including an exception; `Close(F)` SHALL close it earlier and closing twice
SHALL do nothing.

#### Scenario: Exception after writing
- **WHEN** a routine writes a line and then raises an exception caught by its caller, and the caller reads the file
- **THEN** the line is in the file (test to add: tests/runtime/files-close-on-exception.inox)

### Requirement: I/O errors are exceptions
Opening a missing file SHALL raise `FileNotFound`; other failures SHALL raise
`PermissionDenied`, `AlreadyExists`, `DiskFull` or `IOError`, handled by
`try`/`except On`/`Else`/`ensure` and `Retry` like any exception.

#### Scenario: Missing file
- **WHEN** a program opens a missing file inside `try ... except On FileNotFound ... ensure ... ;`
- **THEN** the handler runs, then the ensure (test to add: tests/runtime/files-not-found.inox)

### Requirement: A File is not copyable
A `File` SHALL NOT be assigned, copied or passed by value; it SHALL be passed
as `ref mut`.

#### Scenario: Copying a File
- **WHEN** a program writes `G := F` with F a File
- **THEN** the compiler rejects it (test to add: tests/diagnostics/file-copy.inox)

### Requirement: File system functions
`Exists(Path)`, `DeleteFile(Path)` and `RenameFile(From, To)` SHALL test,
delete and rename files, raising the I/O exceptions on failure.

#### Scenario: Create, rename, delete
- **WHEN** a program creates a file, renames it and deletes it, testing Exists after each step
- **THEN** Exists reports true, then true for the new name, then false (test to add: tests/runtime/files-system.inox)
