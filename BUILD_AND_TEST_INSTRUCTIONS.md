# Build, test, and release instructions

This document summarizes the commands used to build the Inox compiler, run the regression suite, and generate prebuilt release packages.

## Windows development build

Use PowerShell 7, CMake, Ninja, LLVM/Clang, and Visual Studio Build Tools with C++.

From the repository root:

```powershell
cmake -S . -B build -G "Ninja Multi-Config" -DCMAKE_CXX_COMPILER=clang++
cmake --build build --config Debug
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
```

Expected test result:

```text
Summary: 142 passed, 0 failed, 142 total
```

Debug compiler path:

```text
build\Debug\inox.exe
```

Release compiler path:

```text
build\Release\inox.exe
```

## Windows release build

```powershell
cmake --build build --config Release
```

Optional dependency inspection:

```powershell
llvm-objdump -p .\build\Release\inox.exe | Select-String "DLL Name" -NoEmphasis
```

A normal Release build should depend on Release MSVC/UCRT runtime libraries, not Debug runtime DLLs.

## Windows release package

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\package-release.ps1
```

Generated package:

```text
dist\inox-windows-x64.zip
```

The package contains:

```text
inox-windows-x64/
    README.md
    set-inox-env.ps1
    bin/
        inox.exe
    stdlib/
    examples/
    output/
    manual/
        index.html
    licenses/
```

## Linux development build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
cmake --build build
bash scripts/run-tests.sh
```

Expected test result:

```text
Summary: 142 passed, 0 failed, 142 total
```

## Linux release build

```bash
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build-linux
```

Release compiler path:

```text
build-linux/inox
```

## Linux release package

```bash
bash scripts/package-release.sh Release inox-linux-x64 build-linux/inox
```

Generated package:

```text
dist/inox-linux-x64.zip
```

The package contains:

```text
inox-linux-x64/
    README.md
    set-inox-env.sh
    bin/
        inox
    stdlib/
    examples/
    output/
    manual/
        index.html
    licenses/
```

## Testing a Windows release package

```powershell
Expand-Archive .\dist\inox-windows-x64.zip -DestinationPath .\dist\check -Force
cd .\dist\check\inox-windows-x64
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\set-inox-env.ps1
inox .\examples\hello.inox
inox --emit-llvm .\examples\llvm-put-output-basic.inox
```

If Clang is available:

```powershell
inox --build .\examples\llvm-put-output-basic.inox
inox --run .\examples\llvm-put-output-basic.inox
```

## Testing a Linux release package

```bash
rm -rf dist/check-linux
mkdir -p dist/check-linux
unzip -q dist/inox-linux-x64.zip -d dist/check-linux
cd dist/check-linux/inox-linux-x64
source ./set-inox-env.sh
inox ./examples/hello.inox
inox --emit-llvm ./examples/llvm-put-output-basic.inox
```

If Clang is available:

```bash
inox --build ./examples/llvm-put-output-basic.inox
inox --run ./examples/llvm-put-output-basic.inox
```

## Git policy

Do not commit generated directories or packages:

```text
build/
build-linux/
dist/
```

Upload release ZIP files as GitHub Release assets instead:

```text
inox-windows-x64.zip
inox-linux-x64.zip
```

Public download links after publishing a GitHub Release:

```text
https://github.com/fortesm/Inox/releases/latest/download/inox-windows-x64.zip
https://github.com/fortesm/Inox/releases/latest/download/inox-linux-x64.zip
```
