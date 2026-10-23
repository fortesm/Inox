// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// A literal read exactly as a Currency (6 decimal places) or a Crypto (18),
// change decimals (CANON-8, canon r3.91): its value scaled by 10^places, as
// decimal digits, so the analyzer (exactness, range) and the code generator
// (the IR constant, i64 or i128) read it the same way and no binary real is
// ever involved.

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace inox::compiler::support {

enum class DecimalLiteralStatus {
    Ok,
    TooManyPlaces,  // 19.9999999 as a Currency
    OutOfRange,     // beyond the type
    Malformed,
};

struct DecimalLiteral {
    DecimalLiteralStatus status = DecimalLiteralStatus::Malformed;
    std::string scaled;  // the magnitude times 10^places, decimal digits, no sign
};

// The largest magnitude of the type, scaled: Int64.Max for Currency,
// Int128.Max for Crypto.
inline std::string_view decimalMaximum(unsigned places)
{
    return places == 6 ? std::string_view("9223372036854775807")
                       : std::string_view("170141183460469231731687303715884105727");
}

// Whether the digits a and b (no leading zeros) satisfy a <= b.
inline bool digitsAtMost(std::string_view a, std::string_view b)
{
    return a.size() != b.size() ? a.size() < b.size() : a <= b;
}

// An integer or real literal (`19`, `19.99`, `1_000.5`, `1.5e3`) read exactly
// with `places` decimal places.
inline DecimalLiteral parseDecimalLiteral(std::string_view text, unsigned places)
{
    DecimalLiteral result;
    std::string whole;
    std::string fraction;
    long exponent = 0;
    std::size_t index = 0;
    const auto digit = [&](char ch) { return ch >= '0' && ch <= '9'; };
    for (; index < text.size() && (digit(text[index]) || text[index] == '_'); ++index) {
        if (text[index] != '_') {
            whole.push_back(text[index]);
        }
    }
    if (index < text.size() && text[index] == '.') {
        for (++index; index < text.size() && (digit(text[index]) || text[index] == '_'); ++index) {
            if (text[index] != '_') {
                fraction.push_back(text[index]);
            }
        }
    }
    if (index < text.size() && (text[index] == 'e' || text[index] == 'E')) {
        ++index;
        bool negative = false;
        if (index < text.size() && (text[index] == '+' || text[index] == '-')) {
            negative = text[index] == '-';
            ++index;
        }
        for (; index < text.size() && (digit(text[index]) || text[index] == '_'); ++index) {
            if (text[index] != '_' && exponent < 100000) {
                exponent = exponent * 10 + (text[index] - '0');
            }
        }
        if (negative) {
            exponent = -exponent;
        }
    }
    if (whole.empty() || index != text.size()) {
        return result;
    }
    // The value is (whole.fraction) x 10^exponent: move the point.
    std::string digits = whole + fraction;
    long point = static_cast<long>(whole.size()) + exponent;  // digits before the point
    while (point < 0) {
        digits.insert(digits.begin(), '0');
        ++point;
    }
    while (static_cast<long>(digits.size()) < point) {
        digits.push_back('0');
    }
    std::string integerPart = digits.substr(0, static_cast<std::size_t>(point));
    std::string fractionPart = digits.substr(static_cast<std::size_t>(point));
    while (!fractionPart.empty() && fractionPart.back() == '0') {
        fractionPart.pop_back();
    }
    if (fractionPart.size() > places) {
        result.status = DecimalLiteralStatus::TooManyPlaces;
        return result;
    }
    fractionPart.append(places - fractionPart.size(), '0');
    std::string scaled = integerPart + fractionPart;
    const std::size_t first = scaled.find_first_not_of('0');
    scaled = first == std::string::npos ? std::string("0") : scaled.substr(first);
    if (!digitsAtMost(scaled, decimalMaximum(places))) {
        result.status = DecimalLiteralStatus::OutOfRange;
        return result;
    }
    result.status = DecimalLiteralStatus::Ok;
    result.scaled = scaled;
    return result;
}

} // namespace inox::compiler::support
