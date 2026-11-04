// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "UnitTest.h"

#include "compiler/scanner/SourceCursor.h"

namespace inox::compiler::unit {

int runSourceCursorTests()
{
    using scanner::SourceCursor;

    SourceCursor cursor{"A\r\nB=="};
    check(!cursor.atEnd() && cursor.peek() == 'A' && cursor.peekNext() == '\r',
          "source cursor initial lookahead");
    check(cursor.position().offset == 0 && cursor.position().line == 1 && cursor.position().column == 1,
          "source cursor starts at line one column one");
    check(cursor.advance() == 'A' && cursor.match('\r') && cursor.advance() == '\n',
          "source cursor advances over CRLF");
    check(cursor.position().offset == 3 && cursor.position().line == 2 && cursor.position().column == 1,
          "source cursor advances line only on LF");
    check(cursor.at(3) == 'B' && cursor.slice(3, 6) == "B==",
          "source cursor provides byte spans without token classification");
    check(!cursor.match('X') && cursor.offset() == 3,
          "failed match does not consume input");
    check(cursor.advance() == 'B' && cursor.match('=') && cursor.match('=') && cursor.atEnd(),
          "source cursor matches and reaches EOF");
    check(cursor.peek() == '\0' && cursor.peekNext() == '\0' && !cursor.match('='),
          "source cursor lookahead at EOF");
    check(cursor.position().offset == 6 && cursor.position().line == 2 && cursor.position().column == 4,
          "source cursor records final source position");

    SourceCursor empty{""};
    check(empty.atEnd() && empty.peek() == '\0' && empty.peekNext() == '\0' && !empty.match('a'),
          "source cursor handles empty source");
    return 0;
}

} // namespace inox::compiler::unit
