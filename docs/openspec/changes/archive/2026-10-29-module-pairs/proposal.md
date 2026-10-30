## Why

The rule of r3.58 put only "definitions and general data, like a C header"
in `Std.X` and every function in `Std.XUtils`. While planning
`Std.Strings`, the maintainer asked for usable functions in `Std.Strings`
(Marcelo Fortes, 2026-10-13: "como se trata de uma biblioteca para ajudar o
usuário deve ter funções utilizáveis pelo usuário"), and approved moving the
basic file functions too: "3) Arquivos muda isso também o básico fica em
Std.Files e as comodidades para Std.IOUtils".

Risk: normal (which module declares a name; no lowering, runtime or
ownership change).

## What Changes

- Constitution CANON-18 "Library principles": `Std.X` holds a subject's
  types, data and fundamental operations; `Std.XUtils` the conveniences
  built on them, using `Std.X` and giving everything of it.
- `Std.Files`: the type File and OpenRead, Create, OpenAppend, ReadLn, Eof,
  Write, WriteLn, Close. `Std.IOUtils`: Exists, DeleteFile, RenameFile, and
  (as before) everything of Std.Files.
- Diagnostics name the module that declares the function.
- Constitution CANON-17 "Files are visible only with Use"; spec
  `stdlib/files`; stdlib comments; manual; STATUS; CHANGELOG.

Out of scope: the String modules (change `std-strings`). A program that
writes `Use Std.IOUtils` behaves as before.
