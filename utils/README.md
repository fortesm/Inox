<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# utils/

Programs for the users of Inox, shipped with it or next to it, that are not
the compiler itself:

- a source formatter (`inox-fmt`) and a linter;
- the package tool that finds and downloads Inox packages (FUTURE);
- a language server for editors.

One folder per program: `inox-fmt/`, `inox-lint/`, `inox-pkg/`,
`inox-lsp/`. They reuse the compiler's library (`inox-compiler`), so they
never disagree with the language.

Tools for developing Inox itself (test runners, checkers, generators,
packaging) are in `tools/`; utilities here are for the people who write Inox
programs.
