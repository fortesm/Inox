<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Inox

Inox is a compiled, statically typed language for general-purpose and systems programming. It combines modular programming, nominal data types, generics, contracts, and explicit resource ownership.

The language makes several common sources of programming errors easier to identify: implicit changes to data, invalid array accesses, arithmetic failures, and references to objects whose lifetimes have ended. Mutation is expressed through parameter and receiver modes; `Own<T>` and `Link<T>` distinguish ownership from non-owning access; checked operations and contracts provide defined failure paths.

Inox uses short declarations, keyword-based control flow, and explicit but lightweight block boundaries. Routine bodies begin with `:` and end with `;`; the same delimiters organize nested statements. The notation keeps the structure of algorithms visible without requiring lengthy declarations or dense punctuation.

## Language features

- **Types and data:** nominal `Struct` types, associated methods, enumerations, ranges, sets, fixed-size arrays, slices, strings, and numeric types, including exact decimal types.
- **Modularity:** named modules, explicit imports, visibility controls, reusable structures, and generic routines and types.
- **Mutation and lifetime:** immutable value parameters by default, `ref` and `ref mut` parameters, `Self mut` receivers, unique ownership through `Own<T>`, explicit moves, and generation-checked `Link<T>` references.
- **Validation and error handling:** checked arithmetic and indexing, explicit type conversions, exceptions, and preconditions, postconditions, and invariants.
- **Standard library:** modules for text processing, mathematics, console and file operations, dates and times, and system utilities.

The complete language rules are defined in the [language specification](docs/openspec/specs/constitution/spec.md). The [`examples/`](examples/) directory contains programs covering language constructs and library use.

## Build

The compiler is implemented in C++20 and generates LLVM IR. Native executable generation uses Clang. The primary build configurations are Linux x86-64 and Windows x86-64 with the MSVC ABI.

Development builds require CMake 3.25 or later, Ninja, Clang, and a C++20 toolchain. Windows also requires the Visual Studio C++ build tools, Windows SDK, and PowerShell 7.

### Linux

```sh
cmake --preset linux-clang-debug
cmake --build --preset linux-clang-debug
bash tools/scripts/run-tests.sh build/linux-clang-debug/inox
```

### Windows

```powershell
cmake --preset windows-clang-msvc
cmake --build --preset windows-clang-msvc-debug
pwsh -ExecutionPolicy Bypass -File .\tools\scripts\run-tests.ps1 -InoxExe .\build\windows-clang-msvc\Debug\inox.exe
```

Release configurations, environment details, and additional validation procedures are documented in [Build and Test Instructions](docs/BUILD.md).

## Compiler usage

The `inox` command accepts a source file or an explicit operation:

```sh
inox check file.inox       # Parse and analyze a program
inox build file.inox       # Build a native executable
inox run file.inox         # Build and run a program
inox emit-llvm file.inox   # Output LLVM IR
inox interface file.inox   # Display a module's public interface
inox help                  # List commands and options
```

When working with a local Linux development build, use `build/linux-clang-debug/inox` in place of `inox`. Source checking and LLVM IR emission work without a native linker; building and running executables require the Clang toolchain.

## Repository structure

```text
.
├── .claude/                 Project-local agent instructions and workflows
├── .github/                 CI and release workflows
├── cmake/                   Build settings and platform toolchains
├── contrib/                 Editor support, completions, and integrations
├── docs/                    Language and compiler documentation
│   ├── grammar/             Grammar reference
│   ├── history/             Historical specifications and decisions
│   ├── openspec/            Language requirements and change records
│   └── site/                Language documentation site
├── examples/                Inox source examples
├── include/                 Headers for external integration interfaces
├── libs/                    Third-party library area
├── licenses/                License texts and attribution records
├── src/
│   ├── compiler/
│   │   ├── scanner/          Source cursor, byte navigation, and positions
│   │   ├── lexer/            Token recognition and lexical definitions
│   │   ├── parser/           Syntax analysis and AST construction
│   │   ├── ast/              Syntax tree structures and operations
│   │   ├── normalization/    AST normalization
│   │   ├── semantic/         Name resolution and semantic analysis
│   │   ├── types/            Type representations and type context
│   │   ├── tir/              Typed intermediate representation
│   │   ├── codegen/          LLVM IR generation
│   │   ├── exceptions/       Compiler exception support
│   │   ├── main/             Command-line driver
│   │   └── support/          Shared compiler and platform services
│   └── runtime/             Native runtime library (libinoxrt)
├── stdlib/                  Standard library modules
├── tests/                   Language, compiler, and runtime regression tests
│   ├── compiler-unit/       Compiler component tests
│   ├── diagnostics/         Expected diagnostics for invalid programs
│   ├── lexer/               Lexical analysis tests
│   ├── parser/              Syntax analysis tests
│   ├── semantic/            Semantic analysis tests
│   ├── tir/                 Typed IR tests
│   ├── runtime/             Executable behavior and runtime errors
│   └── runtime-unit/        Native runtime component tests
├── tools/                   Build, test, and analysis utilities
└── utils/                   End-user utility area
```

The [compiler architecture](docs/ARCHITECTURE.md) describes the phases, responsibilities, and interfaces between these components.

## Documentation

- [Language specification](docs/openspec/specs/constitution/spec.md) — language syntax and semantics.
- [Language manual](docs/site/index.html) — language documentation.
- [Compiler architecture](docs/ARCHITECTURE.md) — compiler phases and component boundaries.
- [Build and test instructions](docs/BUILD.md) — toolchains, builds, and validation.
- [Implementation status](docs/STATUS.md) — feature coverage and known limitations.
- [Roadmap](docs/ROADMAP.md) and [Changelog](CHANGELOG.md) — releases and planned work.
- [Contribution guidelines](CONTRIBUTING.md) — how to contribute to the repository.

## Project information

**Version:** 0.4.1 (pre-alpha)  
**Maintainer:** Marcelo Fortes  
**License:** [Mozilla Public License 2.0](LICENSE)
