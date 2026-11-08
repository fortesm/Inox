// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Host of tests/native/owner-check.inox: calls Peek(P Own<TNode>) with an
// empty owner. An owner crosses the ABI as one pointer, and null is the
// state Move and Free leave in a slot, so the call is well defined and
// reaches the routine's owner check (trap 17).

#include <cstdint>

extern "C" std::int64_t inox_peek(void* owner);

int main()
{
    return static_cast<int>(inox_peek(nullptr));
}
