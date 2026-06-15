// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Process.h"
#include "Platform.h"

#include <cstdlib>
#include <string>

namespace inox::compiler::support {

bool commandExists(std::string_view command)
{
    if (command.empty()) {
        return false;
    }
    const std::string checkCommand =
        std::string(command) + " --version > " + std::string(nullDevicePath()) + " 2>&1";
    return runShellCommand(checkCommand) == 0;
}

int runShellCommand(const std::string& command)
{
    return std::system(command.c_str());
}

} // namespace inox::compiler::support
