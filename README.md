# Inox

Inox is a strongly typed, compiled programming language for high-integrity software.

The language is designed for codebases where correctness, explicitness, portability, and predictable execution matter. Its design takes inspiration from the safety discipline of Ada/SPARK, the clarity of Modula/Oberon/Pascal-family languages, the practicality of Go and Rust, and the performance-oriented compilation model enabled by LLVM.

Inox is intentionally post-object-oriented. It does not use classes, inheritance trees, Java-style interfaces, duck typing, or mixins as its foundation. The language is being shaped around structs as data, associated methods as behavior, composition over inheritance, explicit mutability, local reasoning, strong static typing, and future contracts/protocols/behaviors for statically checked capabilities without forcing a classical object hierarchy.

The compiler is currently a pre-alpha implementation of the 0.1 language direction. It is useful for studying the syntax, running examples, validating the compiler pipeline, emitting LLVM IR, and exercising a restricted Clang-backed native build/run path.

## Downloading a prebuilt release

Users who only want to try the Inox language do **not** need to build the compiler from source.

Download the latest Windows x64 test package here:

[Download inox-windows-x64.zip](https://github.com/fortesm/Inox/releases/latest/download/inox-windows-x64.zip)

Download the latest Linux x64 test package here:

[Download inox-linux-x64.zip](https://github.com/fortesm/Inox/releases/latest/download/inox-linux-x64.zip)

Each package contains the compiler executable, the bundled standard library, examples, licenses, an output directory, and a local setup script.

## Language tutorial

The complete language tutorial and reference is available in Markdown here:

[Open the Inox language tutorial and reference](docs/LANGUAGE_REFERENCE.md)

An HTML copy is also kept in the repository for local browsing:

```text
docs/index.html
```

Prebuilt release packages include the same documentation under:

```text
docs/LANGUAGE_REFERENCE.md
docs/index.html
```

## Usage profiles and requirements

| Profile                                                                             | Needs Clang/LLVM? |                                           Needs a C++ compiler? | Notes                                                                                              |
| ----------------------------------------------------------------------------------- | ----------------: | --------------------------------------------------------------: | -------------------------------------------------------------------------------------------------- |
| Inox compiler developer                                                             |               Yes |                                                             Yes | Builds `inox.exe` or `inox` from the C++ source code.                                              |
| User who only wants to run a prebuilt compiler                                      |                No |                                                              No | Needs only the Release executable and the normal operating-system/runtime libraries.               |
| User who wants to use `inox --build` or `inox --run` to generate native executables |      Yes, for now | Not to develop the compiler, but a native toolchain is required | The current driver delegates final native executable generation to external tools such as `clang`. |

## Testing the language on Windows

Extract the Windows package and configure the current PowerShell session:

```powershell
Expand-Archive .\inox-windows-x64.zip -DestinationPath C:\Tools
cd C:\Tools\inox-windows-x64

Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\set-inox-env.ps1
```

Then run:

```powershell
inox .\examples\hello.inox
inox --emit-llvm .\examples\llvm-put-output-basic.inox
```

Expected result for `hello.inox`:

```text
parse ok
semantic ok
```

The setup script configures:

```text
PATH            includes the local bin/ directory
INOX_STDLIB     points to the bundled stdlib/ directory
INOX_OUTPUT_DIR points to the bundled output/ directory
```

## Testing the language on Linux

Extract the Linux package and source the local environment script:

```bash
unzip inox-linux-x64.zip -d "$HOME/tools"
cd "$HOME/tools/inox-linux-x64"

source ./set-inox-env.sh
```

Then run:

```bash
inox ./examples/hello.inox
inox --emit-llvm ./examples/llvm-put-output-basic.inox
```

Expected result for `hello.inox`:

```text
parse ok
semantic ok
```

## Building native programs with Inox

The prebuilt compiler can parse, type-check, and emit LLVM IR without Clang or a C++ compiler.

Native executable generation is different. The temporary 0.1 driver can delegate final native executable generation to `clang`:

```bash
inox --build examples/llvm-put-output-basic.inox
inox --run examples/llvm-put-output-basic.inox
```

On Windows, the equivalent commands are:

```powershell
inox --build examples\llvm-put-output-basic.inox
inox --run examples\llvm-put-output-basic.inox
```

This mode currently requires `clang` in `PATH`.

When using a prebuilt package, generated LLVM IR and executables are written to:

```text
output/
```

When running from the source repository without `INOX_OUTPUT_DIR`, the default artifact directory is:

```text
build/inox-artifacts/
```

Override the output directory with:

```powershell
$env:INOX_OUTPUT_DIR = "C:\Temp\inox-out"
```

or on Linux:

```bash
export INOX_OUTPUT_DIR="$HOME/tmp/inox-out"
```

## Standard library and runtime status

The initial standard library lives under `stdlib/` and is intentionally minimal.

The `Std.*.inox` files are early standard-library modules and documentation anchors, not a complete standalone runtime library.

Inox does not yet provide a dedicated runtime library such as:

```text
libinoxrt.a
inoxrt.lib
inox_runtime.dll
```

At this stage:

* the Inox compiler itself is a native C++ program;
* `Put` and `PutLn` are compiler/runtime-lowered facilities used by the current backend;
* generated Inox programs may still rely on the host platform toolchain and system runtimes;
* the standard library is bundled with release packages so the compiler can locate standard modules consistently;
* `INOX_STDLIB` can be used to point the compiler to the standard-library directory;
* `INOX_OUTPUT_DIR` can be used to select the directory where build artifacts are written.

Future work should separate clearly:

* the Inox compiler executable;
* the Inox standard library;
* the Inox runtime library;
* platform-specific backend/linker integration;
* optional SDK packaging.

## Building the compiler from source

Developers who want to modify or build the Inox compiler need a native C++ toolchain.

### Windows development prerequisites

Recommended Windows setup:

* Git for Windows;
* PowerShell 7;
* CMake;
* Ninja;
* LLVM/Clang;
* Visual Studio Build Tools with C++;
* Windows SDK.

The compiler is built with Clang targeting the MSVC ABI:

```text
x86_64-pc-windows-msvc
```

Clang is used as the C++ compiler, while Visual Studio Build Tools provide the native Windows platform components required by that target:

* Windows SDK headers and libraries;
* MSVC C/C++ runtime;
* MSVC STL;
* native linker support;
* platform import libraries.

### Installing the Windows development toolchain

Install the required tools with `winget` from an elevated PowerShell session:

```powershell
winget install -e --id Microsoft.PowerShell --accept-package-agreements --accept-source-agreements
winget install -e --id Git.Git --accept-package-agreements --accept-source-agreements
winget install -e --id Kitware.CMake --accept-package-agreements --accept-source-agreements
winget install -e --id Ninja-build.Ninja --accept-package-agreements --accept-source-agreements
winget install -e --id LLVM.LLVM --accept-package-agreements --accept-source-agreements
```

Install Visual Studio Build Tools with the C++ workload:

```powershell
winget install -e --id Microsoft.VisualStudio.2022.BuildTools `
  --accept-package-agreements `
  --accept-source-agreements `
  --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

After installation, open a new terminal and check:

```powershell
pwsh --version
git --version
cmake --version
ninja --version
clang --version
clang++ --version
```

When building on Windows, prefer using:

```text
Developer PowerShell for VS 2022
```

or a PowerShell session where the Visual Studio C++ environment has been initialized.

### Recommended Windows build

From the repository root:

```powershell
cmake -S . -B build -G "Ninja Multi-Config" -DCMAKE_CXX_COMPILER=clang++
cmake --build build --config Debug
pwsh -ExecutionPolicy Bypass -File .\scripts\run-tests.ps1
```

Expected result:

```text
Summary: 142 passed, 0 failed, 142 total
```

Debug compiler path:

```text
build/Debug/inox.exe
```

Release compiler path:

```text
build/Release/inox.exe
```

### Linux / Unix-like development build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
cmake --build build
bash scripts/run-tests.sh
```

Release build:

```bash
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build-linux
```

The generated compiler is usually:

```text
build/inox
```

or, with the `build-linux` directory shown above:

```text
build-linux/inox
```

The `build/` and `build-linux/` directories are generated and must not be versioned.

## Packaging releases

Windows package:

```powershell
cmake --build build --config Release
pwsh -ExecutionPolicy Bypass -File .\scripts\package-release.ps1
```

Generated package:

```text
dist/inox-windows-x64.zip
```

Linux package:

```bash
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build build-linux
bash scripts/package-release.sh Release inox-linux-x64 build-linux/inox
```

Generated package:

```text
dist/inox-linux-x64.zip
```

Release ZIP files must be uploaded as GitHub Release assets. They should not be committed to the Git repository.

## Package layout

Windows:

```text
inox-windows-x64/
    README.md
    set-inox-env.ps1
    bin/
        inox.exe
    stdlib/
    examples/
    output/
    docs/
        LANGUAGE_REFERENCE.md
        index.html
    licenses/
```

Linux:

```text
inox-linux-x64/
    README.md
    set-inox-env.sh
    bin/
        inox
    stdlib/
    examples/
    output/
    docs/
        LANGUAGE_REFERENCE.md
        index.html
    licenses/
```

## Publishing releases on GitHub

Upload these files as GitHub Release assets:

```text
inox-windows-x64.zip
inox-linux-x64.zip
```

Public download links:

```text
https://github.com/fortesm/Inox/releases/latest/download/inox-windows-x64.zip
https://github.com/fortesm/Inox/releases/latest/download/inox-linux-x64.zip
```

Recommended release title:

```text
Inox 0.1 Test Release
```

Recommended tag format:

```text
v0.1.0-test
```

## Documentation

Start here:

* `docs/LANGUAGE_REFERENCE.md` — complete language tutorial/reference in Markdown.
* `docs/index.html` — browsable HTML copy of the same tutorial/reference.
* `docs/canonical/language-reference.md` — consolidated tutorial/reference.
* `docs/development/toolchain.md` — requirements for compiler developers and language testers.
* `docs/release/prebuilt-usage.md` — packaging and usage guide for prebuilt releases.
* `docs/decisions/ADR-0006-inox-0.1-constitution.md` — frozen 0.1 decisions.
* `AGENTS.md` — operational instructions for AI agents.
* `docs/open-questions/OPEN_QUESTIONS.md` — deferred 0.2+ architecture topics.

## Project layout

```text
Inox/
    contrib/
    docs/
    examples/
    grammar/
    include/
    libs/
    licenses/
    scripts/
    src/
    stdlib/
    tests/
    tools/
    utils/
```

### `src/`

Compiler implementation.

### `stdlib/`

Early Inox standard library files.

### `examples/`

Example Inox programs used for manual testing and regression coverage.

### `tests/`

Compiler regression tests.

### `docs/`

Language, compiler, backend, runtime, and release documentation.

### `scripts/`

Build, test, and packaging helper scripts.

### `licenses/`

License files and third-party license material.

### `contrib/`

Reserved for external contributions and optional experiments.

### `include/`

Reserved for public C/C++ headers or integration interfaces.

### `libs/`

Reserved for internal libraries or future vendored dependencies.

### `tools/`

Reserved for development tools related to the compiler and language.

### `utils/`

Reserved for small helper utilities.

## Design stance

Inox rejects unsafe defaults:

* universal null;
* implicit narrowing;
* integer wraparound guarantees;
* unchecked bounds;
* implicit aliasing;
* classes;
* inheritance;
* Java-style interfaces.

Future work includes:

* modules;
* arrays;
* vectors;
* sets;
* contracts;
* protocols;
* behaviors;
* arenas;
* borrowing;
* unsafe boundaries;
* structured parallelism.

## Version control policy

The following directories are generated and must not be versioned:

```text
build/
build-linux/
dist/
```

Local source ZIP snapshots should not be versioned:

```text
Inox-source-*.zip
Inox.zip
```

Generated test outputs should not be versioned unless they are intentional expected-output fixtures.

Release packages:

```text
dist/inox-windows-x64.zip
dist/inox-linux-x64.zip
```

must be uploaded as GitHub Release assets, not committed to the repository.

## License

Inox is free software licensed under the Mozilla Public License Version 2.0 (MPL-2.0).

See `LICENSE` for the full license text.

Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.

The project does not use the MPL 2.0 "Incompatible With Secondary Licenses" notice.

See also:

* `NOTICE.md`
* `AUTHORS.md`
* `CONTRIBUTING.md`
* `TRADEMARK.md`
