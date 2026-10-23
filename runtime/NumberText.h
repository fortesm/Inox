// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Internal to libinoxrt: numbers as text (change numbers-text, canon r3.90),
// one implementation for the console (Put), files (Write), String(X) and the
// conversions from text, so a number prints the same way everywhere.

#pragma once

#include <cstddef>
#include <cstdint>

namespace inox::runtime {

// Room for any text formatReal writes, NUL included.
inline constexpr std::size_t kRealTextCapacity = 64;

// B3: the shortest text that reads back as the same value (of a Float32 when
// `float32`): fixed notation for exponents -4..15 with at least one digit
// after the point (`2.5`, `1500.0`, `0.30000000000000004`), else an exponent
// (`1.0e16`, `1.5e-5`); `-0.0`, `NaN`, `Inf`, `-Inf`. Returns the length.
std::size_t formatReal(double value, bool float32, char* out) noexcept;

// FormatFloat(F, Decimals): fixed notation with exactly `decimals` digits
// after the point (none and no point when 0), rounded to nearest from the
// exact binary value. `out` holds at least 400 + decimals bytes.
std::size_t formatFixed(double value, int decimals, char* out, std::size_t capacity) noexcept;

// B2: the whole text is an optional '-' and decimal digits, nothing else,
// and the value lies in [low, high]. False otherwise.
bool parseIntegerText(const char* bytes, std::size_t length, std::int64_t low, std::int64_t high,
                      std::int64_t& value) noexcept;
bool parseUnsignedText(const char* bytes, std::size_t length, std::uint64_t& value) noexcept;

// B2: the whole text is an optional '-', digits, an optional fraction and an
// optional exponent; the value is finite in the type (a Float32 rounded once
// from the text). False otherwise.
bool parseRealText(const char* bytes, std::size_t length, bool float32, double& value) noexcept;

} // namespace inox::runtime
