## Phase 1: BinaryFile

```
Use Std.Files

Main :
    Header Array[1..4] Byte
    Header[1] := $49   == 'I'
    Header[2] := $4E   == 'N'
    Header[3] := $58   == 'X'
    Header[4] := 1
    try
        Out := CreateBinary("dados.bin")
        WriteBytes(Out, Header[..])          == a Slice<Byte>
        Write(Out, Int32(-2), 2.5, true)     == 4 + 8 + 1 bytes, little-endian
        Close(Out)

        Input := OpenBinary("dados.bin")
        PutLn(Size(Input))                   == 17
        Magic Array[1..4] Byte
        Count := ReadBytes(Input, Magic)     == fills the array; bytes read (4)
        N Int32 := 0
        X Float := 0.0
        Flag Bool := false
        Read(Input, N, X, Flag)              == each sized by its type
        Seek(Input, 0)                       == byte offset from the start
        PutLn(Count, " ", Magic[1], " ", N, " ", X, " ", Flag, " ", Position(Input), " ", ReadByte(Input))
    except
        On FileNotFound
            PutLn("dados.bin não existe")
        ;
        On IOError
            PutLn("erro de E/S")
        ;
    ;
;
```

Operations proposed (compiler intrinsics, like the text ones):

| Operation | Meaning |
|---|---|
| `OpenBinary(Path) BinaryFile` | an existing file, read and write, at offset 0 |
| `CreateBinary(Path) BinaryFile` | a new or truncated file, read and write |
| `AppendBinary(Path) BinaryFile` | a new or existing file, writing at its end |
| `ReadByte(F) Byte`, `WriteByte(F, B)` | one byte |
| `ReadBytes(F, ref mut A) Integer` | fills a Byte array; returns how many were read (0 at the end) |
| `WriteBytes(F, S Slice<Byte>)` | writes the slice |
| `Write(F, Values...)`, `Read(F, ref mut Targets...)` | numeric and Bool values, sized by their type |
| `Position(F) Integer`, `Seek(F, Offset)`, `Size(F) Integer` | byte offsets, from 0 |
| `Eof(F) Bool`, `Close(F)` | as for text files |

## Decisions for the maintainer

1. **A separate `BinaryFile` type** (recommended): `ReadLn` on a binary
   file or `Seek` on a text file is a compile error, not an IOError at run
   time. Alternative: one `File` type with an open mode.
2. **Typed values in a fixed byte order, little-endian** (recommended), so
   a file written on one system reads on every other. Alternative: the
   machine's order (Delphi), which is the same on x64 and ARM64 today.
3. **`Write`/`Read` sized by the type** (recommended, less to write):
   `Write(F, Int32(X))` writes 4 bytes. An integer literal is an Integer,
   so `Write(F, 7)` writes 8 bytes. Alternative: one name per type
   (`WriteInt32`, `ReadFloat64`, …).
4. **Strings in binary files**: not written by `Write` (recommended; a
   format needs to say how a length is stored). `WriteText(F, S)` writes the
   UTF-8 bytes only, and reading text back waits for `Vector<Byte>` and
   `Std.Strings`. Alternative: a length prefix (Int32 or Int64) chosen now.
5. **Reading past the end**: `ReadByte` and `Read` raise IOError (recommended;
   `Eof` is checked first, as with text), `ReadBytes` returns a short count.
   Alternative: a new exception `EndOfFile` under IOError in the CANON-15
   taxonomy.
6. **Offsets 0-based in bytes** (recommended; a position in a file is not a
   string position, which counts code points from 1).

## Phase 2: streams (after `generics`)

```
CopyAll<S, D>(ref mut Source S, ref mut Target D) Integer :
    Buffer Array[1..4096] Byte
    Total Integer := 0
    repeat
        Count := ReadBytes(Source, Buffer)
        if Count > 0
            WriteBytes(Target, Buffer[1..Count])
        ;
        Total += Count
        until Count = 0
    ;
    Return Total
;
```

Any type with `ReadBytes`, `WriteBytes`, `Position`, `Seek` and `Size` works
as a stream: `BinaryFile`, a `MemoryStream` over `Vector<Byte>` and the
console streams. Each use is checked with its real types and compiled once
per type, with no virtual calls; no nominal protocol is needed (FUTURE).
The decisions for phase 2 come with that change.

## Order of work after approval

`BinaryFile` with bytes, positions and `Close` → typed `Read`/`Write` →
`CopyFile`/`FileSize` in Std.IOUtils → tests (round trip, every error,
close on exception, leak check) and the example extended. Streams after
`generics`.
