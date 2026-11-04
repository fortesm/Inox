// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "SourceCursor.h"

namespace inox::compiler::scanner {

SourceCursor::SourceCursor(std::string_view source) : source_(source) {}

bool SourceCursor::atEnd() const
{
    return current_ >= source_.size();
}

char SourceCursor::peek() const
{
    return atEnd() ? '\0' : source_[current_];
}

char SourceCursor::peekNext() const
{
    return current_ + 1 >= source_.size() ? '\0' : source_[current_ + 1];
}

char SourceCursor::advance()
{
    const char ch = source_[current_++];
    if (ch == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return ch;
}

bool SourceCursor::match(char expected)
{
    if (atEnd() || source_[current_] != expected) {
        return false;
    }
    advance();
    return true;
}

SourceCursor::Position SourceCursor::position() const
{
    return Position{current_, line_, column_};
}

std::size_t SourceCursor::offset() const
{
    return current_;
}

char SourceCursor::at(std::size_t byteOffset) const
{
    return source_[byteOffset];
}

std::string_view SourceCursor::slice(std::size_t beginOffset, std::size_t endOffset) const
{
    return source_.substr(beginOffset, endOffset - beginOffset);
}

} // namespace inox::compiler::scanner
