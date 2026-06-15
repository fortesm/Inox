// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <string>
#include <string_view>

namespace inox::compiler::support {

bool commandExists(std::string_view command);
int runShellCommand(const std::string& command);

} // namespace inox::compiler::support
