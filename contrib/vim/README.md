<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# contrib/vim/

Vim and Neovim support for Inox: file type detection (`*.inox`), syntax
highlighting, indentation and comment settings (`==`).

Expected layout, as a Vim package: `ftdetect/inox.vim`, `syntax/inox.vim`,
`indent/inox.vim`, `ftplugin/inox.vim`. Neovim may also use the tree-sitter
grammar of `contrib/tree-sitter/` when it exists.
