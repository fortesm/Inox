// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: design by contract (CANON-24; OpenSpec change
// design-by-contract). The contract lowering pass and the parser turn every
// clause into `__check(Kind, Condition, "text")` and every `old X` or loop
// variant into `__pure(X)`. Here those intrinsics are checked, a contract is
// kept read-only, and the analyzer records where a struct invariant must
// hold, as Eiffel checks a class invariant:
//   - before and after a qualified method call (the receiver is not the bare
//     `Self`), on every struct of the receiver's path that has an invariant;
//     a `Self` method only reads, so it is checked before only;
//   - after a field or element is written from outside the struct's own
//     methods (`P.FX := 1`, also through `with`);
//   - after a call that changes a ref mut argument;
//   - on a value just built by construction or by a declaration's defaults.
// A struct's invariant includes the invariants of the structs it reuses
// (Eiffel: the parents' invariants apply to the class). Calls inside a
// contract and inside an invariant are not checked again, as in Eiffel
// (no assertion monitoring while an assertion is evaluated).

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"

#include <string>

namespace inox::compiler::semantic {

using namespace detail;

namespace {

class ContractScope {
public:
    explicit ContractScope(bool& flag) : flag_(flag), saved_(flag) { flag_ = true; }
    ~ContractScope() { flag_ = saved_; }
    ContractScope(const ContractScope&) = delete;
    ContractScope& operator=(const ContractScope&) = delete;

private:
    bool& flag_;
    bool saved_;
};

} // namespace

TypeRef SemanticAnalyzer::analyzeContractCheck(const ast::CallExpression& call)
{
    const auto& arguments = call.arguments();
    if (arguments.size() != 3 || arguments[0]->kind() != ast::AstNodeKind::LiteralExpression ||
        arguments[2]->kind() != ast::AstNodeKind::LiteralExpression) {
        throw SemanticError("internal error: malformed contract check");
    }
    TypeRef condition = nullptr;
    {
        ContractScope scope(inContract_);
        condition = analyzeExpression(*arguments[1]);
    }
    if (condition == nullptr || !condition->isBool()) {
        const auto& text = static_cast<const ast::LiteralExpression&>(*arguments[2]).value();
        const std::size_t where = text.find(": ");
        throw SemanticError("a contract condition is Bool (CANON-24): " +
                            (where == std::string::npos ? text : text.substr(where + 2)) + " is " +
                            types::nameOf(condition));
    }
    analyzeExpression(*arguments[0]);
    analyzeExpression(*arguments[2]);
    return types().voidType();
}

TypeRef SemanticAnalyzer::analyzePureCall(const ast::CallExpression& call)
{
    if (call.arguments().size() != 1) {
        throw SemanticError("internal error: malformed contract value");
    }
    ContractScope scope(inContract_);
    const TypeRef type = analyzeExpression(*call.arguments()[0]);
    if (isFileType(type)) {
        throw SemanticError("'old' of a File would copy it (CANON-24)");
    }
    if (isMoveOnlyType(type)) {
        throw SemanticError("'old' of an owner would copy it: read what it holds, old P.Field (CANON-24)");
    }
    return type;
}

void SemanticAnalyzer::rejectInContract(const std::string& what) const
{
    if (inContract_) {
        throw SemanticError("a contract only reads (CANON-24): " + what + " would change the program's state");
    }
}

bool SemanticAnalyzer::hasInvariant(TypeRef type) const
{
    if (type == nullptr || !type->isStruct()) {
        return false;
    }
    if (resolveFunctionSignature(type->name() + ".__invariant") != nullptr) {
        return true;
    }
    for (const types::StructField& field : type->fields()) {
        if (field.isPart && hasInvariant(field.type)) {
            return true;
        }
    }
    return false;
}

// No invariant checks inside a contract or inside an invariant.
bool SemanticAnalyzer::invariantChecksOn() const
{
    if (inContract_) {
        return false;
    }
    const std::size_t dot = currentRoutine_.find('.');
    return dot == std::string::npos || !equalsIgnoreCase(currentRoutine_.substr(dot + 1), "__invariant");
}

// Every struct with an invariant on the path of `place`, from the place to
// its root, except a bare `Self` (an unqualified call or a struct's own
// write, which may break the invariant until the method returns).
void SemanticAnalyzer::recordInvariantPlaces(const ast::AstNode& site, const ast::Expression& place, bool before,
                                             bool after)
{
    if (!invariantChecksOn()) {
        return;
    }
    for (const ast::Expression* current = &place;;) {
        const bool bareSelf = current->kind() == ast::AstNodeKind::IdentifierExpression &&
                              equalsIgnoreCase(static_cast<const ast::IdentifierExpression&>(*current).name(), "Self");
        const TypeRef type = result_.typeOf(*current);
        if (!bareSelf && hasInvariant(type)) {
            if (!isSideEffectFree(*current)) {
                throw SemanticError("the invariant of " + type->name() + " is checked on this place, so it must be "
                                    "computed without side effects (CANON-24): store the index in a variable first");
            }
            if (before) {
                result_.addInvariantCheck(site, false, {current, type});
            }
            if (after) {
                result_.addInvariantCheck(site, true, {current, type});
            }
        }
        if (current->kind() != ast::AstNodeKind::CallExpression) {
            break;
        }
        const auto& call = static_cast<const ast::CallExpression&>(*current);
        if (call.callee().kind() != ast::AstNodeKind::IdentifierExpression || call.arguments().empty()) {
            break;
        }
        const std::string& name = static_cast<const ast::IdentifierExpression&>(call.callee()).name();
        if (name != "__member" && name != "__index") {
            break;
        }
        current = call.arguments()[0].get();
    }
}

} // namespace inox::compiler::semantic
