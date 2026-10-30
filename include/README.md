<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# include/

Public C and C++ headers that programs outside this repository include:

- the C interface of libinoxrt, when programs written in C or C++ call
  Inox code or embed its runtime (the interface with C libraries, FFI, is
  planned after 1.0: constitution FUTURE);
- the declarations a tool needs to read Inox's output, if one is ever
  published as a stable interface.

Headers used only inside the compiler or the runtime stay next to their
sources in `src/compiler/` and `src/runtime/`. A header here is a promise
to users: changing it is a change to a public interface.
