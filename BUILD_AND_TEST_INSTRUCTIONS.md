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

Expected current regression result:

```text
Summary: 154 passed, 0 failed, 154 total
```

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

Expected current regression result:

```text
Summary: 154 passed, 0 failed, 154 total
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

## Git policy

Do not commit generated directories or packages:

```text
build/
build-*/
dist/
```

Upload release ZIP files as GitHub Release assets instead.
