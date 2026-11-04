// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: TRANSITIONAL (F6 step 2, removed at step 5). Code
// generation still walks the AST; the types, symbols and constants of the
// nodes it meets come from the typed IR through the migration index
// (tir/MigrationIndex.h), never from the SemanticResult. A node without its
// TIR node is an internal error in every build; a Debug compiler also
// checks that each answer agrees with the analysis's record, and a
// disagreement is an internal error too, never a fallback.

#include "FunctionEmitter.h"

#include <string>

namespace inox::compiler::codegen::detail {

#ifndef NDEBUG
namespace {

[[noreturn]] void disagree(const char* what, const ast::Expression& node)
{
    std::string message = "internal error: typed IR migration: the ";
    message += what;
    message += " the typed IR gives is not the analysis's";
    if (node.position().known()) {
        message += " (line " + std::to_string(node.position().line) + ")";
    }
    throw tir::MigrationError(message);
}

} // namespace
#endif

// One call, as before step 2: typeOf is on codegen's protected paths
// (tools/stack_frames.py), and its frame stays the same.
TypeRef FunctionEmitter::typeOf(const ast::Expression& expression) const
{
    return migratedTypeOf(expression);
}

TypeRef FunctionEmitter::migratedTypeOf(const ast::Expression& expression) const
{
#ifndef NDEBUG
    if (!index_.typeAgrees(expression, semantics_.typeOf(expression))) {
        disagree("type", expression);
    }
#endif
    return index_.typeOf(expression);
}

const semantic::Symbol* FunctionEmitter::symbolOf(const ast::Expression& node) const
{
    const semantic::Symbol* symbol = index_.symbolOf(node);
#ifndef NDEBUG
    const semantic::Symbol* recorded = nullptr;
    if (node.kind() == ast::AstNodeKind::IdentifierExpression) {
        recorded = semantics_.symbolOf(static_cast<const ast::IdentifierExpression&>(node));
    } else if (node.kind() == ast::AstNodeKind::CallExpression) {
        recorded = semantics_.symbolOf(static_cast<const ast::CallExpression&>(node));
    }
    if (symbol != recorded) {
        disagree("symbol", node);
    }
#endif
    return symbol;
}

const semantic::ConstantValue* FunctionEmitter::resolvedConstant(const ast::IdentifierExpression& identifier) const
{
    const semantic::ConstantValue* constant = index_.constantOf(identifier);
#ifndef NDEBUG
    const semantic::Symbol* symbol = semantics_.symbolOf(identifier);
    const semantic::ConstantValue* recorded =
        symbol != nullptr && symbol->kind == semantic::SymbolKind::Constant ? semantics_.constantValueOf(*symbol)
                                                                            : nullptr;
    if (constant != recorded) {
        disagree("constant", identifier);
    }
#endif
    return constant;
}

TypeRef FunctionEmitter::declaredTypeOf(const ast::VarStatement& declaration) const
{
    const TypeRef type = index_.declaredType(declaration);
#ifndef NDEBUG
    if (type != semantics_.declaredType(declaration)) {
        throw tir::MigrationError("internal error: typed IR migration: the declared type of " + declaration.name() +
                                  " is not the analysis's");
    }
#endif
    return type;
}

} // namespace inox::compiler::codegen::detail
