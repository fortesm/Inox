// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// FunctionEmitter: the semantic facts of the AST nodes code generation
// walks (types, symbols, constants, declared types, the builtin a call
// is), already resolved,
// taken from the typed IR through the AST node map (tir/AstNodeMap.h),
// never from the SemanticResult. Only reading belongs here: no analysis. A node without its
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
    std::string message = "internal error: typed IR: the ";
    message += what;
    message += " the typed IR gives is not the analysis's";
    if (node.position().known()) {
        message += " (line " + std::to_string(node.position().line) + ")";
    }
    throw tir::InvariantError(message);
}

} // namespace
#endif

// One call: typeOf is on codegen's protected paths (tools/stack_frames.py),
// and its frame stays that of a call.
TypeRef FunctionEmitter::typeOf(const ast::Expression& expression) const
{
    return typeFromTir(expression);
}

TypeRef FunctionEmitter::typeFromTir(const ast::Expression& expression) const
{
#ifndef NDEBUG
    if (!astNodes_.typeAgrees(expression, semantics_.typeOf(expression))) {
        disagree("type", expression);
    }
#endif
    return astNodes_.typeOf(expression);
}

const semantic::Symbol* FunctionEmitter::symbolOf(const ast::Expression& node) const
{
    const semantic::Symbol* symbol = astNodes_.symbolOf(node);
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
    const semantic::ConstantValue* constant = astNodes_.constantOf(identifier);
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
    const TypeRef type = astNodes_.declaredType(declaration);
#ifndef NDEBUG
    if (type != semantics_.declaredType(declaration)) {
        throw tir::InvariantError("internal error: typed IR: the declared type of " + declaration.name() +
                                  " is not the analysis's");
    }
#endif
    return type;
}

semantic::BuiltinOp FunctionEmitter::builtinOf(const ast::Expression& call) const
{
    const semantic::BuiltinOp op = astNodes_.builtinOf(call);
#ifndef NDEBUG
    const semantic::CallMeaning* meaning = semantics_.callMeaning(call);
    const semantic::BuiltinOp recorded =
        meaning != nullptr && meaning->kind == semantic::CallKind::Builtin ? meaning->op : semantic::BuiltinOp::None;
    if (op != recorded) {
        disagree("builtin", call);
    }
#endif
    return op;
}

} // namespace inox::compiler::codegen::detail
