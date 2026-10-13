// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ExceptionSupport.h"

#include "LlvmIrEmitter.h"
#include "../exceptions/ExceptionTypes.h"
#include "../support/Platform.h"

#include <string>

namespace inox::compiler::codegen::detail {

std::uint64_t exceptionTypeId(std::string_view name)
{
    const auto* info = exceptions::findExceptionType(name);
    if (info == nullptr) {
        throw CodegenUnsupported("unknown exception type for LLVM emission: " + std::string(name));
    }
    return info->typeId;
}

bool usesMsvcExceptionAbi()
{
    // PoC selector. Marcelo's Windows policy is MSVC ABI only. This is
    // intentionally host-based for the PoC; the explicit target descriptor
    // will replace it before cross-target code generation is generalized.
    return support::hostOperatingSystem() ==
           support::OperatingSystem::Windows;
}

} // namespace inox::compiler::codegen::detail
