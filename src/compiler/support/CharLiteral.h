// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// A Char literal (CANON-14, change char-values): one Unicode scalar value
// between single quotes, written in UTF-8 or as an escape (`'\n'`, `'\r'`,
// `'\t'`, `'\0'`, `'\\'`, `'\''`). Shared by the analyzer, which reports a
// bad literal, and the code generator, which emits its code point.

#pragma once

#include <cstdint>
#include <string_view>

namespace inox::compiler::support {

enum class CharLiteralStatus {
    Ok,
    Empty,          // ''
    NotOneChar,     // 'ab', or bytes that are not UTF-8
    UnknownEscape,  // '\q'
    Surrogate,      // U+D800..U+DFFF
};

struct DecodedChar {
    CharLiteralStatus status = CharLiteralStatus::NotOneChar;
    std::int64_t code = 0;
};

// Decodes `text`, with or without its quotes.
inline DecodedChar decodeCharLiteral(std::string_view text) noexcept
{
    if (text.size() >= 2 && text.front() == '\'' && text.back() == '\'') {
        text = text.substr(1, text.size() - 2);
    }
    if (text.empty()) {
        return {CharLiteralStatus::Empty, 0};
    }
    if (text.front() == '\\') {
        if (text.size() != 2) {
            return {text.size() == 1 ? CharLiteralStatus::UnknownEscape : CharLiteralStatus::NotOneChar, 0};
        }
        switch (text[1]) {
        case 'n':
            return {CharLiteralStatus::Ok, '\n'};
        case 'r':
            return {CharLiteralStatus::Ok, '\r'};
        case 't':
            return {CharLiteralStatus::Ok, '\t'};
        case '0':
            return {CharLiteralStatus::Ok, 0};
        case '\\':
            return {CharLiteralStatus::Ok, '\\'};
        case '\'':
            return {CharLiteralStatus::Ok, '\''};
        default:
            return {CharLiteralStatus::UnknownEscape, 0};
        }
    }
    const auto byte = [&](std::size_t i) { return static_cast<unsigned char>(text[i]); };
    std::size_t length = 0;
    std::int64_t code = 0;
    if (byte(0) < 0x80) {
        length = 1;
        code = byte(0);
    } else if ((byte(0) & 0xE0) == 0xC0) {
        length = 2;
        code = byte(0) & 0x1F;
    } else if ((byte(0) & 0xF0) == 0xE0) {
        length = 3;
        code = byte(0) & 0x0F;
    } else if ((byte(0) & 0xF8) == 0xF0) {
        length = 4;
        code = byte(0) & 0x07;
    } else {
        return {CharLiteralStatus::NotOneChar, 0};
    }
    if (text.size() != length) {
        return {CharLiteralStatus::NotOneChar, 0};
    }
    for (std::size_t i = 1; i < length; ++i) {
        if ((byte(i) & 0xC0) != 0x80) {
            return {CharLiteralStatus::NotOneChar, 0};
        }
        code = (code << 6) | (byte(i) & 0x3F);
    }
    if (code >= 0xD800 && code <= 0xDFFF) {
        return {CharLiteralStatus::Surrogate, code};
    }
    if (code > 0x10FFFF) {
        return {CharLiteralStatus::NotOneChar, 0};
    }
    return {CharLiteralStatus::Ok, code};
}

} // namespace inox::compiler::support
