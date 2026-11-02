// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// F5 (change desugaring-out-of-parser): the parser records chained and
// compound assignments, `with` targets and loop contracts as written; phase
// 2b turns them into the normalized AST, numbered as the parser numbered
// them before F5 (`with` in pre-order, loops in post-order). The IR
// snapshot proves that every program compiles as before; these tests pin
// the forms and the numbering.

#include "UnitTest.h"

#include "compiler/ast/Ast.h"
#include "compiler/lowering/Desugar.h"
#include "compiler/parser/Parser.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace inox::compiler::unit {

namespace {

using namespace ast;

constexpr std::string_view kProgram = R"(Type
    TIn Struct
        FX Integer := 0
        FY Integer := 0
    ;
    TOut Struct
        FIn TIn
        FZ Integer := 0
    ;

Main :
    O TOut
    A := 0
    B := 0
    A := B := O.FZ := 1
    A += 2
    with O
        .FZ -= 1
        with .FIn
            .FX := .FY := 3
        ;
    ;
    with O.FIn
        .FX ^= 2
    ;
    I := 0
    while I < 3
        invariant I <= 3
        variant 3 - I
        J := 0
        while J < 2
            variant 2 - J
            J += 1
        ;
        I += 1
    ;
    for K in 1..2
        invariant K >= 1
        PutLn(K)
    ;
;
)";

std::unique_ptr<ModuleNode> parse()
{
    parser::Parser parser{kProgram};
    return parser.parseModule("DesugarTest");
}

const FunctionDeclaration& mainOf(const ModuleNode& module)
{
    for (const auto& item : module.items()) {
        if (item->kind() == AstNodeKind::FunctionDeclaration) {
            return static_cast<const FunctionDeclaration&>(*item);
        }
    }
    throw std::logic_error("no Main");
}

// The text of a small expression: names, `__member(a, b)`, operators.
std::string text(const Expression& expression)
{
    switch (expression.kind()) {
    case AstNodeKind::IdentifierExpression:
        return static_cast<const IdentifierExpression&>(expression).name();
    case AstNodeKind::LiteralExpression:
        return static_cast<const LiteralExpression&>(expression).value();
    case AstNodeKind::WithTargetExpression:
        return "<with " + std::to_string(static_cast<const WithTargetExpression&>(expression).level()) + ">";
    case AstNodeKind::UnaryExpression:
        return "-" + text(static_cast<const UnaryExpression&>(expression).operand());
    case AstNodeKind::BinaryExpression: {
        const auto& binary = static_cast<const BinaryExpression&>(expression);
        return "(" + text(binary.left()) + " " + std::to_string(static_cast<int>(binary.op())) + " " +
               text(binary.right()) + ")";
    }
    case AstNodeKind::CallExpression: {
        const auto& call = static_cast<const CallExpression&>(expression);
        std::string result = text(call.callee()) + "(";
        for (std::size_t index = 0; index < call.arguments().size(); ++index) {
            result += (index == 0 ? "" : ", ") + text(*call.arguments()[index]);
        }
        return result + ")";
    }
    default:
        return "?";
    }
}

void testWrittenForms()
{
    const auto module = parse();
    const auto& body = mainOf(*module).body();
    int chains = 0;
    int compounds = 0;
    int withs = 0;
    int loopsWithContracts = 0;
    for (const auto& statement : body) {
        chains += statement->kind() == AstNodeKind::ChainedAssignmentStatement;
        compounds += statement->kind() == AstNodeKind::CompoundAssignmentStatement;
        if (statement->kind() == AstNodeKind::WithStatement) {
            ++withs;
            check(static_cast<const WithStatement&>(*statement).bindingName().empty(),
                  "the parser names no with binding");
        }
        if (statement->kind() == AstNodeKind::WhileStatement) {
            loopsWithContracts += !static_cast<const WhileStatement&>(*statement).contracts().empty();
        }
        if (statement->kind() == AstNodeKind::ForInStatement) {
            loopsWithContracts += !static_cast<const ForInStatement&>(*statement).contracts().empty();
        }
    }
    check(chains == 1 && compounds == 1 && withs == 2 && loopsWithContracts == 2,
          "the parser keeps the chain, the compound assignment, the withs and the loop contracts as written");
    const auto& firstWith = static_cast<const WithStatement&>(*body[5]);
    const auto& compound = static_cast<const CompoundAssignmentStatement&>(*firstWith.body()[0]);
    checkEqual(text(compound.target()), "__member(<with 0>, FZ)", "`.FZ` names the target of the outer with");
    const auto& inner = static_cast<const WithStatement&>(*firstWith.body()[1]);
    checkEqual(text(inner.target()), "__member(<with 0>, FIn)", "an inner with's target is read in the outer one");
    bool caught = false;
    try {
        lowering::requireDesugared(*module);
    } catch (const std::logic_error&) {
        caught = true;
    }
    check(caught, "the check finds the written forms before phase 2b");
}

