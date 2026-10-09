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

// How a generated Inox program writes raw bytes to the standard error stream
// (file descriptor 2). Runtime traps (CANON-19) print their diagnostic through
// this C library function so that the emitted LLVM IR stays self-contained: it
// links against the C library alone, without any Inox runtime library. The
// FILE* object `stderr` is deliberately not used: its symbol differs between C
// libraries (glibc/musl `stderr`, macOS/FreeBSD `__stderrp`, NetBSD/OpenBSD
// `__sF`, UCRT `__acrt_iob_func`). `write`/`_write` on file descriptor 2 does not.
struct NativeErrorWriter {
    std::string_view symbol;   // int-fd, const void* buffer, length -> result
    unsigned lengthBits;       // width of the length parameter
    unsigned resultBits;       // width of the result
};

NativeErrorWriter nativeErrorWriter();

} // namespace inox::compiler::support
