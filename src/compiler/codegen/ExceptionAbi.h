// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// How exceptions are caught on the target (OpenSpec change
// compiler-modularization, step 3). Inox code never sees the ABI: a pad
// catches every C++ exception thrown by libinoxrt, captures it as an opaque
// Inox exception state and continues in ordinary control flow. Linux and the
// other Unix-like hosts use the Itanium ABI (landingpad); Windows uses the
// MSVC ABI (funclets: catchswitch, catchpad, catchret).

#pragma once

#include <cstdint>
#include <functional>
#include <ostream>
#include <string>
#include <string_view>

namespace inox::compiler::codegen::detail {

// Returns a fresh SSA value name with the given prefix ("%eh.state" ->
// "%eh.state12"); the emitter owns the counter.
using NewValueName = std::function<std::string(std::string_view prefix)>;

class ExceptionAbi {
public:
    virtual ~ExceptionAbi() = default;

    // The personality function named by every routine that has pads.
    virtual std::string_view personality() const = 0;

    // Emits the pad labelled padLabel: it catches any exception, captures it
    // with __inox_exception_capture and leaves the pad. Returns the SSA value
    // that holds the captured state; the caller continues the code that
    // follows in the same (parent) block.
    virtual std::string emitCapturePad(std::ostream& output, std::string_view padLabel,
                                       const NewValueName& newValue) const = 0;
};

class ItaniumExceptionAbi final : public ExceptionAbi {
public:
    std::string_view personality() const override;
    std::string emitCapturePad(std::ostream& output, std::string_view padLabel,
                               const NewValueName& newValue) const override;
};

class MsvcExceptionAbi final : public ExceptionAbi {
public:
    std::string_view personality() const override;
    std::string emitCapturePad(std::ostream& output, std::string_view padLabel,
                               const NewValueName& newValue) const override;
};

// The ABI of the host: MSVC on Windows, Itanium elsewhere. Host-based for
// now; an explicit target descriptor replaces it with cross compilation.
const ExceptionAbi& hostExceptionAbi();

// The runtime type id of a standard exception type (CANON-15).
std::uint64_t exceptionTypeId(std::string_view name);

} // namespace inox::compiler::codegen::detail
