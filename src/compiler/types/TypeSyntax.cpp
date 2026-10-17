// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "TypeSyntax.h"

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

bool isNameCharacter(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '.';
}

// The generic's name when `text` is `Name<...>`, else empty.
std::string_view genericName(std::string_view text)
{
    if (text.size() < 3 || text.back() != '>') {
        return {};
    }
    const std::size_t open = text.find('<');
    if (open == 0 || open == std::string_view::npos) {
        return {};
    }
    for (std::size_t index = 0; index < open; ++index) {
        if (!isNameCharacter(text[index])) {
            return {};
        }
    }
    return text.substr(0, open);
}

// Splits `A, B<C, D>, E` at the commas outside angle and square brackets;
// empty when the brackets do not balance or a part is empty.
std::vector<std::string_view> splitArguments(std::string_view text)
{
    std::vector<std::string_view> parts;
    int depth = 0;
    std::size_t start = 0;
    for (std::size_t index = 0; index <= text.size(); ++index) {
        const char ch = index < text.size() ? text[index] : ',';
        if (ch == '<' || ch == '[') {
            ++depth;
        } else if (ch == '>' || ch == ']') {
            if (--depth < 0) {
                return {};
            }
        } else if (ch == ',' && depth == 0) {
            std::string_view part = text.substr(start, index - start);
            while (!part.empty() && part.front() == ' ') {
                part.remove_prefix(1);
            }
            while (!part.empty() && part.back() == ' ') {
                part.remove_suffix(1);
            }
            if (part.empty()) {
                return {};
            }
            parts.push_back(part);
            start = index + 1;
        }
    }
    return depth == 0 ? parts : std::vector<std::string_view>{};
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
        syntax.arguments.push_back(std::make_shared<const TypeSyntax>(std::move(*element)));
        return syntax;
    }
    if (const std::string_view name = genericName(text); !name.empty()) {
        TypeSyntax syntax;
        syntax.form = TypeSyntax::Form::Generic;
        syntax.name = std::string(name);
        const auto parts = splitArguments(text.substr(name.size() + 1, text.size() - name.size() - 2));
        if (parts.empty()) {
            return std::nullopt;
        }
        for (const std::string_view part : parts) {
            auto argument = readTypeSyntax(part);
            if (!argument) {
                return std::nullopt;
            }
            syntax.arguments.push_back(std::make_shared<const TypeSyntax>(std::move(*argument)));
        }
        return syntax;
    }
    for (const char ch : text) {
        if (!isNameCharacter(ch)) {
            return std::nullopt;
        }
    }
    TypeSyntax syntax;
    syntax.name = std::string(text);
    return syntax;
}

namespace {

TypeSyntax substitute(const TypeSyntax& syntax, const std::function<std::string(std::string_view)>& replace)
{
    if (syntax.form == TypeSyntax::Form::Named) {
        const std::string replacement = replace(syntax.name);
        if (replacement.empty()) {
            return syntax;
        }
        if (auto read = readTypeSyntax(replacement)) {
            return std::move(*read);
        }
        TypeSyntax named;
        named.name = replacement;
        return named;
    }
    TypeSyntax copy = syntax;
    for (auto& argument : copy.arguments) {
        argument = std::make_shared<const TypeSyntax>(substitute(*argument, replace));
    }
    return copy;
}

} // namespace

std::string substituteTypeNames(std::string_view written,
                                const std::function<std::string(std::string_view)>& replace)
{
    const auto syntax = readTypeSyntax(written);
    if (!syntax) {
        return std::string(written);
    }
    return substitute(*syntax, replace).spelling();
}

std::string TypeSyntax::spelling() const
{
    switch (form) {
    case Form::Named:
        return name;
    case Form::Array:
        return spellArrayType(low, high, argument().spelling());
    case Form::Generic: {
        std::string text = name + "<";
        for (std::size_t index = 0; index < arguments.size(); ++index) {
            text += (index == 0 ? "" : ", ") + arguments[index]->spelling();
        }
        return text + ">";
    }
    }
    return name;
}

} // namespace inox::compiler::types
