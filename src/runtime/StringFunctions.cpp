// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Std.Strings and Std.StrUtils, and String concatenation (OpenSpec change
// std-strings). Positions count code points and start at 1; case mappings
// and White_Space follow the Unicode data of UnicodeTables.inc, the same in
// every language (no locale).
//
// Every function that makes a String returns it owned (+1) and reports a
// failure through `status`, as __inox_str_getln does: 0 ok, 1 a position or
// count out of range (the program stops with "string index out of range"),
// 2 out of memory. The library never prints: the program turns the status
// into its runtime fault (CANON-19).

#include "Decimal.h"
#include "NumberText.h"
#include "StringBlock.h"
#include "../compiler/exceptions/ExceptionTypes.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <string_view>

extern "C" [[noreturn]] void __inox_raise(std::uint64_t typeId);

namespace inox::runtime {

namespace {

struct CaseRun {
    std::uint32_t first;
    std::uint32_t last;
    std::int32_t delta;
    std::uint32_t stride;
};

struct CodeRange {
    std::uint32_t first;
    std::uint32_t last;
};

#include "UnicodeTables.inc"

enum Status : std::int32_t {
    kOk = 0,
    kOutOfRange = 1,
    kOutOfMemory = 2,
};

InoxString* asString(const void* value)
{
    return static_cast<InoxString*>(const_cast<void*>(value));
}

// An owned reference to an existing String (the result of a function that
// leaves its argument unchanged).
InoxString* retained(InoxString* string)
{
    if (!isImmortal(string)) {
        ++string->count;
    }
    return string;
}

std::uint32_t mapCodePoint(const CaseRun* runs, std::size_t count, std::uint32_t code)
{
    // The last run whose first is <= code (tools/gen_unicode_tables.py checks
    // that this reproduces the mapping for every code point).
    std::size_t low = 0;
    std::size_t high = count;
    while (low < high) {
        const std::size_t middle = (low + high) / 2;
        if (runs[middle].first <= code) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    if (low == 0) {
        return code;
    }
    const CaseRun& run = runs[low - 1];
    if (code > run.last || (code - run.first) % run.stride != 0) {
        return code;
    }
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(code) + run.delta);
}

// Whether code lies in one of the sorted ranges.
template <std::size_t Count>
bool inRanges(const CodeRange (&ranges)[Count], std::uint32_t code)
{
    const CodeRange* end = ranges + Count;
    const CodeRange* found =
        std::upper_bound(ranges, end, code, [](std::uint32_t value, const CodeRange& range) { return value < range.first; });
    return found != ranges && code <= (found - 1)->last;
}

bool isWhiteSpace(std::uint32_t code)
{
    return std::binary_search(std::begin(kWhiteSpace), std::end(kWhiteSpace), code);
}

// The code point starting at bytes[index] of well-formed UTF-8; index moves
// past it.
std::uint32_t decode(const char* bytes, std::int64_t& index)
{
    const auto lead = static_cast<unsigned char>(bytes[index]);
    if (lead < 0x80) {
        ++index;
        return lead;
    }
    const int extra = lead >= 0xF0 ? 3 : lead >= 0xE0 ? 2 : 1;
    std::uint32_t code = lead & (0x3F >> extra);
    for (int k = 1; k <= extra; ++k) {
        code = (code << 6) | (static_cast<unsigned char>(bytes[index + k]) & 0x3F);
    }
    index += extra + 1;
    return code;
}

int encodedSize(std::uint32_t code)
{
    return code < 0x80 ? 1 : code < 0x800 ? 2 : code < 0x10000 ? 3 : 4;
}

char* encode(std::uint32_t code, char* out)
{
    if (code < 0x80) {
        *out++ = static_cast<char>(code);
    } else if (code < 0x800) {
        *out++ = static_cast<char>(0xC0 | (code >> 6));
        *out++ = static_cast<char>(0x80 | (code & 0x3F));
    } else if (code < 0x10000) {
        *out++ = static_cast<char>(0xE0 | (code >> 12));
        *out++ = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        *out++ = static_cast<char>(0x80 | (code & 0x3F));
    } else {
        *out++ = static_cast<char>(0xF0 | (code >> 18));
        *out++ = static_cast<char>(0x80 | ((code >> 12) & 0x3F));
        *out++ = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        *out++ = static_cast<char>(0x80 | (code & 0x3F));
    }
    return out;
}

// The byte offset of the code point at `position` (0-based; the length in
// characters gives the end). O(1) for ASCII text.
std::int64_t byteOffset(InoxString* string, std::int64_t position)
{
    if (characterCount(string) == string->length) {
        return position;
    }
    std::int64_t index = 0;
    for (std::int64_t seen = 0; seen < position; ++seen) {
        decode(string->bytes, index);
    }
    return index;
}

// A new String of the given bytes; "" for none. Null when memory is exhausted.
InoxString* makeString(const char* bytes, std::int64_t length, std::int64_t characters)
{
    if (length == 0) {
        return emptyString();
    }
    InoxString* result = allocateString(length);
    if (result == nullptr) {
        return nullptr;
    }
    std::memcpy(result->bytes, bytes, static_cast<std::size_t>(length));
    headerOf(result)->characters = characters;
    return result;
}

void* fail(std::int32_t* status, Status why)
{
    *status = why;
    return nullptr;
}

void* succeed(std::int32_t* status, InoxString* result)
{
    if (result == nullptr) {
        return fail(status, kOutOfMemory);
    }
    *status = kOk;
    return result;
}

// The sum of the byte lengths of the parts, or -1 when it overflows.
std::int64_t totalLength(std::int64_t start, std::int64_t count, void* const* parts)
{
    std::int64_t total = start;
    for (std::int64_t index = 0; index < count; ++index) {
        const std::int64_t length = asString(parts[index])->length;
        if (length > std::numeric_limits<std::int64_t>::max() - total) {
            return -1;
        }
        total += length;
    }
    return total;
}

std::string_view view(const InoxString* string)
{
    return {string->bytes, static_cast<std::size_t>(string->length)};
}

} // namespace

} // namespace inox::runtime

