// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <string_view>

namespace inox::compiler::support {

enum class OperatingSystem {
    Windows,
    Linux,
    MacOS,
    FreeBSD,
    NetBSD,
    OpenBSD,
    DragonFlyBSD,
    Illumos,
    Solaris,
    AIX,
    HPUX,
    Android,
    UnixWare,
    Unknown
};

OperatingSystem hostOperatingSystem();
std::string_view operatingSystemName(OperatingSystem os);
std::string_view nullDevicePath();
std::string_view executableSuffix();

} // namespace inox::compiler::support
