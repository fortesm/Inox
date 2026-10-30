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

// What generated code sees: the String pointer points at `count`, and the
// count, the length and the bytes are at offsets 0, 8 and 16.
struct InoxString final {
    std::int64_t count;   // -1: immortal (literals, "")
    std::int64_t length;  // in bytes
    char bytes[1];        // length bytes, then NUL
};

// A block made by this library has a hidden header before the count (change
// std-strings, design section 2). Immortal blocks have none.
//
//   [ capacity ][ characters ][ count ][ length ][ bytes ... NUL ]
//                             ^ the String pointer
struct StringHeader final {
    std::int64_t capacity;    // bytes available for the text (in-place append)
    std::int64_t characters;  // code points, or -1 until counted
};

inline constexpr std::int64_t kImmortalCount = -1;

inline bool isImmortal(const InoxString* string) noexcept
{
    return string->count == kImmortalCount;
}

// The header of a block made by this library (never of an immortal one).
inline StringHeader* headerOf(InoxString* string) noexcept
{
    return reinterpret_cast<StringHeader*>(reinterpret_cast<char*>(string) - sizeof(StringHeader));
}

// The shared immortal "".
InoxString* emptyString() noexcept;

// A new block (count 1) for `length` bytes, NUL terminated, with room for
// `capacity` bytes (at least `length`); null when memory is exhausted. Its
// characters are counted when first asked for.
InoxString* allocateString(std::int64_t length, std::int64_t capacity = 0) noexcept;

// Frees a block made by allocateString.
void freeString(InoxString* string) noexcept;

// The number of code points: cached in the header of a block of this
// library, counted every time for an immortal block (short literals).
std::int64_t characterCount(InoxString* string) noexcept;

// The number of code points of well-formed UTF-8 bytes.
std::int64_t countCharacters(const char* bytes, std::int64_t length) noexcept;

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
