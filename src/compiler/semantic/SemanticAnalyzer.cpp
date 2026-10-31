// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// SemanticAnalyzer: construction, the entry point, errors.

#include "SemanticAnalyzer.h"

#include "SemanticSupport.h"
#include "../exceptions/ExceptionTypes.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

namespace inox::compiler::semantic {

using namespace detail;

namespace {

// r3.99: internal names of routines of other modules join the module with
// `::` (`Std.Math::Max`); a message shows `Std.Math.Max`. Inox source never
// writes `::`.
std::string shownMessage(std::string message)
{
    for (std::size_t at = message.find("::"); at != std::string::npos; at = message.find("::", at)) {
        message.replace(at, 2, ".");
    }
    return message;
}

} // namespace

SemanticError::SemanticError(std::string message)
    : std::runtime_error(shownMessage(std::move(message)))
{
}

SemanticAnalyzer::SemanticAnalyzer()
{
    pointerTargetRoot_.name = "<the object of a pointer>";
    pointerTargetRoot_.kind = SymbolKind::Variable;
    pointerTargetRoot_.isMutable = true;
    declareBuiltins();
    declareBuiltinTypes();
}

// A diagnostic says where it was found (E14, change source-positions): the
// start of the statement or declaration being checked, in its module.
const SemanticResult& SemanticAnalyzer::analyze(const ast::ModuleNode& module)
{
    try {
        return analyzeProgram(module);
    } catch (SemanticError& error) {
        if (!error.position().known() && errorPosition_.known()) {
            error.locate(errorPosition_, errorModule_);
        }
        throw;
    }
}

// A position inside the item being checked: its module stays the item's.
void SemanticAnalyzer::notePosition(const ast::SourcePosition& position)
{
    if (position.known()) {
        errorPosition_ = position;
    }
}

// A module item (a routine, a section): its position and its module.
void SemanticAnalyzer::notePosition(const ast::AstNode& item)
{
    errorPosition_ = item.position();
    switch (item.kind()) {
    case ast::AstNodeKind::FunctionDeclaration:
        errorModule_ = static_cast<const ast::FunctionDeclaration&>(item).module();
        break;
    case ast::AstNodeKind::SectionDeclaration:
        errorModule_ = static_cast<const ast::SectionDeclaration&>(item).module();
        break;
    default:
        errorModule_ = currentModule_;
        break;
    }
    if (errorModule_.empty()) {
        errorModule_ = program_ != nullptr ? program_->name() : std::string{};
    }
}

const SemanticResult& SemanticAnalyzer::analyzeProgram(const ast::ModuleNode& module)
{
    result_.clear();
    program_ = &module;

    if (module.name().empty()) {
        throw SemanticError("module must have a name");
    }
    declareStandardModuleNames(module);

    registerGenericTemplates(module);
    for (const auto& item : module.items()) {
        declareModuleItem(*item);
    }
    resolveDeclaredTypeNames();

    registerFunctionSignatures(module);

    errorPosition_ = {};  // the errors of the whole program have no position
    if (!hasMain_) {
        throw SemanticError("module must declare Main");
    }

    // Sections first: Const values must be known before any routine uses one
    // as a static value (ADR-0011), wherever the Const appears in the file.
    for (const auto& item : module.items()) {
        if (item->kind() == ast::AstNodeKind::SectionDeclaration) {
            analyzeModuleItem(*item);
        }
    }
    for (const auto& item : module.items()) {
        if (item->kind() != ast::AstNodeKind::SectionDeclaration) {
            analyzeModuleItem(*item);
        }
    }
    // CANON-8: the instances of generic routines the program uses.
    analyzePendingInstances();

    return result_;
}

const SemanticResult& SemanticAnalyzer::result() const
{
    return result_;
}

} // namespace inox::compiler::semantic
