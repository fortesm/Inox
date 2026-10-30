<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# contrib/ci-templates/github-actions/

A workflow (`inox.yml`) and, later, a reusable action (`setup-inox`) that
downloads the latest release package, puts `inox` on PATH and runs
`inox check` and `inox run` on a project.
