<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# contrib/tree-sitter/

A tree-sitter grammar for Inox (`tree-sitter-inox`): one grammar that gives
syntax highlighting, folding and selection to Neovim, Helix, Zed and other
editors, and that GitHub can use to highlight Inox code. It mirrors
`docs/grammar/grammar.ebnf`, which mirrors the constitution.

Expected layout: `grammar.js`, `queries/highlights.scm`,
`queries/indents.scm`, `test/corpus/` with Inox snippets and their trees.
