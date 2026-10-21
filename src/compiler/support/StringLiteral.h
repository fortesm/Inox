// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The bytes a String literal stands for (change case-strings): the escapes
// \n \r \t \\ \" \0 are decoded, any other backslash is kept, exactly as the
// code generator writes the literal (codegen/LlvmText.cpp). Two literals are
// the same String when their bytes are equal.

#pragma once

#include <string>
#include <string_view>

namespace inox::compiler::support {

inline std::string decodeStringLiteral(std::string_view text)
{
    std::string bytes;
    bytes.reserve(text.size());
    for (std::size_t index = 0; index < text.size(); ++index) {
        char byte = text[index];
        if (byte == '\\' && index + 1 < text.size()) {
            const char escape = text[++index];
            switch (escape) {
            case 'n':
                byte = '\n';
                break;
            case 'r':
                byte = '\r';
                break;
            case 't':
                byte = '\t';
                break;
            case '\\':
                byte = '\\';
                break;
            case '"':
                byte = '"';
                break;
            case '0':
                byte = '\0';
                break;
            default:
                bytes.push_back('\\');
                byte = escape;
                break;
            }
        }
        bytes.push_back(byte);
    }
    return bytes;
}

} // namespace inox::compiler::support
