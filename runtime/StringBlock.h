// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// Internal to libinoxrt: the String block layout and the line reader shared
// by the String runtime (GetLn) and the file runtime (ReadLn). Generated code
// never includes this; it sees a String as an opaque pointer whose length is
// at offset 8 and whose bytes start at offset 16.

#pragma once

#include <cstddef>
#include <cstdint>

namespace inox::runtime {

struct InoxString final {
    std::int64_t count;   // -1: immortal (literals, "")
    std::int64_t length;  // in bytes
    char bytes[1];        // length bytes, then NUL
};

// The shared immortal "".
InoxString* emptyString() noexcept;

// A new block (count 1) for `length` bytes, NUL terminated; null when memory
// is exhausted.
InoxString* allocateString(std::int64_t length) noexcept;

// RFC 3629: well-formed UTF-8 (no overlong forms, no surrogates, at most
// U+10FFFF).
bool isValidUtf8(const unsigned char* text, std::int64_t length) noexcept;

enum class LineStatus : std::int32_t {
    Ok = 0,
    InvalidUtf8 = 1,
    OutOfMemory = 2,
};

// Reads one line through readByte (returns a byte, or a negative value at end
// of input): the bytes up to the newline, without it, and without a CR just
// before it. At end of input with nothing read the line is "". The result is
// owned (+1) unless it is "" (immortal); on a failure it is "" and status says
// why. `atEnd` (optional) is set when the input ended before any newline.
InoxString* readLine(int (*readByte)(void* context), void* context, LineStatus& status,
                     bool* atEnd = nullptr) noexcept;

} // namespace inox::runtime
