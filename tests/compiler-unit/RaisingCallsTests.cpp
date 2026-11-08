// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The calls an exception may leave (CANON-25, r3.136): phase 5's
// classification (callMayRaise) against the generated code's ground truth,
// the runtime functions declared without `nounwind`. A new raising runtime
// function fails here until the call that reaches it is classified.

#include "UnitTest.h"

#include "compiler/codegen/RuntimeAbi.h"
#include "compiler/semantic/CallMeaning.h"

#include <set>
#include <string>
#include <string_view>

namespace inox::compiler::unit {

namespace {

using codegen::detail::RuntimeFunction;
using semantic::BuiltinOp;
using semantic::CallKind;

const std::set<BuiltinOp>& raisingBuiltins()
{
    static const std::set<BuiltinOp> builtins = {
        BuiltinOp::OpenRead, BuiltinOp::Create, BuiltinOp::OpenAppend, BuiltinOp::FileReadLn, BuiltinOp::Eof,
        BuiltinOp::Close,    BuiltinOp::Write,  BuiltinOp::WriteLn,    BuiltinOp::DeleteFile, BuiltinOp::RenameFile,
    };
    return builtins;
}

void testBuiltins()
{
    // BuiltinOp::RenameFile is the last enumerator; CallMeaning.cpp's switch
    // rejects a new one at compile time.
    for (int value = 0; value <= static_cast<int>(BuiltinOp::RenameFile); ++value) {
        const auto op = static_cast<BuiltinOp>(value);
        check(semantic::callMayRaise(CallKind::Builtin, op, false) == raisingBuiltins().contains(op),
              "builtin " + std::string(semantic::builtinName(op)) + " classified as the runtime says");
    }
}

void testCallKinds()
{
    check(semantic::callMayRaise(CallKind::Routine, BuiltinOp::None, false), "a routine may raise");
    check(semantic::callMayRaise(CallKind::Method, BuiltinOp::None, false), "a method may raise");
    check(semantic::callMayRaise(CallKind::MethodNoArguments, BuiltinOp::None, false), "a method may raise");
    check(semantic::callMayRaise(CallKind::Conversion, BuiltinOp::None, true), "text to a number may raise");
    check(!semantic::callMayRaise(CallKind::Conversion, BuiltinOp::None, false), "a number conversion traps");
    for (const CallKind kind : {CallKind::Field, CallKind::Index, CallKind::Slice, CallKind::SetLiteral,
                                CallKind::Check, CallKind::Pure, CallKind::Construction, CallKind::StringFunction,
                                CallKind::Intrinsic}) {
        check(!semantic::callMayRaise(kind, BuiltinOp::None, false),
              std::string(semantic::callKindName(kind)) + " traps or cannot fail");
    }
}

// The raising runtime functions and the phase 5 call that reaches each.
bool classifiedByPhase5(std::string_view declaration)
{
    const auto named = [&](std::string_view name) { return declaration.find(name) != std::string_view::npos; };
    if (named("@__inox_raise(") || named("@__inox_exception_rethrow(")) {
        return true;  // Raise, bare Raise and a spent Retry: statements, not calls
    }
    if (named("@__inox_str_to_")) {
        return semantic::callMayRaise(CallKind::Conversion, BuiltinOp::None, true);
    }
    if (named("@__inox_file_open(")) {
        return semantic::callMayRaise(CallKind::Builtin, BuiltinOp::OpenRead, false) &&
               semantic::callMayRaise(CallKind::Builtin, BuiltinOp::Create, false) &&
               semantic::callMayRaise(CallKind::Builtin, BuiltinOp::OpenAppend, false);
    }
    if (named("@__inox_file_readln(")) {
        return semantic::callMayRaise(CallKind::Builtin, BuiltinOp::FileReadLn, false);
    }
    if (named("@__inox_file_eof(")) {
        return semantic::callMayRaise(CallKind::Builtin, BuiltinOp::Eof, false);
    }
    if (named("@__inox_file_close(")) {
        return semantic::callMayRaise(CallKind::Builtin, BuiltinOp::Close, false);
    }
    if (named("@__inox_file_write_")) {
        return semantic::callMayRaise(CallKind::Builtin, BuiltinOp::Write, false) &&
               semantic::callMayRaise(CallKind::Builtin, BuiltinOp::WriteLn, false);
    }
    if (named("@__inox_file_delete(")) {
        return semantic::callMayRaise(CallKind::Builtin, BuiltinOp::DeleteFile, false);
    }
    if (named("@__inox_file_rename(")) {
        return semantic::callMayRaise(CallKind::Builtin, BuiltinOp::RenameFile, false);
    }
    return false;
}

void testRuntimeFunctions()
{
    int raising = 0;
    for (int value = 0;; ++value) {
        const std::string_view declaration = codegen::detail::runtimeDeclaration(static_cast<RuntimeFunction>(value));
        if (declaration.empty()) {
            break;
        }
        if (declaration.find("nounwind") != std::string_view::npos) {
            continue;
        }
        ++raising;
        check(classifiedByPhase5(declaration),
              "the raising runtime function is reached by a call phase 5 classifies: " + std::string(declaration));
    }
    check(raising == 21, "21 runtime functions may raise (raise, rethrow, 5 conversions, 14 file operations)");
}

} // namespace

int runRaisingCallsTests()
{
    const int before = failureCount();
    testBuiltins();
    testCallKinds();
    testRuntimeFunctions();
    return failureCount() - before;
}

} // namespace inox::compiler::unit
