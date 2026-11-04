// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The written form of a type (change structured-types):
// what the program wrote, built by the parser, resolved by the analyzer to a
// types::Type. `ast` does not know `types`. The invariants are those of
// design.md, checked by checkTypeExpr.

#pragma once

#include "SourcePosition.h"

#include <string>
#include <string_view>
#include <vector>

namespace inox::compiler::ast {

// One array bound as written: an integer literal (with its sign, as the
// lexer's text gives it: `-1`, `$1F`, `1000`) or a Const name (CANON-13).
struct ArrayBound {
    std::string text;
    bool isName = false;
};

struct TypeExpr {
    enum class Form { Named, Array, Generic };

    Form form = Form::Named;
    // Named: the name, possibly qualified (`Bank.Accounts.TAccount`).
    // Generic: Slice, Own, Link, Option (spelled with their capitals), Set
    // or a generic struct (`TPair`), as written.
    std::string name;
    ArrayBound low;   // Array only
    ArrayBound high;  // Array only
    // Array: one, the element; Generic: the type arguments, in order.
    std::vector<TypeExpr> arguments;
    SourcePosition position;  // where the type starts

    const TypeExpr& element() const { return arguments.front(); }
    bool isNamed() const { return form == Form::Named; }
    bool isArray() const { return form == Form::Array; }
    bool isGeneric() const { return form == Form::Generic; }
    // A generic named `generic` (case-insensitive), e.g. "Own".
    bool isGeneric(std::string_view generic) const;
};

// A Named type.
TypeExpr namedType(std::string name, SourcePosition position = {});

// The canonical text (design.md, "Canonical spelling"): `Array[1..3] Int64`,
// `TPair<String, Own<TNode>>`. Messages, `inox interface` and the names of
// generic instances use it.
std::string spell(const TypeExpr& type);

// The invariants of design.md (Named, Array, Generic shapes); throws
// std::logic_error naming the broken one. Called in Debug builds where a
// tree is made or changed.
void checkTypeExpr(const TypeExpr& type);

} // namespace inox::compiler::ast
