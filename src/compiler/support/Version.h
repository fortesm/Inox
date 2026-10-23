// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// The one place that names the Inox version (constitution VERSIONING):
// MAJOR.MINOR.RELEASE.BUILD plus the stage. The language and its compiler
// share one version. MAJOR, MINOR, RELEASE and the stage change only by the
// maintainer's decision; BUILD is generated at every build
// (cmake/InoxBuildNumber.cmake). The canon revision numbers the recorded
// decisions (openspec/specs/constitution/revisions.md), not a version.
// scripts/run-tests.sh checks all of it against the top of revisions.md.

#pragma once

#include "InoxBuildNumber.h"

#include <string>
#include <string_view>

namespace inox::compiler::support {

inline constexpr unsigned kVersionMajor = 0;    // 0..99
inline constexpr unsigned kVersionMinor = 2;    // 0..999
inline constexpr unsigned kVersionRelease = 0;  // 0..999
// kBuildNumber (0..9999) comes from InoxBuildNumber.h.
inline constexpr std::string_view kVersionStage = "pre-alpha";
inline constexpr std::string_view kCanonRevision = "r3.98";

// "0.2.0.206"
inline std::string versionText()
{
    return std::to_string(kVersionMajor) + "." + std::to_string(kVersionMinor) + "." +
           std::to_string(kVersionRelease) + "." + std::to_string(kBuildNumber);
}

} // namespace inox::compiler::support
