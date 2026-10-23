# Build, test, and release instructions

How to build the Inox compiler, run the regression suite and the other
quality gates, and produce release packages. Operational guidance only: the
law is `openspec/specs/constitution/` (the quality gates are E19 of its
`engineering.md`), the current state is `docs/STATUS.md`.

## Who needs what

- **Compiler developer**: CMake, Ninja, LLVM/Clang and a C++20 toolchain
  (on Windows also PowerShell 7 and Visual Studio Build Tools with C++ and
  the Windows SDK, which provide the MSVC STL, the UCRT and the linker).
- **User of a prebuilt compiler**: nothing beyond the release package and
  the normal OS runtime libraries (a Windows Release build may need the MSVC
  Redistributable). Checking and emitting IR need no LLVM or clang:
  `inox check examples/hello.inox`, `inox emit-llvm
  examples/put-output-basic.inox`.
- **User who builds or runs programs** (`inox file.inox`, `inox run`,
  `inox build`): `clang` in `PATH` for now; the driver
  delegates object generation and linking to it (`clang++` and the bundled
  `libinoxrt` for programs that use exceptions, Strings or Files).

## Windows development build

Clang with the MSVC ABI (`x86_64-pc-windows-msvc`).

```powershell
cmake --preset windows-clang-msvc
cmake --build --preset windows-clang-msvc-debug
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1 -InoxExe .\build\windows-clang-msvc\Debug\inox.exe
```

Release: `cmake --build --preset windows-clang-msvc-release` (compiler at
`build\windows-clang-msvc\Release\inox.exe`). A Release build must depend on
the Release MSVC/UCRT runtime, never on Debug DLLs (`MSVCP140D.dll`,
`VCRUNTIME140D.dll`, `ucrtbased.dll`); check with
`llvm-objdump -p inox.exe | Select-String "DLL Name"`. Debug builds are never
distributed.

## Linux development build

```bash
cmake --preset linux-clang-debug
cmake --build --preset linux-clang-debug
bash scripts/run-tests.sh build/linux-clang-debug/inox
```

Release: `cmake --preset linux-clang-release && cmake --build --preset
linux-clang-release`. The `linux-clang-debug` preset builds against the
system libstdc++ (14 on the CI runner), which includes fewer headers
transitively than older versions: build with it before pushing.

Every program in `examples/` is built and run; its expected output is
`tests/examples/NAME.out` (input `tests/examples/NAME.in`), so a new
example needs one, and `examples/` holds only programs.

The two runners hold the same checks; CI runs both (Linux, and Windows
natively on windows-2022). Both end with `Summary: N passed, 0 failed`.

## Quality gates (E19)

A green suite alone never proves the absence of regressions. After a
compiler change, run the gates that apply and say in the report which ran:

```bash
# AddressSanitizer + UndefinedBehaviorSanitizer, also with a 1 MiB stack
CXX=g++ cmake -S . -B build-san -G Ninja -DCMAKE_BUILD_TYPE=Debug -DINOX_ENABLE_ASAN=ON -DINOX_ENABLE_UBSAN=ON
cmake --build build-san
(ulimit -s 1024; bash scripts/run-tests.sh build-san/inox)
python3 tools/mutation_fuzz.py build-san/inox --seed 1
python3 tools/mutation_fuzz.py build-san/inox --seed 7
python3 tools/backend_gaps.py build/inox            # 0 BUG; --markdown for docs/BACKEND_GAPS.md
python3 tools/eh_state_balance.py build/inox build/libinoxrt.a
python3 tools/string_leak_check.py build/inox
python3 tools/calc_differential_test.py --inox build/inox   # 2000 expressions and 60 traps
python3 tools/grammar_consistency.py
OPENSPEC_TELEMETRY=0 openspec validate --all --strict --no-interactive
python3 tools/constitution_check.py check
# a refactor that must not change behavior
python3 tools/ir_snapshot.py record build/inox /tmp/snap    # before
python3 tools/ir_snapshot.py compare build/inox /tmp/snap   # after
```

Use `-DINOX_ENABLE_ASAN=ON -DINOX_ENABLE_UBSAN=ON`, not raw
`CMAKE_CXX_FLAGS`, which break the libinoxrt link. The nesting limits of
`src/compiler/parser/Parser.h` were measured with a 1 MiB stack (the Windows
default); `ulimit -s 1024` reproduces it; re-measure before raising a limit
(CANON-19).

Runtime faults: a program that overflows, divides by zero, indexes out of
bounds, etc. prints `Inox runtime error: <category>` on stderr and exits with
status 70; `tests/runtime/*.trap` hold the expected diagnostic.

## Console input by hand

```bash
printf '42\n' | build/inox run tests/integration/input/get-integer.inox            # A=42
printf '40 2 ignored\n' | build/inox run tests/integration/input/getln-two-integers.inox  # S=42
```

```powershell
"42" | .\build\windows-clang-msvc\Debug\inox.exe run .\tests\integration\input\get-integer.inox
```

## When the native build fails

`inox build` and `inox run` (and `inox file.inox`) call clang (and through it the linker). On failure the
error names the tool, its exit code and the first lines of its own messages,
and points to `<output dir>/<name>.toolchain.log` (`INOX_OUTPUT_DIR`, or
`build/inox-artifacts`). A program that needs `libinoxrt` looks for it beside
the compiler (`INOX_RUNTIME_LIB` overrides).

## CMake structure

`cmake/InoxPlatform.cmake`, `InoxCompilerOptions.cmake`,
`InoxWarnings.cmake`, `InoxSanitizers.cmake`, `InoxInstall.cmake`,
`InoxBuildNumber.cmake` (writes the BUILD number at every build) and
`cmake/toolchains/` (real toolchains for the validated targets, stubs for
the others). `CMakePresets.json` defines the validated Windows and Linux
flows.

## Release packages

```bash
bash scripts/package-release.sh Release inox-linux-x64 build-linux/inox
```

```powershell
pwsh -File .\scripts\package-release.ps1     # uses build\Release\inox.exe
```

Each package (`dist/inox-<platform>-x64.zip`, published as a GitHub Release
asset, never committed) holds:

```text
inox-<platform>-x64/
    README.md
    set-inox-env.(sh|ps1)    PATH += bin/, INOX_STDLIB, INOX_OUTPUT_DIR
    bin/inox(.exe)           and libinoxrt.a / inoxrt.lib beside it
    stdlib/                  required by `Use Std.*`
    examples/
    output/
    docs/index.html          the manual
    docs/constitution/       the language law (spec.md, engineering.md, revisions.md)
    licenses/
```

Quick start: Linux `unzip inox-linux-x64.zip -d "$HOME/tools"; cd
"$HOME/tools/inox-linux-x64"; source ./set-inox-env.sh; inox
./examples/hello.inox`. Windows `Expand-Archive .\inox-windows-x64.zip
-DestinationPath C:\Tools; cd C:\Tools\inox-windows-x64;
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass;
.\set-inox-env.ps1; inox .\examples\hello.inox`.

Release checklist (E23): configure; build; run the suite and the gates;
validate the runnable examples and check the frontend-only ones with
`--dump-types`; package; extract into a clean directory; run the package's
examples as a user would; verify library discovery, the output directory,
the documentation and the licenses.

## Git policy

Never commit `build/`, `build-*/`, `dist/` or packages.