void testLowered()
{
    const auto module = parse();
    lowering::desugarModule(*module);
    bool clean = true;
    try {
        lowering::requireDesugared(*module);
    } catch (const std::logic_error&) {
        clean = false;
    }
    check(clean, "no written form is left after phase 2b");

    const auto& body = mainOf(*module).body();
    std::string chain;
    for (std::size_t index = 3; index < 6; ++index) {
        chain += text(static_cast<const ExpressionStatement&>(*body[index]).expression()) + ";";
    }
    checkEqual(chain, "(__member(O, FZ) 0 1);(B 0 __member(O, FZ));(A 0 B);",
               "a chain stores right to left, each target from the one after it");
    const auto& compound = static_cast<const ExpressionStatement&>(*body[6]);
    check(compound.isCompoundAssignment(), "a compound assignment keeps its mark");
    checkEqual(text(compound.expression()), "(A 0 (A 1 2))", "`A += 2` is `A := A + 2`");

    const auto& outer = static_cast<const WithStatement&>(*body[7]);
    checkEqual(outer.bindingName(), "__with_0", "the first with is __with_0");
    checkEqual(text(static_cast<const ExpressionStatement&>(*outer.body()[0]).expression()),
               "(__member(__with_0, FZ) 0 (__member(__with_0, FZ) 2 1))", "`.FZ -= 1` inside it");
    const auto& inner = static_cast<const WithStatement&>(*outer.body()[1]);
    checkEqual(inner.bindingName(), "__with_1", "a nested with is numbered next (pre-order)");
    checkEqual(text(inner.target()), "__member(__with_0, FIn)", "its target reads the outer binding");
    checkEqual(text(static_cast<const ExpressionStatement&>(*inner.body()[0]).expression()),
               "(__member(__with_1, FY) 0 3)", "a chain inside the inner with uses its binding");
    checkEqual(static_cast<const WithStatement&>(*body[8]).bindingName(), "__with_2", "a later with is __with_2");

    // `__variant_previous_2 := -1` before the outer loop: the inner loop,
    // whose body ends first, is loop 1 (post-order).
    checkEqual(text(static_cast<const ExpressionStatement&>(*body[10]).expression()), "(__variant_previous_2 0 -1)",
               "the outer loop's variant starts before it, numbered after the inner loop");
    const auto& outerLoop = static_cast<const WhileStatement&>(*body[11]);
    check(outerLoop.contracts().empty(), "the lowered loop holds no contract");
    checkEqual(text(static_cast<const ExpressionStatement&>(*outerLoop.body()[0]).expression()),
               "__check(4, (I 19 3), loop invariant violated: Main: I <= 3)",
               "the invariant is checked first at the start of every iteration");
    checkEqual(text(static_cast<const ExpressionStatement&>(*body[12]).expression()),
               "__check(4, (I 19 3), loop invariant violated: Main: I <= 3 (at the end))",
               "a while's invariant is checked once more after it");
    const auto& forLoop = static_cast<const ForInStatement&>(*body[13]);
    check(body.size() == 14, "a for loop has no check after it");
    check(forLoop.body().size() == 2, "a for loop's invariant opens its body");
}

} // namespace

int runDesugarTests()
{
    const int before = failureCount();
    testWrittenForms();
    testLowered();
    return failureCount() - before;
}

} // namespace inox::compiler::unit
