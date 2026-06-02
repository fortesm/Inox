# Inox

Inox is a compiled, strongly typed, post-object-oriented systems language for high-integrity software. It is built around structs as data, associated methods as behavior, composition instead of inheritance, explicit mutability, strong typing, bounds checking, and an LLVM-oriented backend.

Inox is currently a 0.1 pre-alpha compiler and language laboratory. The compiler is useful for lexer, parser, semantic analysis, documentation, regression tests, LLVM IR emission, and a restricted Clang-backed native build/run path.

## Usage profiles

| Profile | Needs Clang/LLVM? | Needs a C++ compiler? | Notes |
|---|---:|---:|---|
| Inox compiler developer | Yes | Yes | Builds `inox.exe` from the C++ source code. |
| User who only wants to run a prebuilt `inox.exe` | No | No | Needs only the Release executable and the normal system/MSVC runtimes. |
| User who wants to use `inox.exe --build` or `inox.exe --run` to generate native `.exe` files | Yes, for now | Not to develop the compiler, but a native toolchain is required | If the driver calls `clang`, `lld`, `link.exe`, or other external tools, those tools must be installed. |

## Downloading a prebuilt Windows test release

Users who only want to test the Inox compiler and the language do not need to build the compiler from source.

Download the latest Windows x64 test package here:

https://github.com/fortesm/Inox/releases/latest/download/inox-windows-x64.zip

A prebuilt Windows test package has this layout:
```text
inox-windows-x64/
    bin/
        inox.exe
    stdlib/
        Std.Core.inox
        Std.Debug.inox
        Std.IO.inox
        Std.Math.inox
    examples/
    output/
    licenses/
    README.md
    set-inox-env.ps1
```

To test the language with a prebuilt package:

```powershell
Expand-Archive .\inox-windows-x64.zip -DestinationPath C:\Tools
cd C:\Tools\inox-windows-x64
.\bin\inox.exe .\examples\hello.inox
.\bin\inox.exe --emit-llvm .\examples\llvm-put-output-basic.inox
```

For normal use, either run the compiler from the extracted package root or set `INOX_STDLIB` explicitly:

```powershell
$env:INOX_STDLIB = "C:\Tools\inox-windows-x64\stdlib"
$env:Path += ";C:\Tools\inox-windows-x64\bin"
inox .\examples\hello.inox
```

The prebuilt `inox.exe` Release binary does not require Clang, LLVM, or a C++ compiler merely to parse, type-check, or emit LLVM IR. On Windows it may require the normal Microsoft Visual C++ Redistributable runtime.

## Native build/run of Inox programs

The temporary 0.1 driver can delegate native executable generation to Clang:

```powershell
inox --build examples\llvm-put-output-basic.inox
inox --run examples\llvm-put-output-basic.inox
```

This mode currently requires `clang` in `PATH`. Generated LLVM IR and executables are written to the default artifact directory:

```text
build/inox-artifacts/
```

Override that output directory with:

```powershell
$env:INOX_OUTPUT_DIR = "C:\Temp\inox-out"
```

## Building the compiler from source

Developers who want to modify or build the Inox compiler need a native C++ toolchain.

Windows development prerequisites:

- Git for Windows
- PowerShell 7
- CMake
- Ninja
- LLVM/Clang
- Visual Studio Build Tools with C++
- Windows SDK

Recommended Windows build:

```powershell
cmake -S . -B build -G "Ninja Multi-Config" -DCMAKE_CXX_COMPILER=clang++
cmake --build build --config Debug
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
```

Debug compiler path:

```text
build/Debug/inox.exe
```

Release compiler path:

```text
build/Release/inox.exe
```

Linux / Unix-like development build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
bash scripts/run-tests.sh
```

The `build/` directory is generated and must not be versioned.

## Standard library and runtime status

The initial standard library lives under `stdlib/` and is intentionally minimal. The `Std.*.inox` files are early standard-library modules and documentation anchors, not a complete standalone runtime library.

Inox does not yet provide a dedicated runtime library such as:

```text
libinoxrt.a
inoxrt.lib
inox_runtime.dll
```

At this stage:

- the Inox compiler itself is a native C++ program;
- `Put` and `PutLn` are compiler/runtime-lowered facilities used by the current backend;
- generated Inox programs may still rely on the host platform toolchain and system runtimes;
- the standard library and runtime boundary will become explicit as the language matures.

## Documentation

Start here:

- `docs/canonical/language-reference.md` — consolidated tutorial/reference.
- `docs/development/toolchain.md` — requirements for compiler developers and language testers.
- `docs/release/prebuilt-usage.md` — packaging and usage guide for prebuilt releases.
- `docs/site/index.html` — browsable HTML manual.
- `docs/decisions/ADR-0006-inox-0.1-constitution.md` — frozen 0.1 decisions.
- `AGENTS.md` — operational instructions for AI agents.
- `docs/open-questions/OPEN_QUESTIONS.md` — deferred 0.2+ architecture topics.

## Current compiler capabilities

The compiler currently includes lexer, parser, semantic analyzer, layered tests, typed dumps, a textual LLVM backend for a restricted executable subset, native build/run through Clang, and minimal local multi-file `Module`/`Use` support. The backend is intentionally incremental and test-driven.

## Design stance

Inox rejects unsafe defaults: universal null, implicit narrowing, integer wraparound guarantees, unchecked bounds, implicit aliasing, classes, inheritance, and Java-style interfaces. Future work includes modules, arrays, vectors, sets, contracts/protocols/behaviors, arenas, borrowing, unsafe boundaries, and structured parallelism.

## License

Inox is free software licensed under the Mozilla Public License Version 2.0 (MPL-2.0). See `LICENSE` for the full license text.

Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.

The project does not use the MPL 2.0 "Incompatible With Secondary Licenses" notice. See also `NOTICE.md`, `AUTHORS.md`, `CONTRIBUTING.md`, and `TRADEMARK.md`.
