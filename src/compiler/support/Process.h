// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace inox::compiler::support {

// Run a program by ARGUMENT VECTOR, without going through a shell.
//
// args[0] is the program to execute (resolved through PATH on POSIX, and via the
// standard search on Windows). The remaining entries are passed as separate,
// literal arguments. Because no shell interprets the command line, spaces and
// shell metacharacters inside any argument (e.g. "C:\Program Files\...") are
// treated literally and cannot be split or injected.
//
// Returns the child's exit code (0 == success), or -1 if the process could not
// be started. If captureToNull is true, the child's stdout/stderr are discarded
// (used for probes like `clang --version`); otherwise they are inherited.
int runProcess(const std::vector<std::string>& args, bool captureToNull = false);

// True if `command` can be executed (probed via `command --version`), with all
// output discarded. No shell is involved.
bool commandExists(std::string_view command);

} // namespace inox::compiler::support
