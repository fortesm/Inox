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
#include "../support/ArrayType.h"

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

SemanticError::SemanticError(std::string message)
    : std::runtime_error(std::move(message))
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

const SemanticResult& SemanticAnalyzer::analyze(const ast::ModuleNode& module)
{
    result_.clear();

    if (module.name().empty()) {
        throw SemanticError("module must have a name");
    }
    declareStandardModuleNames(module);

    for (const auto& item : module.items()) {
        declareModuleItem(*item);
    }

    registerFunctionSignatures(module);

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

    return result_;
}

const SemanticResult& SemanticAnalyzer::result() const
{
    return result_;
}

} // namespace inox::compiler::semantic
