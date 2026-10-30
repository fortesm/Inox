<!--
SPDX-License-Identifier: MPL-2.0
Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
-->

# utils/inox-lint/

The linter: warnings about style and likely mistakes that the language
allows (names that differ only in case from a declaration, routines too
long, a `case` whose `else` hides a missing literal, public declarations
without a contract). Planned; it reuses the compiler's analysis, and
reports what the compiler accepts, never what it rejects.
