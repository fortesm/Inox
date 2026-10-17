// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// A minimal check helper for the compiler unit tests (no framework needed).

#pragma once

#include <cstdio>
#include <string>

namespace inox::compiler::unit {

inline int& failureCount()
{
    static int failures = 0;
    return failures;
}

inline void check(bool condition, const std::string& what)
{
    if (!condition) {
        std::printf("[FAIL] compiler unit: %s\n", what.c_str());
        ++failureCount();
    }
}

inline void checkEqual(const std::string& actual, const std::string& expected, const std::string& what)
{
    if (actual != expected) {
        std::printf("[FAIL] compiler unit: %s\n  expected: %s\n  actual:   %s\n", what.c_str(), expected.c_str(),
                    actual.c_str());
        ++failureCount();
    }
}

} // namespace inox::compiler::unit
