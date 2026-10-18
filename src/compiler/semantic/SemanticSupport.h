// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Helpers shared by the SemanticAnalyzer source files (OpenSpec change
// compiler-modularization, step 4): names and operators, the prelude
// signatures, integer ranges and literals, checked constant arithmetic, case
// domains.

#pragma once

#include "../ast/Ast.h"
#include "../types/Type.h"

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace inox::compiler::semantic::detail {

bool equalsIgnoreCase(std::string_view left, std::string_view right);
// Whether the program loaded the module `name` (directly or through Use).
bool usesModule(const ast::ModuleNode& module, std::string_view name);
std::string_view binaryOperatorName(ast::BinaryOperator op);

struct PreludeSignature {
    std::string_view name;
    std::vector<std::string_view> parameterTypes;
    std::string_view returnType;
};

const std::vector<PreludeSignature>& preludeSignatures();

// The range of an integer type, or false for any other type (or null).
bool integerTypeRange(const types::Type* type, std::int64_t& low, std::int64_t& high);
bool isNarrowIntegerType(const types::Type* type);

inline constexpr std::int64_t kInt64Max = std::numeric_limits<std::int64_t>::max();
inline constexpr std::int64_t kInt64Min = std::numeric_limits<std::int64_t>::min();
inline constexpr std::uint64_t kInt64MagnitudeOfMin = static_cast<std::uint64_t>(1) << 63;

// UInt64 (change uint64): the one integer type whose values do not all fit
// Int64. Its constants are kept as their 64-bit pattern in an std::int64_t
// and read unsigned.
bool isUInt64Type(const types::Type* type);
inline constexpr std::uint64_t kUInt64Max = std::numeric_limits<std::uint64_t>::max();

// A constant integer: its 64-bit pattern, read unsigned for a UInt64.
struct IntegerConstant {
    std::int64_t bits = 0;
    bool isUnsigned = false;
};
// Whether the constant is a value of the integer type `target`.
bool constantFits(IntegerConstant value, const types::Type* target);
// The constant as a message writes it.
std::string constantText(IntegerConstant value);
// Checked UInt64 arithmetic for constant folding; false on overflow.
bool checkedAddUnsigned(std::uint64_t a, std::uint64_t b, std::uint64_t& out);
bool checkedSubUnsigned(std::uint64_t a, std::uint64_t b, std::uint64_t& out);
bool checkedMulUnsigned(std::uint64_t a, std::uint64_t b, std::uint64_t& out);

// The magnitude of an integer literal (decimal, $hex or 0x hex, `_`
// separators); false when it does not fit in 64 bits.
bool parseIntegerLiteralMagnitude(std::string_view text, std::uint64_t& value);
// Checked Int64 arithmetic for constant folding; false on overflow.
bool checkedAdd(std::int64_t a, std::int64_t b, std::int64_t& out);
bool checkedSub(std::int64_t a, std::int64_t b, std::int64_t& out);
bool checkedMul(std::int64_t a, std::int64_t b, std::int64_t& out);
// Throws the SemanticError for an overflowing constant operation.
[[noreturn]] void throwConstantOverflow(const char* what, const char* type = "Int64");
// The Unicode scalar value of a Char literal ('a', '\n', ...).
bool decodeCharLiteral(std::string_view text, std::int64_t& value);

// ADR-0011: the value domain of a case selector. Values are compared as Int64:
// Bool is 0/1 and Char is its Unicode scalar value. `provable` is false when the
// domain does not fit in Int64 (UInt64), so coverage cannot be shown and
// `else` is required.
struct CaseDomain {
    std::int64_t low = 0;
    std::int64_t high = 0;
    bool provable = true;
    bool isBool = false;
    bool isChar = false;
    // UInt64: values are compared by a key that keeps their unsigned order
    // in Int64 (the pattern with its top bit flipped, caseKey).
    bool isUnsigned64 = false;
};

// The Int64 key of a UInt64 pattern that orders like the unsigned value, and back.
inline std::int64_t caseKey(std::int64_t bits)
{
    return static_cast<std::int64_t>(static_cast<std::uint64_t>(bits) ^ (static_cast<std::uint64_t>(1) << 63));
}

bool caseDomainFor(const types::Type* type, CaseDomain& domain);
std::string caseValueText(std::int64_t value, const CaseDomain& domain);

// `Name := Value` in a construction argument list, or null.
const ast::BinaryExpression* namedArgument(const ast::Expression& argument);
// An expression built only from literals and operators.
bool isLiteralOnly(const ast::Expression& expression);
// The identifiers an expression reads.
void collectNames(const ast::Expression& expression, std::vector<std::string>& names);

} // namespace inox::compiler::semantic::detail
