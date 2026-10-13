// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: Std.Debug Assert (OpenSpec change debug-assert). The
// arguments are evaluated in order; when the condition is false the program
// stops with the runtime fault "assertion failed" (kind 14, CANON-19), after
// the message when one is given. It is never compiled out.

#include "FunctionEmitter.h"

#include "LlvmIrEmitter.h"
#include "LlvmText.h"
#include "RuntimeHelpersIr.h"

#include <string>

namespace inox::compiler::codegen::detail {

bool FunctionEmitter::isAssertCall(const ast::CallExpression& call) const
{
    if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression) {
        return false;
    }
    const auto& callee = static_cast<const ast::IdentifierExpression&>(call.callee());
    return equalsIgnoreCase(callee.name(), "Assert") &&
           signatures_.find(normalize(callee.name())) == signatures_.end();
}

void FunctionEmitter::emitAssertCall(const ast::CallExpression& call)
{
    const auto& arguments = call.arguments();
    const std::string condition = emitExpression(*arguments[0]);
    const std::string message = arguments.size() == 2 ? emitExpression(*arguments[1]) : std::string();
    const std::size_t label = nextLabel_++;
    const std::string passed = "assert.ok" + std::to_string(label);
    const std::string failed = "assert.fail" + std::to_string(label);
    output_ << "  br i1 " << condition << ", label %" << passed << ", label %" << failed << "\n\n";
    output_ << failed << ":\n";
    if (message.empty()) {
        output_ << "  call void @__inox_arith_fault(i32 14)\n";
    } else {
        requireModuleLine(assertMessageHelper());
        output_ << "  call void @__inox_assert_message(ptr " << message << ")\n";
    }
    output_ << "  unreachable\n\n";
    output_ << passed << ":\n";
}

} // namespace inox::compiler::codegen::detail
