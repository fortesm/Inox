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

inline std::string realLiteralDigits(std::string_view text)
{
    std::string digits;
    digits.reserve(text.size());
    for (const char ch : text) {
        if (ch != '_') {
            digits.push_back(ch);
        }
    }
    return digits;
}

inline double parseRealLiteral(std::string_view text)
{
    return std::strtod(realLiteralDigits(text).c_str(), nullptr);
}

// r3.86: a Float32 literal is read from its decimal text straight into a
// float, rounded once (never through a double, which could round twice).
// The result is the double holding that float exactly.
inline double parseRealLiteral32(std::string_view text)
{
    return static_cast<double>(std::strtof(realLiteralDigits(text).c_str(), nullptr));
}

inline double parseRealLiteral(std::string_view text, bool float32)
{
    return float32 ? parseRealLiteral32(text) : parseRealLiteral(text);
}

} // namespace inox::compiler::support
