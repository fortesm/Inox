// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Internal to libinoxrt: exact decimals (change decimals, canon r3.91). A
// Currency is an Int64 scaled by 10^6, a Crypto an Int128 scaled by 10^18.
// Generated code passes either as two 64-bit halves (the low one unsigned,
// the high one its sign for a Currency) and receives one through a pointer
// to 16 bytes, low half first, so no 128-bit value crosses the C ABI, and no
// 128-bit division (a compiler-rt helper on Windows) is ever compiled.

#pragma once

#include <cstddef>
#include <cstdint>

namespace inox::runtime {

struct Decimal {
    std::uint64_t low = 0;
    std::int64_t high = 0;
};

inline Decimal decimalOf(std::uint64_t low, std::int64_t high) noexcept
{
    return Decimal{low, high};
}

// Room for any text formatDecimal writes, NUL included.
inline constexpr std::size_t kDecimalTextCapacity = 64;

// A5: the value with at least two decimals and no trailing zeros after
// them: 19.99, 5.00, 0.123456, -0.50. Returns the length.
std::size_t formatDecimal(Decimal value, int places, char* out) noexcept;

// B2, C2: the whole text is an optional '-', digits and an optional
// fraction of at most `places` digits; the value fits the type (Currency
// when places is 6). Status 0 ok, 1 not such a text, 2 beyond the type.
int parseDecimal(const char* bytes, std::size_t length, int places, Decimal& value) noexcept;

} // namespace inox::runtime
