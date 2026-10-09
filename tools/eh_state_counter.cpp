// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Test-only wrapper used by tools/eh_state_balance.py. Linked with
// -Wl,--wrap=... (GNU ld), it counts the exception states libinoxrt creates and
// destroys and prints the balance at exit. It is never part of libinoxrt.
#include <cstdio>
#include <cstdlib>
#include <set>

extern "C" void* __real___inox_exception_capture(void* raw);
extern "C" void __real___inox_exception_release(void* state) noexcept;
extern "C" [[noreturn]] void __real___inox_exception_rethrow(void* state);

namespace {
std::set<void*>* live = nullptr;
long captures = 0;
long releases = 0;

void report()
{
    std::fprintf(stderr, "[eh-state] captures=%ld releases=%ld live=%zu\n",
                 captures, releases, live != nullptr ? live->size() : 0);
}

void forget(void* state)
{
    if (state != nullptr) {
        ++releases;
        if (live != nullptr) {
            live->erase(state);
        }
    }
}
}  // namespace

extern "C" void* __wrap___inox_exception_capture(void* raw)
{
    if (live == nullptr) {
        live = new std::set<void*>;
        std::atexit(report);
    }
    void* state = __real___inox_exception_capture(raw);
    ++captures;
    live->insert(state);
    return state;
}

extern "C" void __wrap___inox_exception_release(void* state) noexcept
{
    forget(state);
    __real___inox_exception_release(state);
}

// Rethrow consumes the state (the runtime deletes it before rethrowing).
extern "C" [[noreturn]] void __wrap___inox_exception_rethrow(void* state)
{
    forget(state);
    __real___inox_exception_rethrow(state);
}
