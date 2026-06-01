# Inox Vision and Rationale

Inox is a compiled, strongly typed, post-object-oriented systems language for software where silent failure is unacceptable.

Its goal is not to imitate one existing language. Inox deliberately takes inspiration from several traditions and rejects defaults that are unsafe, obsolete, ambiguous, or hostile to large-scale engineering.

## Language references and influences

Inox draws selectively from:

- C and C++ for performance, low-level realism, and convenient operators, while rejecting undefined behavior as a language design principle.
- Ada and SPARK for robustness, soundness, contract-oriented thinking, and mission-critical software discipline.
- Modula-2, Modula-3, Oberon, Component Pascal, and Zonnon for modules, clarity, restraint, and excellent ideas that have been neglected over time.
- Eiffel and Sather for correctness, contracts, and design-by-contract principles.
- Chapel for structured high-performance parallelism and data-locality ideas.
- modern Object Pascal, Delphi, and Free Pascal for productivity and useful Pascal-family ergonomics.
- Go for composition, slices, simple concurrency ideas, and array/slice ergonomics, while rejecting implicit aliasing hazards and GC-dependent latency as core language assumptions.
- Rust for ownership, explicit mutability, composition, and memory safety without GC.
- Swift, Kotlin, C#, and Java for modern ergonomics and mature ecosystem lessons.
- Vala where it offers useful systems-programming ergonomics.
- Julia for mathematics, scientific computing, finance, and numerical expressiveness.
- Perl, Ruby, Python, and PHP for expressive manipulation of strings, lists, maps, reductions, regular expressions, and high-level data transformation.

Inox drinks from these sources, but does not copy their mistakes.

## Mission-critical target domain

Inox is designed for domains such as aviation, air-traffic control, high-precision industry, finance, stock exchanges, cryptoasset infrastructure, international monetary systems, scientific computing, aerospace, medicine, hospital machinery, nuclear and hydroelectric plants, electrical grid infrastructure, cryptography, and large-scale parallel computation.

In such domains, failure modes like buffer overflow, null dereference, use-after-free, silent integer overflow, silent division by zero, accidental mutation, unchecked indexing, implicit narrowing, hidden aliasing, and nondeterministic latency can cost lives, destroy capital, or compromise infrastructure.

## Core safety stance

Inox 0.1 establishes these defaults:

- no universal `null` or `nil`;
- no unsafe pointers in the safe language core;
- no silent integer overflow as guaranteed semantics;
- no integer `/`; use explicit `div` and `mod`;
- no unchecked array bounds;
- no implicit narrowing conversions;
- no implicit aliasing for future `Vector[T]`;
- parameters are immutable by default;
- mutating methods require `Self mut`;
- structs are data, not classes;
- associated methods provide behavior without inheritance;
- contracts/protocols/behaviors are future static capability checks, not Java-style interfaces.

## Post-object-oriented design

Inox has no classes, classical inheritance, Java-style interfaces, mixins, duck typing, or class taxonomies. It keeps the ergonomic call form `Object.Method(args)` without turning data into objects in the classical OO sense.

The foundation is:

- `Struct` for data;
- free functions and subroutines;
- associated methods declared outside structs;
- composition instead of inheritance;
- future contracts/protocols/behaviors for static capability checks;
- strong nominal typing.

## Performance model

LLVM is the backend. The compiler itself must remain portable C++20 and build on Windows/MSVC and Linux/GCC or Clang. Future portability should account for FreeBSD, Solaris, AIX, HP-UX, UnixWare, and other Unix systems where realistic.

Inox should not rely on a tracing GC for the language core. Future memory management work should prefer explicit ownership, moves, arenas, deterministic resource management, and controlled borrowing.

## Agent rule

When a design question is not covered by the canonical documentation, do not infer from another language. Ask for a language decision and update the canonical specification, ADRs, manual HTML, and tests.
