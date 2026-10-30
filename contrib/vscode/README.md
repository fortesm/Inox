<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# contrib/vscode/

A Visual Studio Code extension for Inox: syntax highlighting (a TextMate
grammar, `inox.tmLanguage.json`), the language configuration (comments
`==`, brackets, the `;` that closes blocks), snippets, and later the client
of the language server (`utils/inox-lsp/`).

Expected layout: `package.json`, `syntaxes/inox.tmLanguage.json`,
`language-configuration.json`, `snippets/inox.json`, a README for users.
The keywords come from the constitution (CANON-2); keep them in step with
the lexer.
