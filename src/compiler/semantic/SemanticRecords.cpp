// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: what the analysis decided, recorded for the typed IR
// (tir/): the meaning of each call, the origin and the symbol
// of each declaration. Out of line, so the recursive analyzers keep their
// frames: each helper takes only references.

#include "SemanticAnalyzer.h"

#include <cstdint>

namespace inox::compiler::semantic {

void SemanticAnalyzer::recordCallMeaning(const ast::Expression& call, CallKind kind)
{
    CallMeaning meaning;
    meaning.kind = kind;
    result_.setCallMeaning(call, meaning);
}

void SemanticAnalyzer::recordFieldAccess(const ast::Expression& access, TypeRef structType, const types::StructField& field)
{
    CallMeaning meaning;
    meaning.kind = CallKind::Field;
    meaning.field = static_cast<std::uint32_t>(&field - structType->fields().data());
    result_.setCallMeaning(access, meaning);
}

void SemanticAnalyzer::recordBuiltinCall(const ast::CallExpression& call, bool file)
{
    CallMeaning meaning;
    meaning.kind = CallKind::Builtin;
    meaning.op = builtinOpNamed(static_cast<const ast::IdentifierExpression&>(call.callee()).name(), file);
    // CANON-13: Low, High and Length of a fixed array, and Low and High of
    // an enum or Range type, are constants of the type; the other folded
    // values (Ord, Succ, Pred of a constant) stay operations.
    const bool ofType = meaning.op == BuiltinOp::Low || meaning.op == BuiltinOp::High || meaning.op == BuiltinOp::Length;
    if (const auto constant = constants_.find(&call); ofType && constant != constants_.end()) {
        meaning.constant = constant->second;
    }
    result_.setCallMeaning(call, meaning);
}

void SemanticAnalyzer::recordBuiltinIdentifier(const ast::IdentifierExpression& identifier)
{
    CallMeaning meaning;
    meaning.kind = CallKind::Builtin;
    meaning.op = builtinOpNamed(identifier.name());
    result_.setCallMeaning(identifier, meaning);
}

void SemanticAnalyzer::recordRoutineCall(const ast::Expression& call, const FunctionSignature& signature, CallKind kind)
{
    CallMeaning meaning;
    meaning.kind = kind;
    meaning.routine = &signature;
    result_.setCallMeaning(call, meaning);
}

void SemanticAnalyzer::recordIntrinsicCall(const ast::Expression& call, const ModuleIntrinsic& intrinsic)
{
    CallMeaning meaning;
    meaning.kind = CallKind::Intrinsic;
    meaning.intrinsic = &intrinsic;
    result_.setCallMeaning(call, meaning);
}

void SemanticAnalyzer::recordDeclaredSymbol(const void* declaration, const std::string& name, VariableOrigin origin)
{
    if (origin != VariableOrigin::None) {
        symbols_.currentScope().setOrigin(name, origin);
    }
    if (const Symbol* symbol = symbols_.currentScope().resolve(name)) {
        result_.setDeclaredSymbol(declaration, *symbol);
    }
}

void SemanticAnalyzer::recordOmittedBound(const ast::CallExpression& slice, bool low)
{
    CallMeaning meaning = result_.callMeaning(slice) != nullptr ? *result_.callMeaning(slice) : CallMeaning{};
    meaning.kind = CallKind::Slice;
    (low ? meaning.lowOmitted : meaning.highOmitted) = true;
    result_.setCallMeaning(slice, meaning);
}

void SemanticAnalyzer::recordLoopIterator(const ast::ForInStatement& loop)
{
    recordDeclaredSymbol(&loop, loop.iterator(), VariableOrigin::None);
}

VariableOrigin SemanticAnalyzer::parameterOrigin(PassMode mode, bool receiver)
{
    if (receiver) {
        return mode == PassMode::RefMut ? VariableOrigin::MutReceiver : VariableOrigin::Receiver;
    }
    switch (mode) {
    case PassMode::Ref: return VariableOrigin::RefParameter;
    case PassMode::RefMut: return VariableOrigin::RefMutParameter;
    case PassMode::Value: break;
    }
    return VariableOrigin::Parameter;
}

} // namespace inox::compiler::semantic
