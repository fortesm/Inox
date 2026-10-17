// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// inox-compiler-tests: unit tests of compiler modules that can be tested
// without a whole program (OpenSpec change type-representation).

#include "UnitTest.h"

#include <cstdio>

namespace inox::compiler::unit {
int runTypesTests();
int runTypeLayoutTests();
int runAstCloneTests();
} // namespace inox::compiler::unit

int main()
{
    using namespace inox::compiler::unit;
    runTypesTests();
    runTypeLayoutTests();
    runAstCloneTests();
    if (failureCount() != 0) {
        return 1;
    }
    std::printf("[PASS] compiler unit tests\n");
    return 0;
}
