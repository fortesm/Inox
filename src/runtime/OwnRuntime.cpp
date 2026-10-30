// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Safe pointers (OpenSpec change safe-pointers): the blocks of owned objects
// (`Own<T>`, `New`, `Free`) and the generations that check every `Link<T>`.
// No reference counting and no collector: a block has one owner, which
// frees it. Each block starts with a header holding a 64-bit generation; a
// Link keeps the generation it saw, and freeing the block increments it, so
// a Link to a freed block never matches again, also after the block is
// reused for another object. Freed blocks go to a free list of their size
// and are never returned to the system, so a header stays readable for as
// long as the program runs (a Link to a freed object reads a header, never
// unmapped memory).
//
// Freeing an object frees the objects it owns (its drop function frees its
// Own fields). The frees are queued and run in a loop, never by recursion,
// so a list of a million nodes does not overflow the stack.
//
// A tombstone is a block of size 0 whose generation stands for a variable
// of the stack: a Link to a variable carries the variable's tombstone, and
// the variable's routine frees the tombstone when the variable ends.

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

struct alignas(16) BlockHeader {
    std::uint64_t generation;
    std::uint64_t size;
};

static_assert(sizeof(BlockHeader) == 16, "the payload of a block is 16-byte aligned");

using DropFunction = void (*)(void*);

BlockHeader* headerOf(void* payload) noexcept
{
    return static_cast<BlockHeader*>(payload) - 1;
}

std::unordered_map<std::uint64_t, std::vector<BlockHeader*>>& freeLists() noexcept
{
    static auto* lists = new std::unordered_map<std::uint64_t, std::vector<BlockHeader*>>();
    return *lists;
}

std::vector<std::pair<void*, DropFunction>>& pendingFrees() noexcept
{
    static auto* pending = new std::vector<std::pair<void*, DropFunction>>();
    return *pending;
}

bool freeing = false;

} // namespace

extern "C" {

// A new zeroed block of `size` bytes; null when memory is exhausted (the
// program turns that into the "out of memory" trap).
void* __inox_own_alloc(std::int64_t size) noexcept
{
    const auto bytes = static_cast<std::uint64_t>(size < 0 ? 0 : size);
    BlockHeader* header = nullptr;
    try {
        auto& list = freeLists()[bytes];
        if (!list.empty()) {
            header = list.back();
            list.pop_back();
        }
    } catch (...) {
        return nullptr;
    }
    if (header == nullptr) {
        header = static_cast<BlockHeader*>(std::calloc(1, sizeof(BlockHeader) + bytes));
        if (header == nullptr) {
            return nullptr;
        }
        header->generation = 1;
        header->size = bytes;
    } else {
        void* payload = header + 1;
        for (std::uint64_t index = 0; index < bytes; ++index) {
            static_cast<unsigned char*>(payload)[index] = 0;
        }
    }
    return header + 1;
}

// Frees the object at `payload` (null does nothing): `drop` releases what it
// holds (Strings, owned objects) first. Every Link to it is dead afterwards.
void __inox_own_free(void* payload, DropFunction drop) noexcept
{
    if (payload == nullptr) {
        return;
    }
    try {
        pendingFrees().emplace_back(payload, drop);
    } catch (...) {
        std::abort();
    }
    if (freeing) {
        return;
    }
    freeing = true;
    auto& pending = pendingFrees();
    while (!pending.empty()) {
        const auto [object, function] = pending.back();
        pending.pop_back();
        BlockHeader* header = headerOf(object);
        ++header->generation;
        if (function != nullptr) {
            function(object);
        }
        try {
            freeLists()[header->size].push_back(header);
        } catch (...) {
            std::abort();
        }
    }
    freeing = false;
}

} // extern "C"
