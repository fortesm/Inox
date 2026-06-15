// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "FileSystem.h"

#include <string_view>

namespace inox::compiler::support {

std::filesystem::path executableDirectory(const char* argv0)
{
    namespace fs = std::filesystem;
    if (argv0 == nullptr || std::string_view(argv0).empty()) {
        return {};
    }

    fs::path executablePath(argv0);
    std::error_code error;
    if (executablePath.is_relative()) {
        executablePath = fs::absolute(executablePath, error);
        if (error) {
            return {};
        }
    }

    return executablePath.parent_path();
}

} // namespace inox::compiler::support
