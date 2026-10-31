// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Unit tests of AST copies (OpenSpec change type-representation): a copy has
// the same shape as its original, shares no node with it, and passes every
// type name and expression through the rewriters.

// F3 (change routine-headers, r3.114): the parser builds the parameters and
// result of a routine header; later phases never read its tokens.

#include "UnitTest.h"

#include "compiler/ast/Ast.h"
#include "compiler/parser/Parser.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace inox::compiler::unit {

namespace {

using namespace ast;

constexpr std::string_view kProgram = R"(Type
    TPoint Struct
        FX Integer := 0
    ;

TPoint.Move(Self mut, ref mut Steps Integer, ref Limit Array[1..3] Integer) Integer Public :
    Return 0
;

Pick<T>(A T, B Std.Text.TName) T :
    Return A
;

Main :
    PutLn(1)
;
)";

const FunctionDeclaration& routine(const ModuleNode& module, std::string_view name)
{
    for (const auto& item : module.items()) {
        if (item->kind() == AstNodeKind::FunctionDeclaration &&
            static_cast<const FunctionDeclaration&>(*item).name() == name) {
            return static_cast<const FunctionDeclaration&>(*item);
        }
    }
    throw std::logic_error("no routine " + std::string(name));
}

void testHeaders()
{
    parser::Parser parser{kProgram};
    const auto module = parser.parseModule("HeaderTest");

    const FunctionDeclaration& move = routine(*module, "TPoint.Move");
    checkEqual(std::to_string(move.parameters().size()), "3", "Move has three parameters");
    if (move.parameters().size() == 3) {
        const ParameterDeclaration& self = move.parameters()[0];
        check(self.isReceiver && self.name == "Self" && self.typeName.empty() &&
                  self.mode == ParameterDeclaration::Mode::RefMut,
              "Self mut is a receiver passed as ref mut, with no written type");
        const ParameterDeclaration& steps = move.parameters()[1];
        check(!steps.isReceiver && steps.name == "Steps" && steps.typeName == "Integer" &&
                  steps.mode == ParameterDeclaration::Mode::RefMut,
              "ref mut Steps Integer");
        const ParameterDeclaration& limit = move.parameters()[2];
        checkEqual(limit.typeName, "Array[1..3] Integer", "an array type is one type name");
        check(limit.mode == ParameterDeclaration::Mode::Ref, "ref Limit is passed as ref");
    }
    checkEqual(move.resultTypeName(), "Integer", "Move returns Integer");
    check(move.isPublic(), "Public ends the header");

    const FunctionDeclaration& pick = routine(*module, "Pick");
    checkEqual(std::to_string(pick.parameters().size()), "2", "Pick has two parameters");
    if (pick.parameters().size() == 2) {
        check(!pick.parameters()[0].isReceiver && pick.parameters()[0].mode == ParameterDeclaration::Mode::Value,
              "a value parameter");
        checkEqual(pick.parameters()[1].typeName, "Std.Text.TName", "a qualified type is one type name");
    }
    checkEqual(pick.resultTypeName(), "T", "Pick returns its type parameter");

    const FunctionDeclaration& main = routine(*module, "Main");
    check(main.parameters().empty() && main.resultTypeName().empty(), "Main has no parameters and no result");
}

} // namespace

int runRoutineHeaderTests()
{
    const int before = failureCount();
    testHeaders();
    return failureCount() - before;
}

} // namespace inox::compiler::unit
