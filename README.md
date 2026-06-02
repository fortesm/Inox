# Inox

Inox is a strongly typed, compiled programming language designed for robust, explicit, and maintainable systems software.

The language is being designed with the ambition of combining the clarity of Pascal/Ada-style syntax, the safety discipline expected from critical software engineering, and the performance-oriented compilation model of modern LLVM-based toolchains.

Inox favors explicit structure over implicit magic. Its syntax is intentionally compact, but not cryptic. Blocks are closed with `;`, declarations are readable, types are strong, unsafe defaults are rejected, and semantic mistakes such as accidental shadowing are treated as errors rather than tolerated as style choices.

The long-term goal is to make Inox suitable for software where correctness, portability, readability, and predictable execution matter: infrastructure, financial systems, scientific computing, industrial automation, cryptography-oriented systems, embedded tooling, and other domains where hidden runtime behavior is unacceptable.

Inox is not a classical object-oriented language. It does not build its design around classes, inheritance trees, or Java-style interfaces. Its direction is composition-first, with data structures, associated methods, explicit capabilities, and future contracts/protocols/behaviors providing reuse without forcing a class hierarchy.

The current compiler is an early-stage implementation written in C++ with a textual LLVM IR backend. It already supports a restricted executable subset of the language and is evolving through regression tests, canonical documentation, and incremental backend work.

## Downloading a prebuilt Windows test release

Users who only want to test the Inox compiler and the language do **not** need to build the compiler from source.

Download the latest Windows x64 test package here:

