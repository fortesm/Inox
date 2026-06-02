# Build, test, and package Inox locally

This document is for people who want to build the Inox compiler from source. Users who only want to test the language with a prebuilt compiler should read `docs/release/prebuilt-usage.md`.

## Windows 11 with LLVM/Clang

Prerequisites:

- Git for Windows.
- PowerShell 7 (`pwsh`).
- CMake.
- Ninja.
- LLVM/Clang (`clang --version` and `clang++ --version` must work).
- Visual Studio Build Tools with C++ and the Windows SDK.

Inox is normally built with Clang targeting the MSVC ABI. Clang compiles the C++ source, while Visual Studio Build Tools provide the Windows SDK, MSVC STL, runtime libraries, and linker support.

From PowerShell 7 or Developer PowerShell for VS 2022:

```powershell
cd C:\Projetos\Inox

Remove-Item .\build -Recurse -Force -ErrorAction SilentlyContinue

cmake -S . -B build -G "Ninja Multi-Config" -DCMAKE_CXX_COMPILER=clang++

cmake --build build --config Debug

pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
```

Debug compiler path:

```text
C:\Projetos\Inox\build\Debug\inox.exe
```

Release compiler path:

```text
C:\Projetos\Inox\build\Release\inox.exe
```

Build Release with:

```powershell
cmake --build build --config Release
```

Inspect Release DLL dependencies with:

```powershell
llvm-objdump -p .\build\Release\inox.exe | Select-String "DLL Name" -NoEmphasis
```

The Release binary should not depend on Debug runtime DLLs such as `MSVCP140D.dll`, `VCRUNTIME140D.dll`, or `ucrtbased.dll`.

## Linux / Unix-like systems

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
bash scripts/run-tests.sh
```

To validate specifically with Clang:

```bash
cmake -S . -B build-clang -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
cmake --build build-clang
bash scripts/run-tests.sh build-clang/inox
```

## Standard library discovery

The compiler resolves standard-library imports such as `Use Std.Math` through `stdlib/`.

Search order:

1. `INOX_STDLIB`, when set.
2. `stdlib/` next to the release package root, when the executable is under `bin/`.
3. `stdlib/` next to the executable.
4. `stdlib/` under the current working directory.
5. `stdlib/` in the source file directory or one of its parent directories.

Example:

```powershell
$env:INOX_STDLIB = "C:\Tools\inox-windows-x64\stdlib"
```

## Output directory for Inox-built programs

The temporary Clang-backed `--build` and `--run` driver writes artifacts to:

```text
build/inox-artifacts/
```

Override with:

```powershell
$env:INOX_OUTPUT_DIR = "C:\Temp\inox-out"
```

This directory is generated output and must not be versioned.

## Compile and run Inox programs

The temporary 0.1 driver uses Clang as an external native toolchain:

```powershell
.\build\Debug\inox.exe --build .\tests\integration\run-hello.inox
.\build\Debug\inox.exe --run .\tests\integration\run-hello.inox
```

For Release:

```powershell
.\build\Release\inox.exe --build .\examples\llvm-put-output-basic.inox
.\build\Release\inox.exe --run .\examples\llvm-put-output-basic.inox
```

`--build` and `--run` require `clang` in `PATH` for now. Parse, semantic-check, dump, and `--emit-llvm` modes do not require Clang when using a prebuilt `inox.exe`.

## Create a Windows x64 test release ZIP

After building Release on Windows:

```powershell
cmake --build build --config Release
pwsh -ExecutionPolicy Bypass -File .\scripts\package-release.ps1
```

This creates:

```text
dist/inox-windows-x64.zip
```

Publish that ZIP as a GitHub Release asset, for example:

```text
https://github.com/<OWNER>/Inox/releases/latest/download/inox-windows-x64.zip
```

## Expected test result

The current expected test result is:

```text
Summary: 142 passed, 0 failed, 142 total
```
