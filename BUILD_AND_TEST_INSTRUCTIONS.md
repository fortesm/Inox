# Build, test, and release instructions

This document summarizes the current commands used to build the Inox compiler, run the regression suite, and generate prebuilt release packages.

`docs/INOX_CANONICAL.md` remains the authoritative source of truth. This file is operational guidance only.

## Windows development build

Use PowerShell 7, CMake, Ninja, LLVM/Clang, and Visual Studio Build Tools with C++.

Recommended preset flow:

```powershell
cmake --preset windows-clang-msvc
cmake --build --preset windows-clang-msvc-debug
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
```

Direct equivalent:

```powershell
cmake -S . -B build\windows-clang-msvc -G "Ninja Multi-Config" -DCMAKE_CXX_COMPILER=clang++
cmake --build build\windows-clang-msvc --config Debug
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1 -InoxExe .\build\windows-clang-msvc\Debug\inox.exe
```

The cross-platform regression script now contains the same 305 checks as the Linux suite. Full native exception execution is currently validated on Linux; exception-enabled Windows binaries still require the documented Windows EH funclet lowering before a complete 305/305 Windows claim can be made.

Debug compiler path:

```text
build\windows-clang-msvc\Debug\inox.exe
```

Release compiler path:

```text
build\windows-clang-msvc\Release\inox.exe
```

## Windows release build

```powershell
cmake --build --preset windows-clang-msvc-release
```

Optional dependency inspection:

```powershell
llvm-objdump -p .\build\windows-clang-msvc\Release\inox.exe | Select-String "DLL Name" -NoEmphasis
```

A normal Release build should depend on Release MSVC/UCRT runtime libraries, not Debug runtime DLLs.

## Linux development build

Recommended preset flow:

```bash
cmake --preset linux-clang-debug
cmake --build --preset linux-clang-debug
bash scripts/run-tests.sh
```

Direct equivalent:

```bash
cmake -S . -B build/linux-clang-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
cmake --build build/linux-clang-debug
bash scripts/run-tests.sh build/linux-clang-debug/inox
```

Expected current Linux regression result:

```text
Summary: 305 passed, 0 failed, 305 total
```

## Linux release build

```bash
cmake --preset linux-clang-release
cmake --build --preset linux-clang-release
```

Release compiler path:

```text
build/linux-clang-release/inox
```

## CMake structure

The build is split into reusable CMake modules:

```text
cmake/InoxPlatform.cmake
cmake/InoxCompilerOptions.cmake
cmake/InoxWarnings.cmake
cmake/InoxSanitizers.cmake
cmake/InoxInstall.cmake
cmake/toolchains/
```

Validated presets currently target Windows Clang/MSVC and Linux Clang. Other toolchain files are stubs for future real validation.

## Testing Get/GetLn manually

Linux:

```bash
printf '42\n' | build/linux-clang-debug/inox --run tests/integration/input/get-integer.inox
printf '40 2 ignored\n' | build/linux-clang-debug/inox --run tests/integration/input/getln-two-integers.inox
printf '\n' | build/linux-clang-debug/inox --run tests/integration/input/getln-pause.inox
```

Windows:

```powershell
"42" | .\build\windows-clang-msvc\Debug\inox.exe --run .\tests\integration\input\get-integer.inox
"40 2 ignored" | .\build\windows-clang-msvc\Debug\inox.exe --run .\tests\integration\input\getln-two-integers.inox
"" | .\build\windows-clang-msvc\Debug\inox.exe --run .\tests\integration\input\getln-pause.inox
```

Expected outputs:

```text
A=42
S=42
before
after
```

## Extra verification tools (any OS)

A green regression suite alone never proves the absence of regressions
(CANON E19, verification principle). These tools answer different questions;
run them after any compiler change and say in the report which ran.

```
python tools/backend_gaps.py <path-to-inox>              # sema-accepts / backend-rejects inventory
python tools/backend_gaps.py <path-to-inox> --markdown   # table for docs/BACKEND_GAPS.md
python tools/mutation_fuzz.py <path-to-inox> --count 4000 --seed 1   # best on an ASan+UBSan build
python tools/calc_differential_test.py --inox <path-to-inox> --count 300
```

Sanitizer build (Linux/macOS with Clang):

```
cmake -S . -B build/asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ -DINOX_ENABLE_ASAN=ON -DINOX_ENABLE_UBSAN=ON
cmake --build build/asan
bash scripts/run-tests.sh build/asan/inox
```

Runtime faults (CANON-19): a program that overflows, divides by zero, etc.
prints `Inox runtime error: <category>` on stderr and exits with status 70. The
`tests/runtime/*.trap` files hold the expected diagnostic.

`backend_gaps.py` exits with status 1 if any probe is a BUG (a crash, a timeout
or a codegen failure not reported as "not yet implemented in the LLVM backend").

The nesting limits in `src/compiler/parser/Parser.h` were measured with a 1 MiB
stack (the Windows main-thread default). On Linux, `ulimit -s 1024` before
running the compiler reproduces that environment; re-measure before raising a
limit (see CANON-19 "Compiler implementation limits").

## Git policy

Do not commit generated directories or packages:

```text
build/
build-*/
dist/
```

Upload release ZIP files as GitHub Release assets instead.
