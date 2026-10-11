// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ArrayType.h"

#include <cctype>

namespace inox::compiler::support {

namespace {

bool startsWithArray(std::string_view name)
{
    constexpr std::string_view prefix = "array[";
    if (name.size() < prefix.size()) {
        return false;
    }
    for (std::size_t index = 0; index < prefix.size(); ++index) {
        if (std::tolower(static_cast<unsigned char>(name[index])) != prefix[index]) {
            return false;
        }
    }
    return true;
}

} // namespace

bool isArrayTypeName(std::string_view name)
{
    return startsWithArray(name);
}

std::optional<ArrayTypeName> splitArrayTypeName(std::string_view name)
{
    if (!startsWithArray(name)) {
        return std::nullopt;
    }
    const std::size_t close = name.find(']');
    const std::size_t range = name.find("..");
    if (close == std::string_view::npos || range == std::string_view::npos || range > close ||
        close + 2 > name.size() || name[close + 1] != ' ') {
        return std::nullopt;
    }
    ArrayTypeName parts;
    parts.low = std::string(name.substr(6, range - 6));
    parts.high = std::string(name.substr(range + 2, close - range - 2));
    parts.element = std::string(name.substr(close + 2));
    if (parts.low.empty() || parts.high.empty() || parts.element.empty()) {
        return std::nullopt;
    }
    return parts;
}

std::string makeArrayTypeName(std::string_view low, std::string_view high, std::string_view element)
{
    std::string name = "Array[";
    name += low;
    name += "..";
    name += high;
    name += "] ";
    name += element;
    return name;
}

} // namespace inox::compiler::support
