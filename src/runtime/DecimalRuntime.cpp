// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Exact decimals (change decimals, canon r3.91, decisions A1-A5 of Marcelo
// Fortes, 2026-10-21): the operations generated code cannot do inline.
// Products and quotients are exact before one rounding to the type's places,
// a half going to the even neighbour (A1, banker's rounding). Numbers are
// unsigned limbs of 32 bits; there is no 128-bit division anywhere.

#include "Decimal.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace inox::runtime {

namespace {

// An unsigned number of 320 bits, little-endian limbs: room for a 128-bit
// magnitude times 10^18 times another 128-bit one, and more.
constexpr std::size_t kLimbs = 10;
using Big = std::array<std::uint32_t, kLimbs>;

Big bigOf(std::uint64_t low, std::uint64_t high = 0) noexcept
{
    Big result{};
    result[0] = static_cast<std::uint32_t>(low);
    result[1] = static_cast<std::uint32_t>(low >> 32);
    result[2] = static_cast<std::uint32_t>(high);
    result[3] = static_cast<std::uint32_t>(high >> 32);
    return result;
}

bool isZero(const Big& value) noexcept
{
    for (const std::uint32_t limb : value) {
        if (limb != 0) {
            return false;
        }
    }
    return true;
}

int compare(const Big& a, const Big& b) noexcept
{
    for (std::size_t index = kLimbs; index-- > 0;) {
        if (a[index] != b[index]) {
            return a[index] < b[index] ? -1 : 1;
        }
    }
    return 0;
}

void subtract(Big& a, const Big& b) noexcept
{
    std::int64_t borrow = 0;
    for (std::size_t index = 0; index < kLimbs; ++index) {
        std::int64_t difference = static_cast<std::int64_t>(a[index]) - b[index] - borrow;
        borrow = difference < 0 ? 1 : 0;
        if (difference < 0) {
            difference += static_cast<std::int64_t>(1) << 32;
        }
        a[index] = static_cast<std::uint32_t>(difference);
    }
}

void addOne(Big& a) noexcept
{
    for (std::uint32_t& limb : a) {
        if (++limb != 0) {
            return;
        }
    }
}

// a * b; false when the product does not fit kLimbs limbs.
bool multiply(const Big& a, const Big& b, Big& product) noexcept
{
    std::array<std::uint64_t, 2 * kLimbs> wide{};
    for (std::size_t i = 0; i < kLimbs; ++i) {
        if (a[i] == 0) {
            continue;
        }
        std::uint64_t carry = 0;
        for (std::size_t j = 0; j < kLimbs; ++j) {
            const std::uint64_t sum = wide[i + j] + static_cast<std::uint64_t>(a[i]) * b[j] + carry;
            wide[i + j] = sum & 0xFFFFFFFFu;
            carry = sum >> 32;
        }
        for (std::size_t k = i + kLimbs; carry != 0 && k < wide.size(); ++k) {
            const std::uint64_t sum = wide[k] + carry;
            wide[k] = sum & 0xFFFFFFFFu;
            carry = sum >> 32;
        }
    }
    for (std::size_t index = kLimbs; index < wide.size(); ++index) {
        if (wide[index] != 0) {
            return false;
        }
    }
    for (std::size_t index = 0; index < kLimbs; ++index) {
        product[index] = static_cast<std::uint32_t>(wide[index]);
    }
    return true;
}

Big powerOfTen(int exponent) noexcept
{
    Big result = bigOf(1);
    const Big ten = bigOf(10);
    for (int step = 0; step < exponent; ++step) {
        multiply(result, ten, result);
    }
    return result;
}

bool bit(const Big& value, std::size_t index) noexcept
{
    return ((value[index / 32] >> (index % 32)) & 1u) != 0;
}

void shiftLeftOne(Big& value) noexcept
{
    std::uint32_t carry = 0;
    for (std::uint32_t& limb : value) {
        const std::uint32_t next = limb >> 31;
        limb = (limb << 1) | carry;
        carry = next;
    }
}

// The quotient of a / divisor rounded to the nearest, a half to even
// (`truncate`: toward zero instead). divisor is not zero.
Big divideRounded(const Big& a, const Big& divisor, bool truncate) noexcept
{
    Big quotient{};
    Big remainder{};
    for (std::size_t index = kLimbs * 32; index-- > 0;) {
        shiftLeftOne(remainder);
        if (bit(a, index)) {
            remainder[0] |= 1u;
        }
        if (compare(remainder, divisor) >= 0) {
            subtract(remainder, divisor);
            quotient[index / 32] |= 1u << (index % 32);
        }
    }
    if (truncate || isZero(remainder)) {
        return quotient;
    }
    Big twice = remainder;
    shiftLeftOne(twice);
    const int half = compare(twice, divisor);
    if (half > 0 || (half == 0 && (quotient[0] & 1u) != 0)) {
        addOne(quotient);
    }
    return quotient;
}

// The magnitude and sign of a two's complement 128-bit value.
Big magnitude(Decimal value, bool& negative) noexcept
{
    negative = value.high < 0;
    std::uint64_t low = value.low;
    auto high = static_cast<std::uint64_t>(value.high);
    if (negative) {
        low = ~low + 1;
        high = ~high + (low == 0 ? 1 : 0);
    }
    return bigOf(low, high);
}

// The largest magnitude of the type: 2^63 - 1 (Currency) or 2^127 - 1
// (Crypto), one more when negative.
Big largest(int places, bool negative) noexcept
{
    Big limit = places == 6 ? bigOf(0x7FFFFFFFFFFFFFFFULL) : bigOf(~0ULL, 0x7FFFFFFFFFFFFFFFULL);
    if (negative) {
        addOne(limit);
    }
    return limit;
}

// The signed value of a magnitude, false when it does not fit the type.
bool signedOf(const Big& value, bool negative, int places, Decimal& out) noexcept
{
    if (compare(value, largest(places, negative)) > 0) {
        return false;
    }
    std::uint64_t low = value[0] | (static_cast<std::uint64_t>(value[1]) << 32);
    std::uint64_t high = value[2] | (static_cast<std::uint64_t>(value[3]) << 32);
    if (negative) {
        low = ~low + 1;
        high = ~high + (low == 0 ? 1 : 0);
    }
    out.low = low;
    out.high = static_cast<std::int64_t>(high);
    if (places == 6) {
        out.high = static_cast<std::int64_t>(low) < 0 ? -1 : 0;
    }
    return true;
}

void store(const Decimal& value, void* out) noexcept
{
    std::memcpy(out, &value.low, 8);
    std::memcpy(static_cast<char*>(out) + 8, &value.high, 8);
}

// The decimal digits of a magnitude, most significant first.
std::size_t digitsOf(Big value, char* out) noexcept
{
    char reversed[100];
    std::size_t count = 0;
    const Big ten = bigOf(10);
    do {
        Big quotient = divideRounded(value, ten, true);
        Big product{};
        multiply(quotient, ten, product);
        Big remainder = value;
        subtract(remainder, product);
        reversed[count++] = static_cast<char>('0' + remainder[0]);
        value = quotient;
    } while (!isZero(value));
    for (std::size_t index = 0; index < count; ++index) {
        out[index] = reversed[count - 1 - index];
    }
    return count;
}

enum Status : std::int32_t {
    kOk = 0,
    kOverflow = 1,
    kDivisionByZero = 2,
};

} // namespace

