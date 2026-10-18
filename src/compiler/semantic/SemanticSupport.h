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

// The magnitude of an integer literal (decimal, $hex or 0x hex, `_`
// separators); false when it does not fit in 64 bits.
bool parseIntegerLiteralMagnitude(std::string_view text, std::uint64_t& value);
// Checked Int64 arithmetic for constant folding; false on overflow.
bool checkedAdd(std::int64_t a, std::int64_t b, std::int64_t& out);
bool checkedSub(std::int64_t a, std::int64_t b, std::int64_t& out);
bool checkedMul(std::int64_t a, std::int64_t b, std::int64_t& out);
// Throws the SemanticError for an overflowing constant operation.
[[noreturn]] void throwConstantOverflow(const char* what);
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
};

bool caseDomainFor(const types::Type* type, CaseDomain& domain);
std::string caseValueText(std::int64_t value, const CaseDomain& domain);

// `Name := Value` in a construction argument list, or null.
const ast::BinaryExpression* namedArgument(const ast::Expression& argument);
// An expression built only from literals and operators.
bool isLiteralOnly(const ast::Expression& expression);
// The identifiers an expression reads.
void collectNames(const ast::Expression& expression, std::vector<std::string>& names);

} // namespace inox::compiler::semantic::detail
