<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# Inox

Inox is a compiled, statically typed programming language under active development. The project includes a compiler written in C++20, an LLVM IR backend, a native runtime library, and a growing standard library.

The language emphasizes explicit state changes, type checking, structured program organization, and defined behavior for operations that can fail. Its language rules are maintained in a versioned specification and exercised through compiler, runtime, and regression tests.

**Current status:** Inox 0.4.0, pre-alpha. The compiler and language are not production-ready. Implemented features, known limitations, and planned work are documented in [Implementation Status](docs/STATUS.md) and the [Roadmap](docs/ROADMAP.md).

## Language overview

The implementation currently supports, among other facilities:

- Nominal types, structures, fixed-size arrays, slices, enumerations, ranges, and sets.
- Functions, subroutines, associated methods, modules, and explicit visibility controls.
- Generic routines and structures, with compile-time specialization.
- Checked arithmetic, explicit conversions, and runtime diagnostics for defined failure conditions.
- Value and reference parameters, including explicitly mutable parameters and receivers.
- Ownership through `Own<T>`, moves, and generation-checked `Link<T>` references.
- Preconditions, postconditions, and structural and loop invariants.
- Exceptions, text I/O, Unicode string operations, and modules in the standard library.

This list is an overview, not a language specification. Refer to the [language specification](docs/openspec/specs/constitution/spec.md) for normative rules and to the [implementation status](docs/STATUS.md) for current coverage.

### Example

```inox
Main :
    PutLn("Hello, Inox")
;
```

Further examples are available in [`examples/`](examples/), including modules, generic types, contracts, data structures, and file operations.

## Compiler and platforms

The compiler processes source files through lexical analysis, parsing, AST normalization, module resolution, contract lowering, semantic analysis, and construction of a typed intermediate representation. Its current backend emits LLVM IR, which is compiled and linked into native programs by an external Clang toolchain. Some code-generation decisions still use the AST while the typed IR migration is being completed.

The primary development and continuous-integration platforms are **Linux x86-64** and **Windows x86-64** (Clang with the MSVC ABI). Other platform configurations in the repository should not be interpreted as validated release targets.

### Build on Linux

Requirements include CMake 3.25 or later, Ninja, Clang, and a C++20 development toolchain.

```sh
cmake --preset linux-clang-debug
cmake --build --preset linux-clang-debug
bash tools/scripts/run-tests.sh build/linux-clang-debug/inox
```

### Build on Windows

The Windows build uses Clang with the MSVC ABI and requires the corresponding C++ build tools and Windows SDK.

```powershell
cmake --preset windows-clang-msvc
cmake --build --preset windows-clang-msvc-debug
pwsh -ExecutionPolicy Bypass -File .\tools\scripts\run-tests.ps1 -InoxExe .\build\windows-clang-msvc\Debug\inox.exe
```

For environment requirements, Release builds, sanitizers, and additional quality gates, see [Build and Test Instructions](docs/BUILD.md).

### Basic commands

After building on Linux:

```sh
build/linux-clang-debug/inox check examples/hello.inox
build/linux-clang-debug/inox run examples/hello.inox
build/linux-clang-debug/inox emit-llvm examples/hello.inox
```

`check` and `emit-llvm` do not require an external linker. Producing or running native executables currently requires the Clang toolchain. Consult `inox help` for the available commands and options.

## Repository structure

The following tree describes the principal directories. Individual implementation files and test fixtures are omitted for readability.

```text
.
├── .claude/                 Agent workflow and project-local assistant configuration
├── .github/                 Continuous integration and release workflows
├── cmake/                   Build configuration, platform support, and toolchains
├── contrib/                 Editor integrations, completion, and contributed tooling
├── docs/                    Architecture, build, status, roadmap, and language documentation
│   ├── grammar/             Grammar reference
│   ├── history/             Historical design and specification records
│   ├── openspec/            Normative specifications and tracked changes
│   └── site/                Language documentation site
├── examples/                Inox example programs
├── include/                 Reserved public headers for future integration interfaces
├── libs/                    Reserved location for third-party dependencies
├── licenses/                Licensing, attribution, and legal notices
├── src/                     Compiler and native runtime implementation
│   ├── compiler/
│   │   ├── ast/              Abstract syntax tree and related transformations
│   │   ├── codegen/          LLVM IR generation and backend support
│   │   ├── exceptions/       Compiler exception-related declarations
│   │   ├── lexer/            Token recognition and lexical definitions
│   │   ├── main/             Command-line driver and compilation orchestration
│   │   ├── normalization/    AST normalization and desugaring
│   │   ├── parser/           Syntax analysis and AST construction
│   │   ├── scanner/          SourceCursor: byte navigation and source positions
│   │   ├── semantic/         Name resolution, type checking, and semantic rules
│   │   ├── support/          Portability and compiler infrastructure
│   │   ├── tir/              Typed intermediate representation and verification
│   │   └── types/            Type representation and interning
│   └── runtime/             Native runtime library (libinoxrt)
├── stdlib/                  Standard library modules written in Inox
├── tests/                   Compiler, diagnostics, integration, and runtime test suites
│   ├── compiler-unit/       Unit tests for compiler components
│   ├── diagnostics/         Expected diagnostic and rejection cases
│   ├── examples/            Expected output for example programs
│   ├── lexer/               Lexical analysis tests
│   ├── parser/              Parsing tests
│   ├── runtime/             Compiled-program behavior and runtime faults
│   ├── runtime-unit/        Native runtime unit tests
│   └── tir/                 Typed IR test cases
├── tools/                   Development, validation, and analysis utilities
└── utils/                   Reserved tools intended for Inox users
```

For the responsibilities and boundaries of each compiler phase, see [Compiler Architecture](docs/ARCHITECTURE.md).

## Documentation and development

- [Language specification](docs/openspec/specs/constitution/spec.md) — normative language requirements.
- [Engineering requirements](docs/openspec/specs/constitution/engineering.md) — compiler design and validation requirements.
- [Compiler architecture](docs/ARCHITECTURE.md) — pipeline, component ownership, and current architectural work.
- [Implementation status](docs/STATUS.md) — implemented capabilities and limitations.
- [Roadmap](docs/ROADMAP.md) and [Changelog](CHANGELOG.md) — planned work and recorded changes.
- [Build and test instructions](docs/BUILD.md) — supported build configurations and validation commands.
- [Contribution guidelines](CONTRIBUTING.md) and [Agent instructions](AGENTS.md) — project development procedures.

Language changes are governed by the project specification and its associated change process. The compiler implementation, examples, and descriptive documentation are expected to remain consistent with the approved specification.

## License

Inox is distributed under the [Mozilla Public License 2.0](LICENSE). Related notices are maintained in [`licenses/`](licenses/).

**Maintainer:** Marcelo Fortes.
