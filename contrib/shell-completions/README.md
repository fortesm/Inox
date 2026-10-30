<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# contrib/shell-completions/

Command-line completion for the `inox` driver: its commands (`run`, `build`,
`check`, `emit-llvm`, `interface`, `help`, `version`), its switches
(`--debug`, `--path <dir>`) and `.inox` files. One folder per shell:
`bash/`, `zsh/`, `fish/` and `powershell/`. Keep them in step with
`inox help` (CANON-23).
