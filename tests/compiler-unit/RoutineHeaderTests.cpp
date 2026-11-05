// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Unit tests of AST copies (OpenSpec change type-representation): a copy has
// the same shape as its original, shares no node with it, and passes every
// type name and expression through the rewriters.

// The parser builds the parameters and (change routine-headers, r3.114)
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
        check(self.isReceiver && self.name == "Self" && !self.type &&
                  self.mode == ParameterDeclaration::Mode::RefMut,
              "Self mut is a receiver passed as ref mut, with no written type");
        const ParameterDeclaration& steps = move.parameters()[1];
        check(!steps.isReceiver && steps.name == "Steps" && steps.type && spell(*steps.type) == "Integer" &&
                  steps.mode == ParameterDeclaration::Mode::RefMut,
              "ref mut Steps Integer");
        const ParameterDeclaration& limit = move.parameters()[2];
        checkEqual(spell(*limit.type), "Array[1..3] Integer", "an array type, spelled");
        check(limit.type->isArray() && limit.type->element().name == "Integer", "an array type is a tree");
        check(limit.mode == ParameterDeclaration::Mode::Ref, "ref Limit is passed as ref");
    }
    checkEqual(spell(*move.resultType()), "Integer", "Move returns Integer");
    check(move.isPublic(), "Public ends the header");

    const FunctionDeclaration& pick = routine(*module, "Pick");
    checkEqual(std::to_string(pick.parameters().size()), "2", "Pick has two parameters");
    if (pick.parameters().size() == 2) {
        check(!pick.parameters()[0].isReceiver && pick.parameters()[0].mode == ParameterDeclaration::Mode::Value,
              "a value parameter");
        checkEqual(pick.parameters()[1].type->name, "Std.Text.TName", "a qualified type is one Named type");
    }
    checkEqual(spell(*pick.resultType()), "T", "Pick returns its type parameter");

    const FunctionDeclaration& main = routine(*module, "Main");
    check(main.parameters().empty() && !main.resultType(), "Main has no parameters and no result");
}

// The dot of a method name: the last one outside angle brackets and after
// any `::` (generic-struct-methods).
void testMethodDot()
{
    const auto dotOf = [](std::string_view name) {
        const std::size_t dot = ast::methodDot(name);
        return dot == std::string_view::npos ? std::string("none") : std::string(name.substr(0, dot));
    };
    checkEqual(dotOf("Main"), "none", "a routine");
    checkEqual(dotOf("TPoint.Move"), "TPoint", "a method");
    checkEqual(dotOf("Bank.Accounts.TAccount.Deposit"), "Bank.Accounts.TAccount", "a method of a module's type");
    checkEqual(dotOf("Std.Math::Max"), "none", "a routine of another module");
    checkEqual(dotOf("Bank::TAccount.Deposit"), "Bank::TAccount", "a method named through its module");
    checkEqual(dotOf("Max<Array[1..3] Int64>"), "none", "an instance of a generic routine");
    checkEqual(dotOf("Max<Bank.TKey>"), "none", "a type argument of a module");
    checkEqual(dotOf("TPair<Bank.TKey, Int64>.Swap"), "TPair<Bank.TKey, Int64>", "an instance's method");
    checkEqual(dotOf("TStack<TPair<String, Integer>>.Push"), "TStack<TPair<String, Integer>>",
               "a method of a nested instance");
}

} // namespace

int runRoutineHeaderTests()
{
    const int before = failureCount();
    testHeaders();
    testMethodDot();
    return failureCount() - before;
}

} // namespace inox::compiler::unit
