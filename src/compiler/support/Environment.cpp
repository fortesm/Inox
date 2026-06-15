// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Environment.h"

#include <cstdlib>
#include <memory>
#include <string_view>

namespace inox::compiler::support {

std::optional<std::string> getEnvironmentVariable(const char* name)
{
    if (name == nullptr || std::string_view(name).empty()) {
        return std::nullopt;
    }

#ifdef _WIN32
    char* value = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&value, &size, name) != 0 || value == nullptr || size == 0) {
        if (value != nullptr) {
            std::free(value);
        }
        return std::nullopt;
    }
    std::unique_ptr<char, decltype(&std::free)> owned(value, &std::free);
    if (std::string_view(owned.get()).empty()) {
        return std::nullopt;
    }
    return std::string(owned.get());
#else
    const char* value = std::getenv(name);
    if (value == nullptr || std::string_view(value).empty()) {
        return std::nullopt;
    }
    return std::string(value);
#endif
}

} // namespace inox::compiler::support
