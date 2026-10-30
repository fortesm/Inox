// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Internal to libinoxrt: the console (change console-abi-terminal, r3.88).
// Every console byte of a program goes through ConsoleRuntime.cpp, so the
// library's own C runtime owns standard output and standard input.

#pragma once

namespace inox::runtime {

// Flushes standard output and gives the terminal back as the program found
// it (raw mode, cursor, colors, Windows console modes and code pages). Safe
// to call more than once and from any exit path: return from Main, a trap,
// out of memory, an exception nobody handles, a signal.
void shutdownConsole() noexcept;

// Stops the program with "Inox runtime error: <category>" and status 70,
// after shutdownConsole (CANON-19).
[[noreturn]] void fault(const char* category) noexcept;

// The command line Main received (SystemRuntime.cpp, change
// datetime-systemutils, r3.92).
void setCommandLine(int argc, char** argv) noexcept;

} // namespace inox::runtime
