// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Platform.h"

namespace inox::compiler::support {

OperatingSystem hostOperatingSystem()
{
#if defined(_WIN32)
    return OperatingSystem::Windows;
#elif defined(__ANDROID__)
    return OperatingSystem::Android;
#elif defined(__linux__)
    return OperatingSystem::Linux;
#elif defined(__APPLE__) && defined(__MACH__)
    return OperatingSystem::MacOS;
#elif defined(__FreeBSD__)
    return OperatingSystem::FreeBSD;
#elif defined(__NetBSD__)
    return OperatingSystem::NetBSD;
#elif defined(__OpenBSD__)
    return OperatingSystem::OpenBSD;
#elif defined(__DragonFly__)
    return OperatingSystem::DragonFlyBSD;
#elif defined(__illumos__)
    return OperatingSystem::Illumos;
#elif defined(__sun)
    return OperatingSystem::Solaris;
#elif defined(_AIX)
    return OperatingSystem::AIX;
#elif defined(__hpux)
    return OperatingSystem::HPUX;
#elif defined(__USLC__)
    return OperatingSystem::UnixWare;
#else
    return OperatingSystem::Unknown;
#endif
}

std::string_view operatingSystemName(OperatingSystem os)
{
    switch (os) {
    case OperatingSystem::Windows:
        return "windows";
    case OperatingSystem::Linux:
        return "linux";
    case OperatingSystem::MacOS:
        return "macos";
    case OperatingSystem::FreeBSD:
        return "freebsd";
    case OperatingSystem::NetBSD:
        return "netbsd";
    case OperatingSystem::OpenBSD:
        return "openbsd";
    case OperatingSystem::DragonFlyBSD:
        return "dragonflybsd";
    case OperatingSystem::Illumos:
        return "illumos";
    case OperatingSystem::Solaris:
        return "solaris";
    case OperatingSystem::AIX:
        return "aix";
    case OperatingSystem::HPUX:
        return "hpux";
    case OperatingSystem::Android:
        return "android";
    case OperatingSystem::UnixWare:
        return "unixware";
    case OperatingSystem::Unknown:
        return "unknown";
    }
    return "unknown";
}

std::string_view nullDevicePath()
{
#if defined(_WIN32)
    return "NUL";
#else
    return "/dev/null";
#endif
}

std::string_view executableSuffix()
{
#if defined(_WIN32)
    return ".exe";
#else
    return "";
#endif
}

} // namespace inox::compiler::support
