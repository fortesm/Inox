// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "NumberText.h"

#include <charconv>
#include <system_error>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace inox::runtime {

namespace {

std::size_t copyText(const char* text, char* out) noexcept
{
    const std::size_t length = std::strlen(text);
    std::memcpy(out, text, length + 1);
    return length;
}

bool isDigit(char ch) noexcept
{
    return ch >= '0' && ch <= '9';
}

// The real grammar of input and of conversions from text: an optional '-',
// digits, an optional fraction, an optional exponent; nothing else.
bool isRealText(const char* bytes, std::size_t length) noexcept
{
    std::size_t index = 0;
    if (index < length && bytes[index] == '-') {
        ++index;
    }
    const auto digits = [&]() {
        const std::size_t start = index;
        while (index < length && isDigit(bytes[index])) {
            ++index;
        }
        return index > start;
    };
    if (!digits()) {
        return false;
    }
    if (index < length && bytes[index] == '.') {
        ++index;
        if (!digits()) {
            return false;
        }
    }
    if (index < length && (bytes[index] == 'e' || bytes[index] == 'E')) {
        ++index;
        if (index < length && (bytes[index] == '+' || bytes[index] == '-')) {
            ++index;
        }
        if (!digits()) {
            return false;
        }
    }
    return index == length;
}

} // namespace

std::size_t formatReal(double value, bool float32, char* out) noexcept
{
    if (std::isnan(value)) {
        return copyText("NaN", out);
    }
    if (std::isinf(value)) {
        return copyText(value < 0 ? "-Inf" : "Inf", out);
    }
    // The shortest round-trip digits, in scientific form: "d.ddde+XX".
    char scientific[kRealTextCapacity];
    const std::to_chars_result result =
        float32 ? std::to_chars(scientific, scientific + sizeof scientific, static_cast<float>(value),
                                std::chars_format::scientific)
                : std::to_chars(scientific, scientific + sizeof scientific, value, std::chars_format::scientific);
    *result.ptr = '\0';
    const char* cursor = scientific;
    std::size_t length = 0;
    if (*cursor == '-') {
        out[length++] = '-';
        ++cursor;
    }
    char digits[kRealTextCapacity];
    std::size_t count = 0;
    while (*cursor != '\0' && *cursor != 'e') {
        if (*cursor != '.') {
            digits[count++] = *cursor;
        }
        ++cursor;
    }
    const int exponent = *cursor == 'e' ? std::atoi(cursor + 1) : 0;
    if (exponent >= -4 && exponent < 16) {
        if (exponent < 0) {
            out[length++] = '0';
            out[length++] = '.';
            for (int zero = 0; zero < -exponent - 1; ++zero) {
                out[length++] = '0';
            }
            for (std::size_t index = 0; index < count; ++index) {
                out[length++] = digits[index];
            }
        } else {
            const std::size_t whole = static_cast<std::size_t>(exponent) + 1;
            for (std::size_t index = 0; index < whole; ++index) {
                out[length++] = index < count ? digits[index] : '0';
            }
            out[length++] = '.';
            if (count > whole) {
                for (std::size_t index = whole; index < count; ++index) {
                    out[length++] = digits[index];
                }
            } else {
                out[length++] = '0';
            }
        }
    } else {
        out[length++] = digits[0];
        out[length++] = '.';
        if (count > 1) {
            for (std::size_t index = 1; index < count; ++index) {
                out[length++] = digits[index];
            }
        } else {
            out[length++] = '0';
        }
        length += static_cast<std::size_t>(std::snprintf(out + length, 8, "e%d", exponent));
    }
    out[length] = '\0';
    return length;
}

std::size_t formatFixed(double value, int decimals, char* out, std::size_t capacity) noexcept
{
    if (std::isnan(value) || std::isinf(value)) {
        return formatReal(value, false, out);
    }
    // std::to_chars rounds the exact binary value to nearest, ties to even,
    // the same on every C++ library (snprintf differs between C runtimes).
    const std::to_chars_result result = std::to_chars(out, out + capacity - 1, value, std::chars_format::fixed, decimals);
    if (result.ec != std::errc()) {
        out[0] = '\0';
        return 0;
    }
    *result.ptr = '\0';
    return static_cast<std::size_t>(result.ptr - out);
}

bool parseIntegerText(const char* bytes, std::size_t length, std::int64_t low, std::int64_t high,
                      std::int64_t& value) noexcept
{
    std::size_t index = 0;
    const bool negative = length > 0 && bytes[0] == '-';
    if (negative) {
        ++index;
    }
    if (index == length) {
        return false;
    }
    // Accumulated as a negative number, which reaches Int64.Min.
    std::int64_t total = 0;
    for (; index < length; ++index) {
        if (!isDigit(bytes[index])) {
            return false;
        }
        const int digit = bytes[index] - '0';
        if (total < (std::numeric_limits<std::int64_t>::min() + digit) / 10) {
            return false;
        }
        total = total * 10 - digit;
    }
    if (!negative) {
        if (total == std::numeric_limits<std::int64_t>::min()) {
            return false;
        }
        total = -total;
    }
    if (total < low || total > high) {
        return false;
    }
    value = total;
    return true;
}

bool parseUnsignedText(const char* bytes, std::size_t length, std::uint64_t& value) noexcept
{
    if (length == 0) {
        return false;
    }
    std::uint64_t total = 0;
    for (std::size_t index = 0; index < length; ++index) {
        if (!isDigit(bytes[index])) {
            return false;
        }
        const auto digit = static_cast<std::uint64_t>(bytes[index] - '0');
        if (total > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) {
            return false;
        }
        total = total * 10 + digit;
    }
    value = total;
    return true;
}

bool parseRealText(const char* bytes, std::size_t length, bool float32, double& value) noexcept
{
    if (!isRealText(bytes, length)) {
        return false;
    }
    // strtod needs a NUL; long texts are copied to the heap.
    char local[256];
    char* text = length < sizeof local ? local : static_cast<char*>(std::malloc(length + 1));
    if (text == nullptr) {
        return false;
    }
    std::memcpy(text, bytes, length);
    text[length] = '\0';
    const double parsed = float32 ? static_cast<double>(std::strtof(text, nullptr)) : std::strtod(text, nullptr);
    if (text != local) {
        std::free(text);
    }
    if (std::isinf(parsed)) {
        return false;
    }
    value = parsed;
    return true;
}

} // namespace inox::runtime
