// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// libinoxrt owned blocks (OwnRuntime.cpp, change safe-pointers).

#include <cstdint>
#include <cstdio>

extern "C" {
void* __inox_own_alloc(std::int64_t size) noexcept;
void __inox_own_free(void* payload, void (*drop)(void*)) noexcept;
}

namespace {

int failures = 0;

void check(bool condition, const char* what)
{
    if (!condition) {
        std::printf("[FAIL] inoxrt own %s\n", what);
        ++failures;
    }
}

std::uint64_t generationOf(void* payload)
{
    return *(static_cast<std::uint64_t*>(payload) - 2);
}

struct Node {
    std::int64_t value;
    void* next;
};

int dropped = 0;

void dropNode(void* payload)
{
    ++dropped;
    __inox_own_free(static_cast<Node*>(payload)->next, dropNode);
}

void testAllocFree()
{
    void* block = __inox_own_alloc(24);
    check(block != nullptr, "alloc returns a block");
    check(reinterpret_cast<std::uintptr_t>(block) % 16 == 0, "the payload is 16-byte aligned");
    check(static_cast<std::int64_t*>(block)[0] == 0 && static_cast<std::int64_t*>(block)[2] == 0,
          "a new block is zeroed");
    const std::uint64_t seen = generationOf(block);
    static_cast<std::int64_t*>(block)[1] = 7;
    __inox_own_free(block, nullptr);
    check(generationOf(block) != seen, "free changes the generation");
    void* again = __inox_own_alloc(24);
    check(again == block, "a freed block of the same size is reused");
    check(static_cast<std::int64_t*>(again)[1] == 0, "a reused block is zeroed");
    check(generationOf(again) != seen, "a reused block never matches an old generation");
    __inox_own_free(again, nullptr);
    __inox_own_free(nullptr, nullptr);
    void* tombstone = __inox_own_alloc(0);
    check(tombstone != nullptr, "a tombstone (size 0) is a block");
    __inox_own_free(tombstone, nullptr);
}

void testLongList()
{
    // A million nodes: the frees run in a loop, never by recursion.
    void* head = nullptr;
    for (int index = 0; index < 1000000; ++index) {
        auto* node = static_cast<Node*>(__inox_own_alloc(sizeof(Node)));
        node->value = index;
        node->next = head;
        head = node;
    }
    dropped = 0;
    __inox_own_free(head, dropNode);
    check(dropped == 1000000, "freeing a list frees every node without recursion");
}

} // namespace

int runOwnRuntimeTests()
{
    testAllocFree();
    testLongList();
    if (failures == 0) {
        std::printf("[PASS] inoxrt own runtime unit tests\n");
    }
    return failures;
}
