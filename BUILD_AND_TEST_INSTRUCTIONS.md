# Build and test Inox locally

This package was validated on Linux with both GCC and Clang, and is intended to remain portable to Windows 11 with LLVM/Clang, CMake, and PowerShell 7.

## Windows 11 with LLVM/Clang

Prerequisites:

- LLVM/Clang installed and available in `PATH` (`clang --version` and `clang++ --version` must work).
- CMake installed and available in `PATH`.
- PowerShell 7 (`pwsh`) installed.
- Ninja is recommended for a single, portable command line. If Ninja is not installed, use your normal CMake generator and keep the same build/test flow.

From PowerShell 7:

```powershell
cd C:\Projetos\Inox

cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++

cmake --build build --config Debug

pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
```

If `build` already exists from an older generator and CMake reports a generator mismatch, delete only the build directory and configure again:

```powershell
cd C:\Projetos\Inox
Remove-Item .\build -Recurse -Force
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
cmake --build build --config Debug
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
```

To compile and run an Inox program through the temporary Clang-backed driver:

```powershell
.\build\inox.exe --build .\tests\integration\run-hello.inox
.\build\inox.exe --run .\tests\integration\run-hello.inox
```

If you use a multi-config generator such as Visual Studio, the executable may be under `build\Debug\inox.exe` instead of `build\inox.exe`; the test script already checks the usual locations.

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

## Expected result for this package

The current expected test result is:

```text
Summary: 142 passed, 0 failed, 142 total
```