[Download inox-windows-x64.zip](https://github.com/fortesm/Inox/releases/latest/download/inox-windows-x64.zip)

The package is intended for people who want to try the language quickly from a ready-to-run `inox.exe`.

The Windows test package has this layout:

```text
inox-windows-x64/
    bin/
        inox.exe
    stdlib/
        README.md
        Std.Core.inox
        Std.Debug.inox
        Std.IO.inox
        Std.Math.inox
    examples/
    output/
        README.txt
    licenses/
    README.md
    set-inox-env.ps1
```

After extracting the ZIP file, open PowerShell inside the extracted `inox-windows-x64` directory and run:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\set-inox-env.ps1
```

This configures the current PowerShell session:

```text
PATH            includes the local bin/ directory
INOX_STDLIB     points to the bundled stdlib/ directory
INOX_OUTPUT_DIR points to the bundled output/ directory
```

Then test the compiler:

```powershell
inox examples\hello.inox
```

Expected result:

```text
parse ok
semantic ok
```

To emit LLVM IR:

```powershell
inox --emit-llvm examples\llvm-put-output-basic.inox
```

If a native toolchain is installed, the following commands can also build and run a native executable:

```powershell
inox --build examples\llvm-put-output-basic.inox
inox --run examples\llvm-put-output-basic.inox
```

Generated native executables and intermediate files are written to:

```text
output/
```

For example:

```text
output/
    llvm-put-output-basic.exe
    llvm-put-output-basic.ll
```

## Usage profiles and requirements

| Profile                                                                                      | Needs Clang/LLVM? |                                           Needs a C++ compiler? | Notes                                                                                                   |
| -------------------------------------------------------------------------------------------- | ----------------: | --------------------------------------------------------------: | ------------------------------------------------------------------------------------------------------- |
| Inox compiler developer                                                                      |               Yes |                                                             Yes | Builds `inox.exe` from the C++ source code.                                                             |
| User who only wants to run a prebuilt `inox.exe`                                             |                No |                                                              No | Needs only the Release executable and the normal system/MSVC runtimes.                                  |
| User who wants to use `inox.exe --build` or `inox.exe --run` to generate native `.exe` files |      Yes, for now | Not to develop the compiler, but a native toolchain is required | If the driver calls `clang`, `lld`, `link.exe`, or other external tools, those tools must be installed. |

## Testing the language with a prebuilt package

To test the language with a prebuilt Windows package:
=======
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

| Profile | Needs Clang/LLVM? | Needs a C++ compiler? | Notes |
|---|---:|---:|---|
| Inox compiler developer | Yes | Yes | Builds `inox.exe` or `inox` from the C++ source code. |
| User who only wants to run a prebuilt compiler | No | No | Needs only the Release executable and the normal operating-system/runtime libraries. |
| User who wants to use `inox --build` or `inox --run` to generate native executables | Yes, for now | Not to develop the compiler, but a native toolchain is required | The current driver delegates final native executable generation to external tools such as `clang`. |

## Testing the language on Windows

Extract the Windows package and configure the current PowerShell session:
>>>>>>> 5c51dcb (Updated docs and releases.)

```powershell
Expand-Archive .\inox-windows-x64.zip -DestinationPath C:\Tools
cd C:\Tools\inox-windows-x64

Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\set-inox-env.ps1
<<<<<<< HEAD

inox .\examples\hello.inox
inox --emit-llvm .\examples\llvm-put-output-basic.inox
=======
>>>>>>> 5c51dcb (Updated docs and releases.)
```

Then run:

```powershell
<<<<<<< HEAD
$env:INOX_STDLIB = "C:\Tools\inox-windows-x64\stdlib"
$env:Path += ";C:\Tools\inox-windows-x64\bin"

=======
>>>>>>> 5c51dcb (Updated docs and releases.)
inox .\examples\hello.inox
inox --emit-llvm .\examples\llvm-put-output-basic.inox
```

<<<<<<< HEAD
The prebuilt `inox.exe` Release binary does **not** require Clang, LLVM, or a C++ compiler merely to parse, type-check, or emit LLVM IR.

On Windows, the prebuilt Release binary may require the normal Microsoft Visual C++ Redistributable runtime.

## Important distinction: testing vs building native programs

A prebuilt `inox.exe` can parse, semantically check, and emit LLVM IR without requiring the user to build the compiler from source.

For example, these commands do not require the user to compile the Inox compiler:

```powershell
inox examples\hello.inox
inox --emit-llvm examples\llvm-put-output-basic.inox
```

However, building native executables from Inox programs may still require external native toolchain tools during the current development stage.

Commands such as:
=======
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
>>>>>>> 5c51dcb (Updated docs and releases.)

```powershell
inox --build examples\llvm-put-output-basic.inox
inox --run examples\llvm-put-output-basic.inox
```

<<<<<<< HEAD
may require Clang/LLVM, LLD, the MSVC linker, or another supported native toolchain to be installed and available in `PATH`.

This requirement is temporary and reflects the current stage of the compiler. A future Inox SDK may package the required backend/linker tools or provide a more complete standalone build pipeline.

## Native build/run of Inox programs

The temporary 0.1 driver can delegate native executable generation to Clang.

```powershell
inox --build examples\llvm-put-output-basic.inox
inox --run examples\llvm-put-output-basic.inox
```

This mode currently requires `clang` in `PATH`.

Generated LLVM IR and executables are written to the default artifact directory.

When running from the source repository, the default artifact directory is:
=======
This mode currently requires `clang` in `PATH`.

When using a prebuilt package, generated LLVM IR and executables are written to:

```text
output/
```

When running from the source repository without `INOX_OUTPUT_DIR`, the default artifact directory is:
>>>>>>> 5c51dcb (Updated docs and releases.)

```text
build/inox-artifacts/
```

<<<<<<< HEAD
When running from the prebuilt Windows test package, `set-inox-env.ps1` sets:

```text
output/
```

Override that output directory with:
=======
Override the output directory with:
>>>>>>> 5c51dcb (Updated docs and releases.)

```powershell
$env:INOX_OUTPUT_DIR = "C:\Temp\inox-out"
```

<<<<<<< HEAD
## Runtime and standard library status
=======
or on Linux:

```bash
export INOX_OUTPUT_DIR="$HOME/tmp/inox-out"
```

## Standard library and runtime status
>>>>>>> 5c51dcb (Updated docs and releases.)

The initial standard library lives under `stdlib/` and is intentionally minimal.

The `Std.*.inox` files are early standard-library modules and documentation anchors, not a complete standalone runtime library.

Inox does not yet provide a dedicated runtime library such as:

```text
libinoxrt.a
inoxrt.lib
inox_runtime.dll
```

At this stage:

<<<<<<< HEAD
* the Inox compiler itself is a native C++ program;
* `Put` and `PutLn` are compiler/runtime-lowered facilities used by the current backend;
* generated Inox programs may still rely on the host platform toolchain and system runtimes;
* the standard library is bundled with the test release so that the compiler can locate standard modules consistently;
* `INOX_STDLIB` can be used to point the compiler to the standard library directory;
* `INOX_OUTPUT_DIR` can be used to select the directory where build artifacts are written.

Future work should separate clearly:

* the Inox compiler executable;
* the Inox standard library;
* the Inox runtime library;
* platform-specific backend/linker integration;
* optional SDK packaging.

## Environment variables

### `INOX_STDLIB`

`INOX_STDLIB` points to the directory containing the Inox standard library files.

In the Windows test package, `set-inox-env.ps1` configures it automatically:

```powershell
$env:INOX_STDLIB = "<release-root>\stdlib"
```

Manual example:

```powershell
$env:INOX_STDLIB = "C:\Tools\inox-windows-x64\stdlib"
```

### `INOX_OUTPUT_DIR`

`INOX_OUTPUT_DIR` points to the directory where native build artifacts should be written.

In the Windows test package, `set-inox-env.ps1` configures it automatically:

```powershell
$env:INOX_OUTPUT_DIR = "<release-root>\output"
```

Manual example:

```powershell
$env:INOX_OUTPUT_DIR = "C:\Tools\inox-windows-x64\output"
```

If `INOX_OUTPUT_DIR` is not set, the compiler uses its default output directory.
=======
- the Inox compiler itself is a native C++ program;
- `Put` and `PutLn` are compiler/runtime-lowered facilities used by the current backend;
- generated Inox programs may still rely on the host platform toolchain and system runtimes;
- the standard library is bundled with release packages so the compiler can locate standard modules consistently;
- `INOX_STDLIB` can be used to point the compiler to the standard-library directory;
- `INOX_OUTPUT_DIR` can be used to select the directory where build artifacts are written.

Future work should separate clearly:

- the Inox compiler executable;
- the Inox standard library;
- the Inox runtime library;
- platform-specific backend/linker integration;
- optional SDK packaging.
>>>>>>> 5c51dcb (Updated docs and releases.)

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

<<<<<<< HEAD
The `build/` directory is generated and must not be versioned.

### Build Release on Windows

```powershell
cmake --build build --config Release
```

The Release executable is:

```text
build/Release/inox.exe
```

### Inspect Release dependencies on Windows

```powershell
llvm-objdump -p .\build\Release\inox.exe | Select-String "DLL Name" -NoEmphasis
```

A normal Release build should depend on Release MSVC/UCRT runtime libraries such as:

```text
MSVCP140.dll
VCRUNTIME140.dll
KERNEL32.dll
api-ms-win-crt-*.dll
```

It should not depend on Debug runtime DLLs such as:

```text
MSVCP140D.dll
VCRUNTIME140D.dll
ucrtbased.dll
```

It also should not depend on LLVM dynamic libraries unless the compiler is explicitly changed to link against LLVM dynamically.

## Linux / Unix-like development build

On Linux or another Unix-like system, developers need:

* Git;
* CMake;
* Ninja or Make;
* Clang or another supported C++ compiler;
* the usual system development tools.

A typical Clang/Ninja build is:
=======
### Linux / Unix-like development build
>>>>>>> 5c51dcb (Updated docs and releases.)

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
cmake --build build
bash scripts/run-tests.sh
```

<<<<<<< HEAD
The generated compiler is usually:

```text
build/inox
```

## Packaging a Windows test release

The Windows release package is generated from a Release build.

First build the Release compiler:

```powershell
cmake --build build --config Release
```

Then generate the package:

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\package-release.ps1
```

The generated ZIP is:
=======
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
>>>>>>> 5c51dcb (Updated docs and releases.)

```text
dist/inox-windows-x64.zip
```

<<<<<<< HEAD
This ZIP is the file that should be uploaded as a GitHub Release asset.

It should not be committed to the Git repository.

## Windows test release package layout

The generated package has this layout:

```text
inox-windows-x64/
    bin/
        inox.exe
    stdlib/
        README.md
        Std.Core.inox
        Std.Debug.inox
        Std.IO.inox
        Std.Math.inox
    examples/
    output/
        README.txt
    licenses/
    README.md
    set-inox-env.ps1
```

The `bin/` directory contains the prebuilt compiler.

The `stdlib/` directory contains the bundled standard-library files required by the compiler.

The `examples/` directory contains sample Inox source files.

The `output/` directory is the default output directory for generated native programs and intermediate files.

The `set-inox-env.ps1` script configures the current PowerShell session to use the release package.

## Publishing the Windows test release on GitHub

The release ZIP should be uploaded to GitHub Releases as:

```text
inox-windows-x64.zip
```

The public download link is:

```text
https://github.com/fortesm/Inox/releases/latest/download/inox-windows-x64.zip
```

The link is valid when a GitHub Release exists and contains an asset named exactly:

```text
inox-windows-x64.zip
=======
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
>>>>>>> 5c51dcb (Updated docs and releases.)
```

Recommended release title:

```text
<<<<<<< HEAD
Inox Windows x64 Test Release
=======
Inox 0.1 Test Release
>>>>>>> 5c51dcb (Updated docs and releases.)
```

Recommended tag format:

```text
v0.1.0-test
```
<<<<<<< HEAD

## Running examples from the repository

From the repository root, after building the compiler:

```powershell
.\build\Debug\inox.exe examples\hello.inox
```

To emit LLVM IR:

```powershell
.\build\Debug\inox.exe --emit-llvm examples\llvm-put-output-basic.inox
```

To build and run a native executable, if the required native toolchain is available:

```powershell
.\build\Debug\inox.exe --build examples\llvm-put-output-basic.inox
.\build\Debug\inox.exe --run examples\llvm-put-output-basic.inox
```

Release build equivalent:

```powershell
.\build\Release\inox.exe examples\hello.inox
```

## Running examples from the Windows test release

After extracting `inox-windows-x64.zip`, enter the extracted package directory:

```powershell
cd C:\Tools\inox-windows-x64
```

Configure the current PowerShell session:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\set-inox-env.ps1
```

Run a basic example:

```powershell
inox examples\hello.inox
```

Emit LLVM IR:

```powershell
inox --emit-llvm examples\llvm-put-output-basic.inox
```

Build a native executable, if the required native toolchain is installed:

```powershell
inox --build examples\llvm-put-output-basic.inox
```

Run the native executable through the Inox driver:

```powershell
inox --run examples\llvm-put-output-basic.inox
```

Generated files are written to:

```text
output/
```

## Language tutorial

The language tutorial and browsable manual are available here:

[Open the Inox HTML language tutorial](docs/site/index.html)

The tutorial should be kept synchronized with the compiler implementation and the canonical language reference.
=======
>>>>>>> 5c51dcb (Updated docs and releases.)

## Documentation

Start here:

<<<<<<< HEAD
* `docs/canonical/language-reference.md` — consolidated tutorial/reference.
* `docs/development/toolchain.md` — requirements for compiler developers and language testers.
* `docs/release/prebuilt-usage.md` — packaging and usage guide for prebuilt releases.
* `docs/site/index.html` — browsable HTML manual.
* `docs/decisions/ADR-0006-inox-0.1-constitution.md` — frozen 0.1 decisions.
* `AGENTS.md` — operational instructions for AI agents.
* `docs/open-questions/OPEN_QUESTIONS.md` — deferred 0.2+ architecture topics.
=======
- `docs/LANGUAGE_REFERENCE.md` — complete language tutorial/reference in Markdown.
- `docs/index.html` — browsable HTML copy of the same tutorial/reference.
- `docs/canonical/language-reference.md` — consolidated tutorial/reference.
- `docs/development/toolchain.md` — requirements for compiler developers and language testers.
- `docs/release/prebuilt-usage.md` — packaging and usage guide for prebuilt releases.
- `docs/decisions/ADR-0006-inox-0.1-constitution.md` — frozen 0.1 decisions.
- `AGENTS.md` — operational instructions for AI agents.
- `docs/open-questions/OPEN_QUESTIONS.md` — deferred 0.2+ architecture topics.
>>>>>>> 5c51dcb (Updated docs and releases.)

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

<<<<<<< HEAD
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
=======
- universal null;
- implicit narrowing;
- integer wraparound guarantees;
- unchecked bounds;
- implicit aliasing;
- classes;
- inheritance;
- Java-style interfaces.

Future work includes:

- modules;
- arrays;
- vectors;
- sets;
- contracts;
- protocols;
- behaviors;
- arenas;
- borrowing;
- unsafe boundaries;
- structured parallelism.
>>>>>>> 5c51dcb (Updated docs and releases.)

## Version control policy

The following directories are generated and must not be versioned:

```text
build/
<<<<<<< HEAD
=======
build-linux/
>>>>>>> 5c51dcb (Updated docs and releases.)
dist/
```

Local source ZIP snapshots should not be versioned:

```text
Inox-source-*.zip
Inox.zip
```

Generated test outputs should not be versioned unless they are intentional expected-output fixtures.

<<<<<<< HEAD
The Windows release package:

```text
dist/inox-windows-x64.zip
```

must be uploaded as a GitHub Release asset, not committed to the repository.
=======
Release packages:

```text
dist/inox-windows-x64.zip
dist/inox-linux-x64.zip
```

must be uploaded as GitHub Release assets, not committed to the repository.
>>>>>>> 5c51dcb (Updated docs and releases.)

## License

Inox is free software licensed under the Mozilla Public License Version 2.0 (MPL-2.0).

See `LICENSE` for the full license text.

Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.

The project does not use the MPL 2.0 "Incompatible With Secondary Licenses" notice.

See also:

<<<<<<< HEAD
* `NOTICE.md`;
* `AUTHORS.md`;
* `CONTRIBUTING.md`;
* `TRADEMARK.md`.
=======
- `NOTICE.md`
- `AUTHORS.md`
- `CONTRIBUTING.md`
- `TRADEMARK.md`
>>>>>>> 5c51dcb (Updated docs and releases.)
