// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// The one place that names the Inox version (canon SECTION 02, VERSIONING).
// The language and its compiler share one version; the canon revision is the
// number of the decision log in docs/INOX_CANONICAL.md, not a version.
// scripts/run-tests.sh checks that both match the canon header.

#pragma once

#include <string_view>

namespace inox::compiler::support {

inline constexpr std::string_view kInoxVersion = "0.2.0-pre-alpha";
inline constexpr std::string_view kCanonRevision = "r3.58";

} // namespace inox::compiler::support
