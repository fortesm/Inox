<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# libs/_template/

The layout every vendored library follows; copy this folder as
`libs/<name>/` and fill it in. Inox vendors no library today.

    libs/<name>/
      README.md    what it is, why Inox needs it, and why it cannot be a
                   system dependency
      VERSION      the exact upstream version or commit
      LICENSE      the library's license, unchanged
      UPSTREAM.md  where it comes from, how to update it, and every local
                   change (ideally none)
      src/         the library's sources, as published upstream
      include/     its headers, if separate

Also add the license to `licenses/` and say in `licenses/README.md` which
packages ship it.