std::size_t formatDecimal(Decimal value, int places, char* out) noexcept
{
    bool negative = false;
    const Big amount = magnitude(value, negative);
    char digits[64];
    std::size_t count = digitsOf(amount, digits);
    // At least one digit before the point.
    char padded[64];
    std::size_t length = 0;
    for (std::size_t zero = count; zero < static_cast<std::size_t>(places) + 1; ++zero) {
        padded[length++] = '0';
    }
    std::memcpy(padded + length, digits, count);
    length += count;
    const std::size_t whole = length - static_cast<std::size_t>(places);
    std::size_t fraction = static_cast<std::size_t>(places);
    while (fraction > 2 && padded[whole + fraction - 1] == '0') {
        --fraction;
    }
    std::size_t written = 0;
    if (negative && !isZero(amount)) {
        out[written++] = '-';
    }
    std::memcpy(out + written, padded, whole);
    written += whole;
    out[written++] = '.';
    std::memcpy(out + written, padded + whole, fraction);
    written += fraction;
    out[written] = '\0';
    return written;
}

int parseDecimal(const char* bytes, std::size_t length, int places, Decimal& value) noexcept
{
    std::size_t index = 0;
    const bool negative = length > 0 && bytes[0] == '-';
    if (negative) {
        ++index;
    }
    Big amount{};
    const Big ten = bigOf(10);
    std::size_t wholeDigits = 0;
    int fractionDigits = 0;
    bool point = false;
    for (; index < length; ++index) {
        const char ch = bytes[index];
        if (ch == '.' && !point && wholeDigits > 0) {
            point = true;
            continue;
        }
        if (ch < '0' || ch > '9') {
            return 1;
        }
        if (point) {
            if (++fractionDigits > places) {
                return 1;
            }
        } else {
            ++wholeDigits;
        }
        if (!multiply(amount, ten, amount)) {
            return 2;
        }
        Big digit = bigOf(static_cast<std::uint64_t>(ch - '0'));
        // amount += digit
        std::uint64_t carry = digit[0];
        for (std::uint32_t& limb : amount) {
            const std::uint64_t sum = static_cast<std::uint64_t>(limb) + carry;
            limb = static_cast<std::uint32_t>(sum);
            carry = sum >> 32;
            if (carry == 0) {
                break;
            }
        }
    }
    if (wholeDigits == 0 || (point && fractionDigits == 0)) {
        return 1;
    }
    if (!multiply(amount, powerOfTen(places - fractionDigits), amount)) {
        return 2;
    }
    return signedOf(amount, negative, places, value) ? 0 : 2;
}

} // namespace inox::runtime

