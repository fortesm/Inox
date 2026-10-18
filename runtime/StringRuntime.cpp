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

#include "StringBlock.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace inox::runtime {

namespace {

InoxString kEmpty = {kImmortalCount, 0, {'\0'}};

} // namespace

InoxString* emptyString() noexcept
{
    return &kEmpty;
}

InoxString* allocateString(std::int64_t length, std::int64_t capacity) noexcept
{
    if (capacity < length) {
        capacity = length;
    }
    const std::size_t size =
        sizeof(StringHeader) + offsetof(InoxString, bytes) + static_cast<std::size_t>(capacity) + 1;
    auto* header = static_cast<StringHeader*>(std::malloc(size));
    if (header == nullptr) {
        return nullptr;
    }
    header->capacity = capacity;
    header->characters = -1;
    auto* block = reinterpret_cast<InoxString*>(header + 1);
    block->count = 1;
    block->length = length;
    block->bytes[length] = '\0';
    return block;
}

void freeString(InoxString* string) noexcept
{
    std::free(headerOf(string));
}

std::int64_t countCharacters(const char* bytes, std::int64_t length) noexcept
{
    // Every byte that is not a continuation byte (10xxxxxx) starts a code point.
    std::int64_t characters = 0;
    for (std::int64_t index = 0; index < length; ++index) {
        characters += (static_cast<unsigned char>(bytes[index]) & 0xC0) != 0x80 ? 1 : 0;
    }
    return characters;
}

std::int64_t characterCount(InoxString* string) noexcept
{
    if (isImmortal(string)) {
        return countCharacters(string->bytes, string->length);
    }
    StringHeader* header = headerOf(string);
    if (header->characters < 0) {
        header->characters = countCharacters(string->bytes, string->length);
    }
    return header->characters;
}

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

InoxString* readLine(int (*readByte)(void* context), void* context, LineStatus& status, bool* atEnd) noexcept
{
    std::int64_t length = 0;
    std::int64_t capacity = 64;
    auto* buffer = static_cast<unsigned char*>(std::malloc(static_cast<std::size_t>(capacity)));
    if (atEnd != nullptr) {
        *atEnd = false;
    }
    if (buffer == nullptr) {
        status = LineStatus::OutOfMemory;
        return &kEmpty;
    }
    for (;;) {
        const int ch = readByte(context);
        if (ch < 0) {
            if (atEnd != nullptr) {
                *atEnd = true;
            }
            break;
        }
        if (ch == '\n') {
            break;
        }
        if (length == capacity) {
            capacity *= 2;
            auto* grown = static_cast<unsigned char*>(std::realloc(buffer, static_cast<std::size_t>(capacity)));
            if (grown == nullptr) {
                std::free(buffer);
                status = LineStatus::OutOfMemory;
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
        status = LineStatus::InvalidUtf8;
        return &kEmpty;
    }
    if (length == 0) {
        std::free(buffer);
        status = LineStatus::Ok;
        return &kEmpty;
    }
    InoxString* string = allocateString(length);
    if (string == nullptr) {
        std::free(buffer);
        status = LineStatus::OutOfMemory;
        return &kEmpty;
    }
    std::memcpy(string->bytes, buffer, static_cast<std::size_t>(length));
    std::free(buffer);
    status = LineStatus::Ok;
    return string;
}

} // namespace inox::runtime

namespace {

using inox::runtime::InoxString;
using inox::runtime::kImmortalCount;

int readThroughProgram(void* context)
{
    // context points at the program's getchar (a function pointer is never
    // converted to void* itself).
    const auto readByte = *static_cast<int (**)()>(context);
    const int ch = readByte();
    return ch == EOF ? -1 : ch;
}

} // namespace

extern "C" {

void* __inox_str_empty() noexcept
{
    return inox::runtime::emptyString();
}

void __inox_str_retain(void* value) noexcept
{
    auto* string = static_cast<InoxString*>(value);
    if (string != nullptr && string->count != kImmortalCount) {
        ++string->count;
    }
}

void __inox_str_release(void* value) noexcept
{
    auto* string = static_cast<InoxString*>(value);
    if (string == nullptr || string->count == kImmortalCount) {
        return;
    }
    if (--string->count == 0) {
        inox::runtime::freeString(string);
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

// Reads one line through the program's getchar (inox::runtime::readLine).
// Status 0 ok, 1 invalid UTF-8, 2 out of memory; the program turns 1 and 2
// into runtime faults.
void* __inox_str_getln(int (*readByte)(), std::int32_t* status) noexcept
{
    inox::runtime::LineStatus lineStatus = inox::runtime::LineStatus::Ok;
    InoxString* line = inox::runtime::readLine(readThroughProgram, &readByte, lineStatus);
    *status = static_cast<std::int32_t>(lineStatus);
    return line;
}

} // extern "C"
