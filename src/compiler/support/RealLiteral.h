// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// A real literal (`1.5`, `1_000.25`, change float-conversions): its value,
// read once the same way by the analyzer (constant folding, Float32 range)
// and the code generator (the IR constant).

#pragma once

#include <cstdlib>
#include <string>
#include <string_view>

namespace inox::compiler::support {

inline double parseRealLiteral(std::string_view text)
{
    std::string digits;
    digits.reserve(text.size());
    for (const char ch : text) {
        if (ch != '_') {
            digits.push_back(ch);
        }
    }
    return std::strtod(digits.c_str(), nullptr);
}

} // namespace inox::compiler::support
