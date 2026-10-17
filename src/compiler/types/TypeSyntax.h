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

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace inox::compiler::types {

struct TypeSyntax {
    enum class Form { Named, Array, Generic };

    Form form = Form::Named;
    // Named: the type name; Generic: its name as written (Slice, Own, Link,
    // Option, or a generic struct of the program such as TPair).
    std::string name;
    // Array: the bounds as written (a literal or a Const name).
    std::string low;
    std::string high;
    // Array: one, the element; Generic: the type arguments, in order.
    std::vector<std::shared_ptr<const TypeSyntax>> arguments;

    // The first argument: an array's element, or a pointer's target.
    const TypeSyntax& argument() const { return *arguments.front(); }

    // The text this tree was read from (canonical spacing: `TPair<K, V>`).
    std::string spelling() const;
};

// Null when the text is not a type's shape (an empty name, a malformed
// array, unbalanced angle brackets, an empty argument).
std::optional<TypeSyntax> readTypeSyntax(std::string_view text);

// The written form of a fixed array type, which the parser records.
std::string spellArrayType(std::string_view low, std::string_view high, std::string_view element);

// Shape tests on written type text, for code that has only the text.
bool isArraySpelling(std::string_view text);
// `Name<...>`: Slice, Own, Link, Option or a generic struct.
bool isGenericSpelling(std::string_view text);
// The generic's name is `name` (case-insensitive), e.g. "Own".
bool isGenericSpelling(std::string_view text, std::string_view name);

// The written type with every Named part that `replace` maps replaced by
// the text it returns (an empty result keeps the name). A generic instance
// replaces its type parameters this way: with T = Int64, `Array[1..N] T`
// becomes `Array[1..N] Int64`. Text that is not a type is returned as is.
std::string substituteTypeNames(std::string_view written,
                                const std::function<std::string(std::string_view)>& replace);

} // namespace inox::compiler::types
