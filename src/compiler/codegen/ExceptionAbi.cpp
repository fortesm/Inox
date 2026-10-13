// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ExceptionAbi.h"

#include "LlvmIrEmitter.h"
#include "../exceptions/ExceptionTypes.h"
#include "../support/Platform.h"

namespace inox::compiler::codegen::detail {

std::string_view ItaniumExceptionAbi::personality() const
{
    return "__gxx_personality_v0";
}

std::string ItaniumExceptionAbi::emitCapturePad(std::ostream& output, std::string_view padLabel,
                                                const NewValueName& newValue) const
{
    output << padLabel << ":\n";
    const std::string landing = newValue("%eh.lp");
    const std::string raw = newValue("%eh.raw");
    const std::string state = newValue("%eh.state");
    output << "  " << landing << " = landingpad { ptr, i32 } catch ptr null\n";
    output << "  " << raw << " = extractvalue { ptr, i32 } " << landing << ", 0\n";
    output << "  " << state << " = call ptr @__inox_exception_capture(ptr " << raw << ")\n";
    return state;
}

std::string_view MsvcExceptionAbi::personality() const
{
    return "__CxxFrameHandler3";
}

// MSVC EH funclets are only an ABI bridge. No Inox user statement is emitted
// inside the catchpad: capture the active C++ exception, carry the opaque
// state in SSA, and return to the parent CFG with catchret.
std::string MsvcExceptionAbi::emitCapturePad(std::ostream& output, std::string_view padLabel,
                                             const NewValueName& newValue) const
{
    const std::string catchSwitch = newValue("%eh.cs");
    const std::string catchPad = newValue("%eh.cp");
    const std::string state = newValue("%eh.state");
    const std::string catchPadLabel = std::string(padLabel) + ".catch";
    const std::string parentLabel = std::string(padLabel) + ".parent";

    output << padLabel << ":\n";
    output << "  " << catchSwitch << " = catchswitch within none [label %" << catchPadLabel
           << "] unwind to caller\n\n";
    output << catchPadLabel << ":\n";
    output << "  " << catchPad << " = catchpad within " << catchSwitch << " [ptr null, i32 64, ptr null]\n";
    output << "  " << state << " = call ptr @__inox_exception_capture(ptr null)"
           << " [ \"funclet\"(token " << catchPad << ") ]\n";
    output << "  catchret from " << catchPad << " to label %" << parentLabel << "\n\n";
    output << parentLabel << ":\n";
    return state;
}

const ExceptionAbi& hostExceptionAbi()
{
    static const ItaniumExceptionAbi itanium;
    static const MsvcExceptionAbi msvc;
    if (support::hostOperatingSystem() == support::OperatingSystem::Windows) {
        return msvc;
    }
    return itanium;
}

std::uint64_t exceptionTypeId(std::string_view name)
{
    const auto* info = exceptions::findExceptionType(name);
    if (info == nullptr) {
        throw CodegenUnsupported("unknown exception type for LLVM emission: " + std::string(name));
    }
    return info->typeId;
}

} // namespace inox::compiler::codegen::detail
