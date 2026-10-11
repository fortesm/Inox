// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace inox::compiler::support {

// CANON-13 fixed arrays travel through the front end as type names spelled
// `Array[Low..High] Element`. A multi-dimensional array is written
// `Array[1..3, 1..4] T` in source and spelled as nested one-dimensional
// arrays: `Array[1..3] Array[1..4] T`. Bounds are integer literals or Const
// names until semantic analysis replaces them with their values.
struct ArrayTypeName {
    std::string low;
    std::string high;
    std::string element;
};

std::optional<ArrayTypeName> splitArrayTypeName(std::string_view name);
std::string makeArrayTypeName(std::string_view low, std::string_view high, std::string_view element);
bool isArrayTypeName(std::string_view name);

} // namespace inox::compiler::support
