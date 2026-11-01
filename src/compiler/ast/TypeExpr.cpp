// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "TypeExpr.h"

#include <cctype>
#include <cstddef>
#include <stdexcept>

namespace inox::compiler::ast {

namespace {

bool isIdentifier(std::string_view text)
{
    if (text.empty() || !(std::isalpha(static_cast<unsigned char>(text.front())) || text.front() == '_')) {
        return false;
    }
    for (const char ch : text) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')) {
            return false;
        }
    }
    return true;
}

// One identifier, or identifiers joined by single dots.
bool isQualifiedName(std::string_view text)
{
    std::size_t start = 0;
    while (true) {
        const std::size_t dot = text.find('.', start);
        if (!isIdentifier(text.substr(start, dot == std::string_view::npos ? std::string_view::npos : dot - start))) {
            return false;
        }
        if (dot == std::string_view::npos) {
            return true;
        }
        start = dot + 1;
    }
}

bool isBound(const ArrayBound& bound)
{
    if (bound.isName) {
        return isIdentifier(bound.text);
    }
    std::string_view digits = bound.text;
    if (!digits.empty() && digits.front() == '-') {
        digits.remove_prefix(1);
    }
    if (!digits.empty() && digits.front() == '$') {
        digits.remove_prefix(1);
        for (const char ch : digits) {
            if (!std::isxdigit(static_cast<unsigned char>(ch))) {
                return false;
            }
        }
        return !digits.empty();
    }
    for (const char ch : digits) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return false;
        }
    }
    return !digits.empty();
}

void spellInto(const TypeExpr& type, std::string& text)
{
    switch (type.form) {
    case TypeExpr::Form::Named:
        text += type.name;
        return;
    case TypeExpr::Form::Array:
        text += "Array[";
        text += type.low.text;
        text += "..";
        text += type.high.text;
        text += "] ";
        if (!type.arguments.empty()) {  // a broken tree is still spelled, for its message
            spellInto(type.element(), text);
        }
        return;
    case TypeExpr::Form::Generic:
        text += type.name;
        text += '<';
        for (std::size_t index = 0; index < type.arguments.size(); ++index) {
            if (index != 0) {
                text += ", ";
            }
            spellInto(type.arguments[index], text);
        }
        text += '>';
        return;
    }
}

} // namespace

bool TypeExpr::isGeneric(std::string_view generic) const
{
    if (form != Form::Generic || name.size() != generic.size()) {
        return false;
    }
    for (std::size_t index = 0; index < name.size(); ++index) {
        if (std::tolower(static_cast<unsigned char>(name[index])) !=
            std::tolower(static_cast<unsigned char>(generic[index]))) {
            return false;
        }
    }
    return true;
}

TypeExpr namedType(std::string name, SourcePosition position)
{
    TypeExpr type;
    type.name = std::move(name);
    type.position = position;
    return type;
}

std::string spell(const TypeExpr& type)
{
    std::string text;
    spellInto(type, text);
    return text;
}

void checkTypeExpr(const TypeExpr& type)
{
    const auto broken = [&](const char* rule) {
        throw std::logic_error(std::string("TypeExpr invariant broken (") + rule + "): " + spell(type));
    };
    switch (type.form) {
    case TypeExpr::Form::Named:
        if (!isQualifiedName(type.name) || !type.low.text.empty() || !type.high.text.empty() ||
            !type.arguments.empty()) {
            broken("1, Named");
        }
        return;
    case TypeExpr::Form::Array:
        if (!type.name.empty() || !isBound(type.low) || !isBound(type.high) || type.arguments.size() != 1) {
            broken("2, Array");
        }
        break;
    case TypeExpr::Form::Generic:
        if (!isQualifiedName(type.name) || type.arguments.empty() || !type.low.text.empty() ||
            !type.high.text.empty()) {
            broken("3, Generic");
        }
        break;
    }
    for (const TypeExpr& argument : type.arguments) {
        checkTypeExpr(argument);
    }
}

} // namespace inox::compiler::ast