using inox::runtime::InoxString;
using namespace inox::runtime;

extern "C" {

// `A + B + C`: one allocation, each part copied once (design section 1).
void* __inox_str_concat(std::int64_t count, void* const* parts, std::int32_t* status) noexcept
{
    const std::int64_t length = totalLength(0, count, parts);
    if (length < 0) {
        return fail(status, kOutOfMemory);
    }
    if (length == 0) {
        return succeed(status, emptyString());
    }
    InoxString* result = allocateString(length);
    if (result == nullptr) {
        return fail(status, kOutOfMemory);
    }
    char* out = result->bytes;
    std::int64_t characters = 0;
    for (std::int64_t index = 0; index < count; ++index) {
        InoxString* part = asString(parts[index]);
        std::memcpy(out, part->bytes, static_cast<std::size_t>(part->length));
        out += part->length;
        characters += characterCount(part);
    }
    headerOf(result)->characters = characters;
    return succeed(status, result);
}

// `S := S + X ...` and `S += X`: *slot is the owned String of S. Appended in
// place when S holds the only reference and the block has room; otherwise a
// new block with room to grow replaces it (capacity twice the new length),
// so N appends cost O(N) amortized. A part may be S itself.
void __inox_str_append(void** slot, std::int64_t count, void* const* parts, std::int32_t* status) noexcept
{
    InoxString* old = asString(*slot);
    const std::int64_t length = totalLength(old->length, count, parts);
    if (length < 0) {
        *status = kOutOfMemory;
        return;
    }
    std::int64_t characters = characterCount(old);
    for (std::int64_t index = 0; index < count; ++index) {
        characters += characterCount(asString(parts[index]));
    }
    if (!isImmortal(old) && old->count == 1 && headerOf(old)->capacity >= length) {
        // The parts are read before the length changes: a part that is S
        // itself still has its old length, and its bytes do not move.
        char* out = old->bytes + old->length;
        for (std::int64_t index = 0; index < count; ++index) {
            InoxString* part = asString(parts[index]);
            std::memmove(out, part->bytes, static_cast<std::size_t>(part->length));
            out += part->length;
        }
        old->length = length;
        old->bytes[length] = '\0';
        headerOf(old)->characters = characters;
        *status = kOk;
        return;
    }
    // Twice the new length, so the block has room after every growth.
    const std::int64_t capacity = length <= std::numeric_limits<std::int64_t>::max() / 2 ? length * 2 : length;
    InoxString* result = allocateString(length, capacity);
    if (result == nullptr) {
        *status = kOutOfMemory;
        return;
    }
    char* out = result->bytes;
    std::memcpy(out, old->bytes, static_cast<std::size_t>(old->length));
    out += old->length;
    for (std::int64_t index = 0; index < count; ++index) {
        InoxString* part = asString(parts[index]);
        std::memcpy(out, part->bytes, static_cast<std::size_t>(part->length));
        out += part->length;
    }
    headerOf(result)->characters = characters;
    // Released only after the copy: a part may be the old block.
    if (!isImmortal(old) && --old->count == 0) {
        freeString(old);
    }
    *slot = result;
    *status = kOk;
}

std::int64_t __inox_str_length(void* string) noexcept
{
    return characterCount(asString(string));
}

// Substr(S, Start, Count): 1 <= Start <= Length(S) + 1, Count >= 0 and
// Start + Count - 1 <= Length(S) (design section 3).
void* __inox_str_substr(void* value, std::int64_t start, std::int64_t count, std::int32_t* status) noexcept
{
    InoxString* string = asString(value);
    const std::int64_t characters = characterCount(string);
    if (start < 1 || start > characters + 1 || count < 0 || count > characters - start + 1) {
        return fail(status, kOutOfRange);
    }
    if (count == characters) {
        return succeed(status, retained(string));
    }
    const std::int64_t from = byteOffset(string, start - 1);
    std::int64_t to = from;
    if (characters == string->length) {
        to = from + count;
    } else {
        for (std::int64_t seen = 0; seen < count; ++seen) {
            decode(string->bytes, to);
        }
    }
    return succeed(status, makeString(string->bytes + from, to - from, count));
}

// Left(S, N) (side 1) and Right(S, N) (side 2): 0 <= N <= Length(S).
void* __inox_str_edge(void* value, std::int64_t count, std::int32_t side, std::int32_t* status) noexcept
{
    const std::int64_t characters = characterCount(asString(value));
    if (count < 0 || count > characters) {
        return fail(status, kOutOfRange);
    }
    return __inox_str_substr(value, side == 1 ? 1 : characters - count + 1, count, status);
}

// Pos(Sub, S): the position of the first Sub in S, 0 when absent or empty.
// UTF-8 is self-synchronizing, so a byte match is a match of characters.
std::int64_t __inox_str_pos(void* sub, void* value) noexcept
{
    InoxString* needle = asString(sub);
    InoxString* string = asString(value);
    if (needle->length == 0) {
        return 0;
    }
    const std::size_t found = view(string).find(view(needle));
    if (found == std::string_view::npos) {
        return 0;
    }
    return countCharacters(string->bytes, static_cast<std::int64_t>(found)) + 1;
}

// UpperCase (mode 0) and LowerCase (mode 1): one code point to one.
void* __inox_str_case(void* value, std::int32_t mode, std::int32_t* status) noexcept
{
    InoxString* string = asString(value);
    const CaseRun* runs = mode == 0 ? kUpperRuns : kLowerRuns;
    const std::size_t runCount = mode == 0 ? std::size(kUpperRuns) : std::size(kLowerRuns);
    std::int64_t length = 0;
    bool changed = false;
    for (std::int64_t index = 0; index < string->length;) {
        const std::uint32_t code = decode(string->bytes, index);
        const std::uint32_t mapped = mapCodePoint(runs, runCount, code);
        changed = changed || mapped != code;
        length += encodedSize(mapped);
    }
    if (!changed) {
        return succeed(status, retained(string));
    }
    InoxString* result = allocateString(length);
    if (result == nullptr) {
        return fail(status, kOutOfMemory);
    }
    char* out = result->bytes;
    for (std::int64_t index = 0; index < string->length;) {
        out = encode(mapCodePoint(runs, runCount, decode(string->bytes, index)), out);
    }
    headerOf(result)->characters = characterCount(string);
    return succeed(status, result);
}

// LTrim (sides 1), RTrim (2), Trim (3): without White_Space at those ends.
void* __inox_str_trim(void* value, std::int32_t sides, std::int32_t* status) noexcept
{
    InoxString* string = asString(value);
    std::int64_t from = 0;
    std::int64_t to = string->length;
    if ((sides & 1) != 0) {
        while (from < to) {
            std::int64_t next = from;
            if (!isWhiteSpace(decode(string->bytes, next))) {
                break;
            }
            from = next;
        }
    }
    if ((sides & 2) != 0) {
        while (to > from) {
            std::int64_t start = to - 1;
            while (start > from && (static_cast<unsigned char>(string->bytes[start]) & 0xC0) == 0x80) {
                --start;
            }
            std::int64_t next = start;
            if (!isWhiteSpace(decode(string->bytes, next))) {
                break;
            }
            to = start;
        }
    }
    if (from == 0 && to == string->length) {
        return succeed(status, retained(string));
    }
    return succeed(status, makeString(string->bytes + from, to - from,
                                      countCharacters(string->bytes + from, to - from)));
}

// PadLeft (side 1) and PadRight (side 2): spaces up to Width characters;
// a String already that long is returned as it is. Width >= 0.
void* __inox_str_pad(void* value, std::int64_t width, std::int32_t side, std::int32_t* status) noexcept
{
    InoxString* string = asString(value);
    if (width < 0) {
        return fail(status, kOutOfRange);
    }
    const std::int64_t characters = characterCount(string);
    if (characters >= width) {
        return succeed(status, retained(string));
    }
    const std::int64_t spaces = width - characters;
    if (spaces > std::numeric_limits<std::int64_t>::max() - string->length) {
        return fail(status, kOutOfMemory);
    }
    InoxString* result = allocateString(string->length + spaces);
    if (result == nullptr) {
        return fail(status, kOutOfMemory);
    }
    char* text = result->bytes + (side == 1 ? spaces : 0);
    char* padding = result->bytes + (side == 1 ? 0 : string->length);
    std::memcpy(text, string->bytes, static_cast<std::size_t>(string->length));
    std::memset(padding, ' ', static_cast<std::size_t>(spaces));
    headerOf(result)->characters = width;
    return succeed(status, result);
}

// Replicate(S, N): S repeated N times; N >= 0.
void* __inox_str_replicate(void* value, std::int64_t times, std::int32_t* status) noexcept
{
    InoxString* string = asString(value);
    if (times < 0) {
        return fail(status, kOutOfRange);
    }
    if (times == 0 || string->length == 0) {
        return succeed(status, emptyString());
    }
    if (times == 1) {
        return succeed(status, retained(string));
    }
    if (string->length > std::numeric_limits<std::int64_t>::max() / times) {
        return fail(status, kOutOfMemory);
    }
    InoxString* result = allocateString(string->length * times);
    if (result == nullptr) {
        return fail(status, kOutOfMemory);
    }
    for (std::int64_t index = 0; index < times; ++index) {
        std::memcpy(result->bytes + index * string->length, string->bytes, static_cast<std::size_t>(string->length));
    }
    headerOf(result)->characters = characterCount(string) * times;
    return succeed(status, result);
}

// Replace(S, Old, New): every Old, left to right, without overlaps; an empty
// Old changes nothing.
void* __inox_str_replace(void* value, void* oldValue, void* newValue, std::int32_t* status) noexcept
{
    InoxString* string = asString(value);
    InoxString* from = asString(oldValue);
    InoxString* to = asString(newValue);
    if (from->length == 0) {
        return succeed(status, retained(string));
    }
    const std::string_view text = view(string);
    const std::string_view pattern = view(from);
    std::int64_t matches = 0;
    for (std::size_t at = text.find(pattern); at != std::string_view::npos; at = text.find(pattern, at + pattern.size())) {
        ++matches;
    }
    if (matches == 0) {
        return succeed(status, retained(string));
    }
    const std::int64_t growth = to->length - from->length;
    if (growth > 0 && matches > (std::numeric_limits<std::int64_t>::max() - string->length) / growth) {
        return fail(status, kOutOfMemory);
    }
    const std::int64_t length = string->length + matches * growth;
    if (length == 0) {
        return succeed(status, emptyString());
    }
    InoxString* result = allocateString(length);
    if (result == nullptr) {
        return fail(status, kOutOfMemory);
    }
    char* out = result->bytes;
    std::size_t copied = 0;
    for (std::size_t at = text.find(pattern); at != std::string_view::npos; at = text.find(pattern, at + pattern.size())) {
        std::memcpy(out, text.data() + copied, at - copied);
        out += at - copied;
        std::memcpy(out, to->bytes, static_cast<std::size_t>(to->length));
        out += to->length;
        copied = at + pattern.size();
    }
    std::memcpy(out, text.data() + copied, text.size() - copied);
    return succeed(status, result);
}

// Contains (where 0), StartsWith (1), EndsWith (2); an empty Sub is found.
bool __inox_str_find(void* value, void* sub, std::int32_t where) noexcept
{
    const std::string_view text = view(asString(value));
    const std::string_view part = view(asString(sub));
    if (where == 1) {
        return text.substr(0, part.size()) == part;
    }
    if (where == 2) {
        return text.size() >= part.size() && text.substr(text.size() - part.size()) == part;
    }
    return text.find(part) != std::string_view::npos;
}

// CompareText(A, B): -1, 0 or 1, comparing code points after simple case
// folding (a prefix first); SameText is CompareText = 0.
std::int32_t __inox_str_compare_text(void* left, void* right) noexcept
{
    const InoxString* a = asString(left);
    const InoxString* b = asString(right);
    std::int64_t i = 0;
    std::int64_t j = 0;
    while (i < a->length && j < b->length) {
        const std::uint32_t x = mapCodePoint(kFoldRuns, std::size(kFoldRuns), decode(a->bytes, i));
        const std::uint32_t y = mapCodePoint(kFoldRuns, std::size(kFoldRuns), decode(b->bytes, j));
        if (x != y) {
            return x < y ? -1 : 1;
        }
    }
    if (i < a->length) {
        return 1;
    }
    return j < b->length ? -1 : 0;
}

// String(C) (change char-values): the one-character String of a Char, a
// Unicode scalar the compiler already checked.
void* __inox_str_from_char(std::int64_t code, std::int32_t* status) noexcept
{
    char bytes[4];
    const char* end = encode(static_cast<std::uint32_t>(code), bytes);
    return succeed(status, makeString(bytes, end - bytes, 1));
}

// CharAt(S, I) (change char-values): the code point at position I, counted
// from 1 in characters; outside 1..Length(S) the status is out of range.
std::int64_t __inox_str_char_at(void* value, std::int64_t position, std::int32_t* status) noexcept
{
    InoxString* string = asString(value);
    if (position < 1 || position > characterCount(string)) {
        *status = kOutOfRange;
        return 0;
    }
    *status = kOk;
    std::int64_t index = byteOffset(string, position - 1);
    return decode(string->bytes, index);
}

// r3.89 (C3): the class of a Char by its Unicode general category: 0
// IsLetter (L*), 1 IsDigit (Nd), 2 IsSpace (White_Space), 3 IsUpper (Lu),
// 4 IsLower (Ll), 5 IsPunctuation (P*).
bool __inox_char_is(std::int64_t code, std::int32_t which) noexcept
{
    const auto point = static_cast<std::uint32_t>(code);
    switch (which) {
    case 0:
        return inRanges(kLetterRanges, point);
    case 1:
        return inRanges(kDigitRanges, point);
    case 2:
        return isWhiteSpace(point);
    case 3:
        return inRanges(kUpperRanges, point);
    case 4:
        return inRanges(kLowerRanges, point);
    default:
        return inRanges(kPunctuationRanges, point);
    }
}

// UpperCase(C) (mode 0) and LowerCase(C) (mode 1): the simple mapping of one
// code point, as the String functions map each character.
std::int64_t __inox_char_case(std::int64_t code, std::int32_t mode) noexcept
{
    const auto point = static_cast<std::uint32_t>(code);
    return mode == 0 ? mapCodePoint(kUpperRuns, std::size(kUpperRuns), point)
                     : mapCodePoint(kLowerRuns, std::size(kLowerRuns), point);
}

// ---- Numbers and text (change numbers-text, r3.90) ----

// B1: String(N), String(U), String(F), String(G) of a Float32.
void* __inox_str_from_i64(std::int64_t number, std::int32_t* status) noexcept
{
    char text[32];
    const int length = std::snprintf(text, sizeof text, "%lld", static_cast<long long>(number));
    return succeed(status, makeString(text, length, length));
}

void* __inox_str_from_u64(std::uint64_t number, std::int32_t* status) noexcept
{
    char text[32];
    const int length = std::snprintf(text, sizeof text, "%llu", static_cast<unsigned long long>(number));
    return succeed(status, makeString(text, length, length));
}

void* __inox_str_from_f64(double number, std::int32_t* status) noexcept
{
    char text[kRealTextCapacity];
    const auto length = static_cast<std::int64_t>(formatReal(number, false, text));
    return succeed(status, makeString(text, length, length));
}

void* __inox_str_from_f32(float number, std::int32_t* status) noexcept
{
    char text[kRealTextCapacity];
    const auto length = static_cast<std::int64_t>(formatReal(static_cast<double>(number), true, text));
    return succeed(status, makeString(text, length, length));
}

// FormatFloat(F, Decimals): Decimals in 0..20, else status 1.
void* __inox_str_format_float(double number, std::int64_t decimals, std::int32_t* status) noexcept
{
    if (decimals < 0 || decimals > 20) {
        return fail(status, kOutOfRange);
    }
    char text[400];
    const auto length = static_cast<std::int64_t>(formatFixed(number, static_cast<int>(decimals), text, sizeof text));
    return succeed(status, makeString(text, length, length));
}

} // extern "C"

