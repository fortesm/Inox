// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The written form of a type, as the parser records it in the AST
// (`Int64`, `Array[1..Size] Byte`, `Link<Array[1..5] TItem>`), read into a
// tree (OpenSpec change type-representation). Reading checks only the
// shape; the analyzer resolves names, bounds and the language rules.

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace inox::compiler::types {

struct TypeSyntax {
    enum class Form { Named, Array, Generic };

    Form form = Form::Named;
    // Named: the type name; Generic: Slice, Own, Link or Option as written.
    std::string name;
    // Array: the bounds as written (a literal or a Const name).
    std::string low;
    std::string high;
    // Array: the element; Generic: the argument.
    std::shared_ptr<const TypeSyntax> argument;

    // The text this tree was read from (canonical spacing).
    std::string spelling() const;
};

// Null when the text is not a type's shape (an empty name, a malformed
// array, unbalanced angle brackets).
std::optional<TypeSyntax> readTypeSyntax(std::string_view text);

// The written form of a fixed array type, which the parser records.
std::string spellArrayType(std::string_view low, std::string_view high, std::string_view element);

// Shape tests on written type text, for code that has only the text.
bool isArraySpelling(std::string_view text);
// `Slice<...>`, `Own<...>`, `Link<...>` or `Option<...>`.
bool isGenericSpelling(std::string_view text);
// The generic's name is `name` (case-insensitive), e.g. "Own".
bool isGenericSpelling(std::string_view text, std::string_view name);

} // namespace inox::compiler::types
