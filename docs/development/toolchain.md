# Toolchain requirements

Inox has different requirements depending on how it is used.

## Usage profiles

| Profile | Needs Clang/LLVM? | Needs a C++ compiler? | Notes |
|---|---:|---:|---|
| Inox compiler developer | Yes | Yes | Builds `inox.exe` from the C++ source code. |
| User who only wants to run a prebuilt `inox.exe` | No | No | Needs only the Release executable and the normal system/MSVC runtimes. |
| User who wants to use `inox.exe --build` or `inox.exe --run` to generate native `.exe` files | Yes, for now | Not to develop the compiler, but a native toolchain is required | If the driver calls `clang`, `lld`, `link.exe`, or other external tools, those tools must be installed. |

## Compiler developers

Developers who want to modify or build the compiler need a full native C++ toolchain.

Recommended Windows setup:

- Git for Windows
- PowerShell 7
- CMake
- Ninja
- LLVM/Clang
- Visual Studio Build Tools with C++
- Windows SDK

The Windows compiler build uses Clang with the MSVC ABI target:

```text
x86_64-pc-windows-msvc
```

Clang is the C++ compiler. Visual Studio Build Tools provide the Windows platform pieces required by that target: SDK headers, import libraries, the MSVC STL, the MSVC/UCRT runtime, and linker support.

Recommended Windows commands:

```powershell
cmake -S . -B build -G "Ninja Multi-Config" -DCMAKE_CXX_COMPILER=clang++
cmake --build build --config Debug
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
```

Recommended Linux / Unix-like commands:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
bash scripts/run-tests.sh
```

## Users testing a prebuilt compiler

Users who only want to test a prebuilt Release `inox.exe` do not need LLVM, Clang, CMake, Ninja, or a C++ compiler merely to run the compiler.

They can run:

```powershell
.\bin\inox.exe .\examples\hello.inox
.\bin\inox.exe --parse-only .\examples\hello.inox
.\bin\inox.exe --emit-llvm .\examples\llvm-put-output-basic.inox
```

On Windows, a Release build may require the normal Microsoft Visual C++ Redistributable runtime. A Debug build must not be distributed because it depends on non-redistributable Debug runtime DLLs such as `MSVCP140D.dll`, `VCRUNTIME140D.dll`, or `ucrtbased.dll`.

## Users building native programs with Inox

The current `--build` and `--run` driver is temporary and Clang-backed. It emits LLVM IR and invokes external tooling to produce a native executable.

Therefore, users who want this mode need `clang` available in `PATH` for now:

```powershell
inox --build examples\llvm-put-output-basic.inox
inox --run examples\llvm-put-output-basic.inox
```

This does not mean they need to build the Inox compiler source code. It means the current Inox driver still delegates final native code generation and linking to an external platform toolchain.

Future SDK work may package the required backend and linker tools with Inox or replace this path with a more complete native backend/runtime strategy.