namespace {

[[noreturn]] void raiseConversionError()
{
    const auto* info = inox::compiler::exceptions::findExceptionType("ConversionError");
    __inox_raise(info != nullptr ? info->typeId : 0x100);
}

} // namespace

extern "C" {

// B2: T(S) of a String into an integer type whose values are low..high, a
// UInt64, a Float or a Float32. Text that is not a number of the type raises
// ConversionError (Std.Errors), which a handler can catch.
std::int64_t __inox_str_to_i64(void* value, std::int64_t low, std::int64_t high)
{
    const InoxString* string = asString(value);
    std::int64_t result = 0;
    if (!parseIntegerText(string->bytes, static_cast<std::size_t>(string->length), low, high, result)) {
        raiseConversionError();
    }
    return result;
}

std::uint64_t __inox_str_to_u64(void* value)
{
    const InoxString* string = asString(value);
    std::uint64_t result = 0;
    if (!parseUnsignedText(string->bytes, static_cast<std::size_t>(string->length), result)) {
        raiseConversionError();
    }
    return result;
}

double __inox_str_to_f64(void* value)
{
    const InoxString* string = asString(value);
    double result = 0;
    if (!parseRealText(string->bytes, static_cast<std::size_t>(string->length), false, result)) {
        raiseConversionError();
    }
    return result;
}

float __inox_str_to_f32(void* value)
{
    const InoxString* string = asString(value);
    double result = 0;
    if (!parseRealText(string->bytes, static_cast<std::size_t>(string->length), true, result)) {
        raiseConversionError();
    }
    return static_cast<float>(result);
}

// r3.91: String(C) of a Currency or a Crypto, and Currency(S), Crypto(S)
// (raising ConversionError).
void* __inox_str_from_dec(std::uint64_t low, std::int64_t high, std::int32_t places, std::int32_t* status) noexcept
{
    char text[kDecimalTextCapacity];
    const auto length = static_cast<std::int64_t>(formatDecimal(decimalOf(low, high), places, text));
    return succeed(status, makeString(text, length, length));
}

void __inox_str_to_dec(void* value, std::int32_t places, void* out)
{
    const InoxString* string = asString(value);
    Decimal result;
    if (parseDecimal(string->bytes, static_cast<std::size_t>(string->length), places, result) != 0) {
        raiseConversionError();
    }
    std::memcpy(out, &result.low, 8);
    std::memcpy(static_cast<char*>(out) + 8, &result.high, 8);
}

// B4: FieldCount(S, Sep) is the number of fields of S between the
// separators (one more than the separators; one for "" or an empty Sep);
// Field(S, Sep, I) is the I-th, counted from 1 (outside, status 1).
std::int64_t __inox_str_field_count(void* value, void* separator) noexcept
{
    const std::string_view text = view(asString(value));
    const std::string_view sep = view(asString(separator));
    if (sep.empty()) {
        return 1;
    }
    std::int64_t count = 1;
    for (std::size_t at = text.find(sep); at != std::string_view::npos; at = text.find(sep, at + sep.size())) {
        ++count;
    }
    return count;
}

void* __inox_str_field(void* value, void* separator, std::int64_t index, std::int32_t* status) noexcept
{
    const std::string_view text = view(asString(value));
    const std::string_view sep = view(asString(separator));
    if (index < 1) {
        return fail(status, kOutOfRange);
    }
    std::size_t start = 0;
    for (std::int64_t field = 1; field < index; ++field) {
        const std::size_t at = sep.empty() ? std::string_view::npos : text.find(sep, start);
        if (at == std::string_view::npos) {
            return fail(status, kOutOfRange);
        }
        start = at + sep.size();
    }
    const std::size_t end = sep.empty() ? std::string_view::npos : text.find(sep, start);
    const std::string_view field = text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);
    if (field.size() == text.size()) {
        return succeed(status, retained(asString(value)));
    }
    return succeed(status, makeString(field.data(), static_cast<std::int64_t>(field.size()), -1));
}

} // extern "C"
