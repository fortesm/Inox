// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "TypeSyntax.h"

#include <array>
#include <cctype>
#include <cstddef>

namespace inox::compiler::types {

namespace {

bool startsWithIgnoreCase(std::string_view text, std::string_view prefix)
{
    if (text.size() < prefix.size()) {
        return false;
    }
    for (std::size_t index = 0; index < prefix.size(); ++index) {
        if (std::tolower(static_cast<unsigned char>(text[index])) !=
            std::tolower(static_cast<unsigned char>(prefix[index]))) {
            return false;
        }
    }
    return true;
}

constexpr std::array<std::string_view, 4> kGenericNames = {"Slice", "Own", "Link", "Option"};

// The generic's name when `text` is `Name<...>` with a known Name.
std::string_view genericName(std::string_view text)
{
    if (text.empty() || text.back() != '>') {
        return {};
    }
    for (std::string_view name : kGenericNames) {
        if (text.size() >= name.size() + 3 && startsWithIgnoreCase(text, name) && text[name.size()] == '<') {
            return text.substr(0, name.size());
        }
    }
    return {};
}

} // namespace

bool isArraySpelling(std::string_view text)
{
    return startsWithIgnoreCase(text, "array[");
}

bool isGenericSpelling(std::string_view text)
{
    return !genericName(text).empty();
}

bool isGenericSpelling(std::string_view text, std::string_view name)
{
    const std::string_view found = genericName(text);
    return !found.empty() && found.size() == name.size() && startsWithIgnoreCase(found, name);
}

std::string spellArrayType(std::string_view low, std::string_view high, std::string_view element)
{
    std::string text = "Array[";
    text += low;
    text += "..";
    text += high;
    text += "] ";
    text += element;
    return text;
}

std::optional<TypeSyntax> readTypeSyntax(std::string_view text)
{
    if (text.empty()) {
        return std::nullopt;
    }
    if (isArraySpelling(text)) {
        // `Array[Low..High] Element`, the element after one space.
        const std::size_t close = text.find(']');
        const std::size_t range = text.find("..");
        if (close == std::string_view::npos || range == std::string_view::npos || range > close ||
            close + 2 > text.size() || text[close + 1] != ' ') {
            return std::nullopt;
        }
        TypeSyntax syntax;
        syntax.form = TypeSyntax::Form::Array;
        syntax.low = std::string(text.substr(6, range - 6));
        syntax.high = std::string(text.substr(range + 2, close - range - 2));
        const std::string_view elementText = text.substr(close + 2);
        if (syntax.low.empty() || syntax.high.empty() || elementText.empty()) {
            return std::nullopt;
        }
        auto element = readTypeSyntax(elementText);
        if (!element) {
            return std::nullopt;
        }
        syntax.argument = std::make_shared<const TypeSyntax>(std::move(*element));
        return syntax;
    }
    if (const std::string_view name = genericName(text); !name.empty()) {
        TypeSyntax syntax;
        syntax.form = TypeSyntax::Form::Generic;
        syntax.name = std::string(name);
        auto argument = readTypeSyntax(text.substr(name.size() + 1, text.size() - name.size() - 2));
        if (!argument) {
            return std::nullopt;
        }
        syntax.argument = std::make_shared<const TypeSyntax>(std::move(*argument));
        return syntax;
    }
    TypeSyntax syntax;
    syntax.name = std::string(text);
    return syntax;
}

std::string TypeSyntax::spelling() const
{
    switch (form) {
    case Form::Named:
        return name;
    case Form::Array:
        return spellArrayType(low, high, argument->spelling());
    case Form::Generic:
        return name + "<" + argument->spelling() + ">";
    }
    return name;
}

} // namespace inox::compiler::types
