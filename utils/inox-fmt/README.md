<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# utils/inox-fmt/

The formatter: rewrites an Inox file in the canonical layout (indentation,
spacing, the case of keywords and names as declared, one statement per
line, `;` on its own line), without changing what the program means.
Planned; it reuses the compiler's lexer and parser (the `inox-compiler`
library) so it never disagrees with the language.

Planned use: `inox-fmt file.inox` prints the formatted text; `--write`
changes the file; `--check` fails when a file is not formatted (for CI).
