// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cstddef>
#include <string_view>

namespace inox::compiler::scanner {

// Byte-oriented cursor. It does not recognize tokens or decode Unicode.
// Source storage must outlive the cursor, just as for the former Lexer view.
class SourceCursor {
public:
    struct Position {
        std::size_t offset;
        std::size_t line;
        std::size_t column;
    };

    explicit SourceCursor(std::string_view source);

    bool atEnd() const;
    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);

    Position position() const;
    std::size_t offset() const;
    char at(std::size_t byteOffset) const;
    std::string_view slice(std::size_t beginOffset, std::size_t endOffset) const;

private:
    std::string_view source_;
    std::size_t current_ = 0;
    std::size_t line_ = 1;
    std::size_t column_ = 1;
};

} // namespace inox::compiler::scanner
