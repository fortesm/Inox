<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# contrib/

Community contributions that live with Inox but are not part of the
compiler, the runtime or the standard library, and are not covered by its
tests or its release guarantees:

- editor and IDE support (syntax highlighting for VS Code, Vim, Emacs,
  Notepad++, Sublime Text), snippets and color schemes;
- shell completions for the `inox` driver;
- integrations with other tools (build systems, CI templates);
- community examples and experiments that are not official examples.

Each contribution goes in its own folder, with a README naming its
maintainer and how to use it:

- `vscode/`, `vim/`, `emacs/`, `sublime-text/`, `notepad-plus-plus/`,
  `jetbrains/` — editor support;
- `tree-sitter/` — one grammar for Neovim, Helix, Zed and GitHub;
- `shell-completions/` — completion of the `inox` driver for `bash/`,
  `zsh/`, `fish/` and `powershell/`;
- `ci-templates/` — `github-actions/` and `gitlab-ci/` jobs for Inox projects;
- `docker/` — a container image with Inox, clang and the library.

Code that the compiler or the runtime needs never goes here: it belongs in
`src/`.
