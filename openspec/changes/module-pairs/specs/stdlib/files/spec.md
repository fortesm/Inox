## MODIFIED Requirements

### Requirement: The file names exist only with Use
The type File and the basic operations (OpenRead, Create, OpenAppend,
ReadLn, Eof, Write, WriteLn, Close) SHALL be visible only in a program that
uses Std.Files or Std.IOUtils, and the conveniences (Exists, DeleteFile,
RenameFile) only in a program that uses Std.IOUtils; elsewhere their names
SHALL be free for the program's own routines, and a call to a file function
SHALL report the module that declares it.

#### Scenario: Without Use
- **WHEN** a program without `Use` calls OpenRead, or declares a File parameter
- **THEN** the compiler asks for `Use Std.Files` (tests/diagnostics/file-without-use.inox, tests/diagnostics/file-type-without-use.inox)

#### Scenario: Basics with Std.Files alone
- **WHEN** a program with only `Use Std.Files` creates, writes, appends to and reads a file, and declares its own routine named Exists
- **THEN** the file operations work and its own Exists is called (tests/runtime/files-basic-module.inox)

#### Scenario: Conveniences need Std.IOUtils
- **WHEN** a program with only `Use Std.Files` calls Exists
- **THEN** the compiler asks for `Use Std.IOUtils` (tests/diagnostics/file-utility-needs-ioutils.inox)

#### Scenario: Names free without Use
- **WHEN** a program without `Use` declares its own Create and Close
- **THEN** it calls them (tests/runtime/file-names-free.inox)

#### Scenario: Names taken with Use
- **WHEN** a program with `Use Std.IOUtils` declares its own Create
- **THEN** the compiler rejects the duplicate name (tests/diagnostics/file-name-taken.inox)
