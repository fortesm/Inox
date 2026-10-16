// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "PointerType.h"

#include "ArrayType.h"

#include <cctype>
#include <cstddef>

namespace inox::compiler::support {

namespace {

// `Prefix<Inner>` with a case-insensitive prefix (written lower case here).
bool hasGenericPrefix(std::string_view name, std::string_view prefix)
{
    if (name.size() < prefix.size() + 2 || name.back() != '>') {
        return false;
    }
    for (std::size_t index = 0; index < prefix.size(); ++index) {
        if (std::tolower(static_cast<unsigned char>(name[index])) != prefix[index]) {
            return false;
        }
    }
    return name[prefix.size()] == '<';
}

std::string genericInner(std::string_view name, std::string_view prefix)
{
    return std::string(name.substr(prefix.size() + 1, name.size() - prefix.size() - 2));
}

} // namespace

bool isOwnTypeName(std::string_view name)
{
    return hasGenericPrefix(name, "own");
}

bool isLinkTypeName(std::string_view name)
{
    return hasGenericPrefix(name, "link");
}

bool isOptionTypeName(std::string_view name)
{
    return hasGenericPrefix(name, "option");
}

bool isOptionOwnTypeName(std::string_view name)
{
    return isOptionTypeName(name) && isOwnTypeName(optionInnerTypeName(name));
}

bool isPointerTypeName(std::string_view name)
{
    return isOwnTypeName(name) || isLinkTypeName(name) || isOptionOwnTypeName(name);
}

std::string optionInnerTypeName(std::string_view name)
{
    return isOptionTypeName(name) ? genericInner(name, "option") : std::string{};
}

std::string pointerTargetTypeName(std::string_view name)
{
    if (isOwnTypeName(name)) {
        return genericInner(name, "own");
    }
    if (isLinkTypeName(name)) {
        return genericInner(name, "link");
    }
    if (isOptionOwnTypeName(name)) {
        return pointerTargetTypeName(optionInnerTypeName(name));
    }
    return {};
}

std::string makeOwnTypeName(std::string_view target)
{
    return "Own<" + std::string(target) + ">";
}

std::string makeOptionOwnTypeName(std::string_view target)
{
    return "Option<" + makeOwnTypeName(target) + ">";
}

std::string makeLinkTypeName(std::string_view target)
{
    return "Link<" + std::string(target) + ">";
}

bool isGenericTypeName(std::string_view name)
{
    return isSliceTypeName(name) || isOwnTypeName(name) || isLinkTypeName(name) || isOptionTypeName(name);
}

} // namespace inox::compiler::support
