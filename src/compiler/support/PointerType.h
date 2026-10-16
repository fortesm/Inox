// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <string>
#include <string_view>

namespace inox::compiler::support {

// Safe pointers (OpenSpec change safe-pointers) travel through the front end
// as type names, like `Slice<T>`: `Own<T>` (the single owner of a heap
// object), `Option<Own<T>>` (an owner that may be absent) and `Link<T>` (a
// reference that does not own, checked by a generation on each use).
bool isOwnTypeName(std::string_view name);
bool isOptionOwnTypeName(std::string_view name);
bool isLinkTypeName(std::string_view name);
// `Option<X>` with any X (only Option<Own<T>> is accepted by the analyzer).
bool isOptionTypeName(std::string_view name);
// Own, Option<Own> or Link.
bool isPointerTypeName(std::string_view name);
// The T of Own<T>, Option<Own<T>> or Link<T>; empty for any other name.
std::string pointerTargetTypeName(std::string_view name);
// The X of Option<X>.
std::string optionInnerTypeName(std::string_view name);
std::string makeOwnTypeName(std::string_view target);
std::string makeOptionOwnTypeName(std::string_view target);
std::string makeLinkTypeName(std::string_view target);
// A type written with `<...>`: Slice, Own, Option or Link.
bool isGenericTypeName(std::string_view name);

} // namespace inox::compiler::support
