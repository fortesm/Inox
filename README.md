# Inox

Inox is a compiled, strongly typed, case-insensitive programming language inspired by ObjectPascal, Ada, SPARK, Modula, Oberon and Eiffel.

The current compiler is a pre-alpha front end plus a textual LLVM IR backend prototype. The official language specifications are under:

```text
docs/canonical/
```

## Supported host platforms

The compiler is intentionally kept portable C++20 and is expected to build on:

- Windows with Visual Studio Build Tools / MSVC and CMake.
- Linux with GCC or Clang and CMake.

Platform-specific behavior should be isolated in build/test scripts when needed. Do not add C++ preprocessor conditionals for host platforms unless a real platform API difference requires them.

## Build

### Windows / MSVC

Run from a Visual Studio Developer Command Prompt or a terminal where CMake can find MSVC:

```powershell
cd C:\Projetos\Inox
cmake -S . -B build -G "Visual Studio 18 2026"
cmake --build build --config Debug
```

If the build directory is already configured, the build step is enough:

```powershell
cmake --build build --config Debug
```

### Linux / GCC or Clang

```bash
cd /path/to/Inox
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

To force Clang on Linux:

```bash
CC=clang CXX=clang++ cmake -S . -B build-clang -DCMAKE_BUILD_TYPE=Debug
cmake --build build-clang
```

## Tests

### Windows

```powershell
cd C:\Projetos\Inox
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
```

You can pass an explicit compiler path:

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1 -InoxExe .\build\Debug\inox.exe
```

### Linux

```bash
cd /path/to/Inox
./scripts/run-tests.sh
```

You can pass an explicit compiler path:

```bash
./scripts/run-tests.sh ./build/inox
```

## Development workflow

Before committing any compiler change:

1. Build the project for your host platform.
2. Run the full test suite.
3. Add only the files changed by the task.
4. Do not use `git add .`.

Windows example:

```powershell
cmake --build build --config Debug
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
git status --short
```

Linux example:

```bash
cmake --build build
./scripts/run-tests.sh
git status --short
```