using inox::runtime::Decimal;

extern "C" {

// A1: a * b and a / b, exact, then rounded half to even to the type's
// places. Status 1 beyond the type, 2 division by zero.
std::int32_t __inox_dec_mul(std::uint64_t aLow, std::int64_t aHigh, std::uint64_t bLow, std::int64_t bHigh,
                            std::int32_t places, void* out) noexcept
{
    using namespace inox::runtime;
    bool aNegative = false;
    bool bNegative = false;
    const Big a = magnitude(decimalOf(aLow, aHigh), aNegative);
    const Big b = magnitude(decimalOf(bLow, bHigh), bNegative);
    Big product{};
    if (!multiply(a, b, product)) {
        return kOverflow;
    }
    const Big scaled = divideRounded(product, powerOfTen(places), false);
    Decimal result;
    if (!signedOf(scaled, aNegative != bNegative && !isZero(scaled), places, result)) {
        return kOverflow;
    }
    store(result, out);
    return kOk;
}

std::int32_t __inox_dec_div(std::uint64_t aLow, std::int64_t aHigh, std::uint64_t bLow, std::int64_t bHigh,
                            std::int32_t places, void* out) noexcept
{
    using namespace inox::runtime;
    bool aNegative = false;
    bool bNegative = false;
    const Big a = magnitude(decimalOf(aLow, aHigh), aNegative);
    const Big b = magnitude(decimalOf(bLow, bHigh), bNegative);
    if (isZero(b)) {
        return kDivisionByZero;
    }
    Big numerator{};
    if (!multiply(a, powerOfTen(places), numerator)) {
        return kOverflow;
    }
    const Big quotient = divideRounded(numerator, b, false);
    Decimal result;
    if (!signedOf(quotient, aNegative != bNegative && !isZero(quotient), places, result)) {
        return kOverflow;
    }
    store(result, out);
    return kOk;
}

// A4: a value of `from` places in `to` places: exact when there are more,
// rounded half to even when fewer (Currency of a Crypto). Status 1 beyond
// the type.
std::int32_t __inox_dec_rescale(std::uint64_t low, std::int64_t high, std::int32_t from, std::int32_t to,
                                void* out) noexcept
{
    using namespace inox::runtime;
    bool negative = false;
    Big amount = magnitude(decimalOf(low, high), negative);
    if (to > from) {
        if (!multiply(amount, powerOfTen(to - from), amount)) {
            return kOverflow;
        }
    } else if (to < from) {
        amount = divideRounded(amount, powerOfTen(from - to), false);
    }
    Decimal result;
    if (!signedOf(amount, negative && !isZero(amount), to, result)) {
        return kOverflow;
    }
    store(result, out);
    return kOk;
}

// A3: Integer(C) truncates toward zero (`round` 0); Round(C) rounds half to
// even (`round` 1). Status 1 beyond Int64.
std::int64_t __inox_dec_to_int(std::uint64_t low, std::int64_t high, std::int32_t places, std::int32_t round,
                               std::int32_t* status) noexcept
{
    using namespace inox::runtime;
    bool negative = false;
    const Big amount = divideRounded(magnitude(decimalOf(low, high), negative), powerOfTen(places), round == 0);
    Decimal result;
    if (!signedOf(amount, negative && !isZero(amount), 6, result)) {
        *status = kOverflow;
        return 0;
    }
    *status = kOk;
    return static_cast<std::int64_t>(result.low);
}

// Currency(F), Crypto(F): the exact binary value of F rounded half to even
// to the type's places. Status 1 for NaN, an infinity or beyond the type.
std::int32_t __inox_dec_from_f64(double number, std::int32_t places, void* out) noexcept
{
    using namespace inox::runtime;
    if (!std::isfinite(number)) {
        return kOverflow;
    }
    const bool negative = std::signbit(number);
    int exponent = 0;
    const double fraction = std::frexp(std::fabs(number), &exponent);
    // |number| = mantissa * 2^(exponent - 53), mantissa < 2^53.
    const auto mantissa = static_cast<std::uint64_t>(std::ldexp(fraction, 53));
    int shift = exponent - 53;
    Big amount{};
    multiply(bigOf(mantissa), powerOfTen(places), amount);
    if (shift > 0) {
        if (shift > 200) {
            return kOverflow;
        }
        for (int step = 0; step < shift; ++step) {
            shiftLeftOne(amount);
        }
    } else if (shift < 0) {
        if (shift < -(static_cast<int>(kLimbs) * 32 - 1)) {
            amount = Big{};
        } else {
            Big divisor = bigOf(1);
            for (int step = 0; step < -shift; ++step) {
                shiftLeftOne(divisor);
            }
            amount = divideRounded(amount, divisor, false);
        }
    }
    Decimal result;
    if (!signedOf(amount, negative && !isZero(amount), places, result)) {
        return kOverflow;
    }
    store(result, out);
    return kOk;
}

// Float(C) and Float32(C): the nearest real, rounded once from the exact
// decimal text.
double __inox_dec_to_f64(std::uint64_t low, std::int64_t high, std::int32_t places) noexcept
{
    char text[inox::runtime::kDecimalTextCapacity];
    inox::runtime::formatDecimal(inox::runtime::decimalOf(low, high), places, text);
    return std::strtod(text, nullptr);
}

float __inox_dec_to_f32(std::uint64_t low, std::int64_t high, std::int32_t places) noexcept
{
    char text[inox::runtime::kDecimalTextCapacity];
    inox::runtime::formatDecimal(inox::runtime::decimalOf(low, high), places, text);
    return std::strtof(text, nullptr);
}

} // extern "C"
