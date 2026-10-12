// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// String runtime for Inox (CANON-14, OpenSpec change string-runtime).
//
// A String value is a pointer to an immutable block:
//     { int64 count, int64 length, char bytes[length + 1] }   (NUL terminated)
// count -1 marks an immortal block (literals and ""), which retain and
// release leave alone. Counts are not atomic: Inox 0.1 has no threads
// (maintainer decision, 2026-10-12).
//
// This file never touches the console: on Windows the program and this
// library may use different C runtimes, so all console output stays in the
// program (printf) and line input reads through the program's getchar, passed
// in as a function pointer. Memory is allocated and freed only here.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

struct InoxString final {
    std::int64_t count;
    std::int64_t length;
    char bytes[1];
};

constexpr std::int64_t kImmortal = -1;

InoxString kEmpty = {kImmortal, 0, {'\0'}};

// Status codes of __inox_str_getln; the program maps them to runtime faults.
constexpr std::int32_t kStatusOk = 0;
constexpr std::int32_t kStatusInvalidUtf8 = 1;
constexpr std::int32_t kStatusOutOfMemory = 2;

InoxString* allocate(std::int64_t length) noexcept
{
    const std::size_t size = offsetof(InoxString, bytes) + static_cast<std::size_t>(length) + 1;
    auto* block = static_cast<InoxString*>(std::malloc(size));
    if (block == nullptr) {
        return nullptr;
    }
    block->count = 1;
    block->length = length;
    block->bytes[length] = '\0';
    return block;
}

// RFC 3629: well-formed UTF-8 (no overlong forms, no surrogates, at most
// U+10FFFF).
bool isValidUtf8(const unsigned char* text, std::int64_t length) noexcept
{
    std::int64_t index = 0;
    while (index < length) {
        const unsigned char lead = text[index];
        if (lead < 0x80) {
            ++index;
            continue;
        }
        int extra = 0;
        unsigned char low = 0x80;
        unsigned char high = 0xBF;
        if (lead >= 0xC2 && lead <= 0xDF) {
            extra = 1;
        } else if (lead >= 0xE0 && lead <= 0xEF) {
            extra = 2;
            if (lead == 0xE0) low = 0xA0;
            if (lead == 0xED) high = 0x9F;
        } else if (lead >= 0xF0 && lead <= 0xF4) {
            extra = 3;
            if (lead == 0xF0) low = 0x90;
            if (lead == 0xF4) high = 0x8F;
        } else {
            return false;
        }
        if (index + extra >= length) {
            return false;
        }
        for (int k = 1; k <= extra; ++k) {
            const unsigned char next = text[index + k];
            const unsigned char min = k == 1 ? low : 0x80;
            const unsigned char max = k == 1 ? high : 0xBF;
            if (next < min || next > max) {
                return false;
            }
        }
        index += extra + 1;
    }
    return true;
}

} // namespace

extern "C" {

void* __inox_str_empty() noexcept
{
    return &kEmpty;
}

void __inox_str_retain(void* value) noexcept
{
    auto* string = static_cast<InoxString*>(value);
    if (string != nullptr && string->count != kImmortal) {
        ++string->count;
    }
}

void __inox_str_release(void* value) noexcept
{
    auto* string = static_cast<InoxString*>(value);
    if (string == nullptr || string->count == kImmortal) {
        return;
    }
    if (--string->count == 0) {
        std::free(string);
    }
}

bool __inox_str_equal(const void* left, const void* right) noexcept
{
    const auto* a = static_cast<const InoxString*>(left);
    const auto* b = static_cast<const InoxString*>(right);
    if (a == b) {
        return true;
    }
    return a->length == b->length &&
           std::memcmp(a->bytes, b->bytes, static_cast<std::size_t>(a->length)) == 0;
}

// CANON-14 (v3.56): Strings are ordered byte by byte, the shorter first when
// one is a prefix of the other. On UTF-8 this is Unicode code point order
// ("Z" < "a" < "á"), not a locale collation. Returns -1, 0 or 1.
std::int32_t __inox_str_compare(const void* left, const void* right) noexcept
{
    const auto* a = static_cast<const InoxString*>(left);
    const auto* b = static_cast<const InoxString*>(right);
    if (a == b) {
        return 0;
    }
    const std::int64_t common = a->length < b->length ? a->length : b->length;
    const int order = std::memcmp(a->bytes, b->bytes, static_cast<std::size_t>(common));
    if (order != 0) {
        return order < 0 ? -1 : 1;
    }
    if (a->length == b->length) {
        return 0;
    }
    return a->length < b->length ? -1 : 1;
}

// Reads one line through the program's getchar: the bytes up to the newline,
// without it (a `\r` before it is dropped too). At end of input with nothing
// read, the line is "". The result is owned (+1) unless it is "".
void* __inox_str_getln(int (*readByte)(), std::int32_t* status) noexcept
{
    std::int64_t length = 0;
    std::int64_t capacity = 64;
    auto* buffer = static_cast<unsigned char*>(std::malloc(static_cast<std::size_t>(capacity)));
    if (buffer == nullptr) {
        *status = kStatusOutOfMemory;
        return &kEmpty;
    }
    for (;;) {
        const int ch = readByte();
        if (ch == EOF || ch == '\n') {
            break;
        }
        if (length == capacity) {
            capacity *= 2;
            auto* grown = static_cast<unsigned char*>(std::realloc(buffer, static_cast<std::size_t>(capacity)));
            if (grown == nullptr) {
                std::free(buffer);
                *status = kStatusOutOfMemory;
                return &kEmpty;
            }
            buffer = grown;
        }
        buffer[length++] = static_cast<unsigned char>(ch);
    }
    if (length > 0 && buffer[length - 1] == '\r') {
        --length;
    }
    if (!isValidUtf8(buffer, length)) {
        std::free(buffer);
        *status = kStatusInvalidUtf8;
        return &kEmpty;
    }
    if (length == 0) {
        std::free(buffer);
        *status = kStatusOk;
        return &kEmpty;
    }
    InoxString* string = allocate(length);
    if (string == nullptr) {
        std::free(buffer);
        *status = kStatusOutOfMemory;
        return &kEmpty;
    }
    std::memcpy(string->bytes, buffer, static_cast<std::size_t>(length));
    std::free(buffer);
    *status = kStatusOk;
    return string;
}

} // extern "C"
