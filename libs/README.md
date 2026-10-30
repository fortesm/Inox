<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# libs/

Third-party libraries kept in the repository (vendored) because Inox
builds or ships with them, each in its own folder with its license and its
exact version:

- a dependency that cannot be expected on the build machine;
- a library bundled into the release packages.

Inox has none today: it uses only the C++20 standard library, and LLVM
and Clang are found on the system. Before adding a library, record why it
cannot be a system dependency, and add its license to `licenses/`. Inox's
own libraries are not vendored here: the standard library is `stdlib/`,
and community Inox libraries will come with the package tool (FUTURE).
