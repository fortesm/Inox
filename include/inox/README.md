<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# include/inox/

The public C headers of Inox, included as `#include <inox/inox.h>`; the
`inox/` folder keeps them from clashing with other libraries' headers. They
are for C and C++ programs that call Inox code or embed its runtime, through
the interface with C planned after 1.0 (constitution FUTURE). None exists
yet; this is the planned layout:

- `inox.h` — the one header a C program includes; it includes the others.
- `version.h` — the version of Inox the headers belong to
  (MAJOR.MINOR.RELEASE, VERSIONING).
- `types.h` — the C side of Inox's types: integers of fixed width, Bool,
  Char (a code point), Currency and Crypto, String and File handles.
- `runtime.h` — starting and stopping the runtime, Strings (create, read,
  release), and the console.
- `errors.h` — how an Inox exception or runtime fault reaches C code.

Rules: plain C (C11), usable from C++ (`extern "C"`); no compiler internals;
every name prefixed `inox_` or `INOX_`; a change to a published header is a
change to a public interface, recorded in the CHANGELOG.
