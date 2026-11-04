// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The builtin enumeration the analysis and
// the typed IR share, name by name; and a program built into the typed IR,
// whose places carry their origin (a `with` binding is its statement, not
// a name) and whose calls their routine or operation.

#include "UnitTest.h"

#include "compiler/normalization/AstNormalizer.h"
#include "compiler/parser/Parser.h"
#include "compiler/semantic/CallMeaning.h"
#include "compiler/semantic/ContractLowering.h"
#include "compiler/semantic/ModuleResolution.h"
#include "compiler/semantic/SemanticAnalyzer.h"
#include "compiler/tir/Build.h"
#include "compiler/tir/Verify.h"

#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace inox::compiler::unit {

namespace {

using semantic::BuiltinOp;

struct Expected {
    std::string_view name;
    BuiltinOp op;
};

// Every builtin, written out: a builtin added to the enumeration without a
// name, or a name that changes, fails here.
constexpr Expected kBuiltins[] = {
    {"Put", BuiltinOp::Put}, {"PutLn", BuiltinOp::PutLn}, {"Get", BuiltinOp::Get}, {"GetLn", BuiltinOp::GetLn},
    {"ReadLn", BuiltinOp::ReadLn}, {"EndOfInput", BuiltinOp::EndOfInput}, {"Abs", BuiltinOp::Abs},
    {"Sqrt", BuiltinOp::Sqrt}, {"Cbrt", BuiltinOp::Cbrt}, {"Sin", BuiltinOp::Sin}, {"Cos", BuiltinOp::Cos},
    {"Tan", BuiltinOp::Tan}, {"ArcSin", BuiltinOp::ArcSin}, {"ArcCos", BuiltinOp::ArcCos},
    {"ArcTan", BuiltinOp::ArcTan}, {"ArcTan2", BuiltinOp::ArcTan2}, {"Sinh", BuiltinOp::Sinh},
    {"Cosh", BuiltinOp::Cosh}, {"Tanh", BuiltinOp::Tanh}, {"Exp", BuiltinOp::Exp}, {"Ln", BuiltinOp::Ln},
    {"LnXP1", BuiltinOp::LnXP1}, {"Log2", BuiltinOp::Log2}, {"Log10", BuiltinOp::Log10}, {"LogN", BuiltinOp::LogN},
    {"Power", BuiltinOp::Power}, {"Floor", BuiltinOp::Floor}, {"Ceil", BuiltinOp::Ceil}, {"FMod", BuiltinOp::FMod},
    {"Hypot", BuiltinOp::Hypot}, {"Hypot3", BuiltinOp::Hypot3}, {"RadToDeg", BuiltinOp::RadToDeg},
    {"DegToRad", BuiltinOp::DegToRad}, {"RadToGrad", BuiltinOp::RadToGrad}, {"GradToRad", BuiltinOp::GradToRad},
    {"RadToCycle", BuiltinOp::RadToCycle}, {"CycleToRad", BuiltinOp::CycleToRad}, {"Round", BuiltinOp::Round},
    {"Trunc", BuiltinOp::Trunc}, {"Low", BuiltinOp::Low}, {"High", BuiltinOp::High}, {"Length", BuiltinOp::Length},
    {"Ord", BuiltinOp::Ord}, {"Succ", BuiltinOp::Succ}, {"Pred", BuiltinOp::Pred}, {"Fill", BuiltinOp::Fill},
    {"New", BuiltinOp::New}, {"Free", BuiltinOp::Free}, {"Move", BuiltinOp::Move}, {"Value", BuiltinOp::Value},
    {"Link", BuiltinOp::Link}, {"Alive", BuiltinOp::Alive}, {"Same", BuiltinOp::Same}, {"Union", BuiltinOp::Union},
    {"Intersection", BuiltinOp::Intersection}, {"Difference", BuiltinOp::Difference},
    {"SymmetricDifference", BuiltinOp::SymmetricDifference}, {"Include", BuiltinOp::Include},
    {"Exclude", BuiltinOp::Exclude}, {"Assert", BuiltinOp::Assert},
};

constexpr Expected kFileFunctions[] = {
    {"OpenRead", BuiltinOp::OpenRead}, {"Create", BuiltinOp::Create}, {"OpenAppend", BuiltinOp::OpenAppend},
    {"ReadLn", BuiltinOp::FileReadLn}, {"Eof", BuiltinOp::Eof}, {"Close", BuiltinOp::Close},
    {"Write", BuiltinOp::Write}, {"WriteLn", BuiltinOp::WriteLn}, {"Exists", BuiltinOp::Exists},
    {"DeleteFile", BuiltinOp::DeleteFile}, {"RenameFile", BuiltinOp::RenameFile},
};

std::string lower(std::string_view text)
{
    std::string result(text);
    for (char& ch : result) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return result;
}

void testBuiltinNames()
{
    for (const Expected& builtin : kBuiltins) {
        const std::string name(builtin.name);
        check(semantic::builtinOpNamed(builtin.name) == builtin.op, "the builtin " + name);
        check(semantic::builtinOpNamed(lower(builtin.name)) == builtin.op, "the builtin " + name + " in lower case");
        checkEqual(std::string(semantic::builtinName(builtin.op)), name, "the name of the builtin " + name);
        check(semantic::builtinOpNamed(builtin.name, true) == BuiltinOp::None ||
                  builtin.op == BuiltinOp::ReadLn,
              name + " is not a file function");
    }
    for (const Expected& function : kFileFunctions) {
        const std::string name(function.name);
        check(semantic::builtinOpNamed(function.name, true) == function.op, "the file function " + name);
        checkEqual(std::string(semantic::builtinName(function.op)), name, "the name of the file function " + name);
    }
    const std::size_t count = sizeof kBuiltins / sizeof kBuiltins[0] + sizeof kFileFunctions / sizeof kFileFunctions[0];
    check(count == static_cast<std::size_t>(BuiltinOp::RenameFile), "every builtin is listed once");
    check(semantic::builtinOpNamed("Main") == BuiltinOp::None, "a name that is not a builtin");
    check(semantic::builtinOpNamed("__member") == BuiltinOp::None, "an encoding is not a builtin");
}

constexpr std::string_view kProgram = R"(Type
    TPoint Struct
        FX Integer := 0
        FY Integer := 0
    ;

TPoint.Move(Self mut, D Integer) :
    Self.FX += D
;

Main :
    P := TPoint(FX := 1)
    with P
        .FY := .FX
        .Move(2)
    ;
    PutLn(P.FY)
;
)";

// The index answers from the TIR node each AST node became, and never for
// a node it does not know.
void testMigrationIndex(const ast::ModuleNode& program, const semantic::SemanticResult& semantics,
                        const tir::AstNodeMap& index)
{
    const ast::FunctionDeclaration* main = semantics.entry();
    check(main != nullptr && main->body().size() == 3, "Main of the index test");
    if (main == nullptr || main->body().size() != 3) {
        return;
    }
    // `P := TPoint(FX := 1)`: the declared name, the construction, its field.
    const auto& declaration = static_cast<const ast::ExpressionStatement&>(*main->body()[0]);
    const auto& assignment = static_cast<const ast::BinaryExpression&>(declaration.expression());
    const auto& construction = static_cast<const ast::CallExpression&>(assignment.right());
    const types::Type* point = semantics.constructedStruct(construction);
    check(index.typeOf(assignment.left()) == point && index.symbolOf(assignment.left()) != nullptr &&
              index.symbolOf(assignment.left())->name == "P",
          "the declared name has the declared type and its symbol");
    check(index.typeOf(construction) == point, "a construction has its struct's type");
    check(index.typeOf(construction.callee()) == point, "the name of T(...) is the type T");
    check(index.typeOf(*construction.arguments()[0]) == semantics.types().int64Type(),
          "F := V of a construction has the field's type");
    // `PutLn(P.FY)`: a call without a result is Void; its name has no value.
    const auto& put = static_cast<const ast::CallExpression&>(
        static_cast<const ast::ExpressionStatement&>(*main->body()[2]).expression());
    check(index.typeOf(put) != nullptr && index.typeOf(put)->isVoid(), "a call without a result is Void");
    check(index.typeOf(put.callee()) == nullptr, "the name of a routine has no value");
    check(index.typeAgrees(put, semantics.typeOf(put)), "Void agrees with the analysis's absent type");
    check(index.builtinOf(put) == BuiltinOp::PutLn, "PutLn is the builtin the analysis decided");
    check(index.builtinOf(construction) == BuiltinOp::None, "a construction is no builtin");
    // A node the builder never met is an error, not an answer.
    const ast::IdentifierExpression stranger("Stranger");
    bool refused = false;
    try {
        (void)index.typeOf(stranger);
    } catch (const tir::InvariantError&) {
        refused = true;
    }
    check(refused, "a node without its TIR node is an internal error");
    (void)program;
}

void testBuiltProgram()
{
    parser::Parser parser{kProgram};
    auto module = parser.parseModule("Unit");
    normalization::normalizeModule(*module);
    std::vector<std::unique_ptr<ast::ModuleNode>> modules;
    modules.push_back(std::move(module));
    auto program = semantic::resolveModules(std::move(modules));
    semantic::lowerContracts(*program);
    semantic::SemanticAnalyzer analyzer;
    const semantic::SemanticResult& semantics = analyzer.analyze(*program);
    tir::AstNodeMap index;
    const auto typedIr = tir::buildProgram(*program, semantics, &index);
    checkEqual(tir::verifyProgram(*typedIr), "", "the typed IR verifies");
    testMigrationIndex(*program, semantics, index);
    check(typedIr->routines.size() == 2 && typedIr->entry == typedIr->routines[1], "Main is the entry, last");

    const tir::Routine& move = *typedIr->routines[0];
    check(move.parameters.size() == 2 && move.parameters[0].isReceiver &&
              move.parameters[0].mode == semantic::PassMode::RefMut,
          "Self mut is the receiver, passed as ref mut");

    const tir::Routine& main = *typedIr->entry;
    check(main.body.size() == 3, "Main has three statements");
    const tir::Stmt& with = *main.body[1];
    check(with.kind == tir::StmtKind::With, "the second statement is a with");
    const tir::Stmt& assign = *with.body[0];
    check(assign.kind == tir::StmtKind::Assign && assign.target->kind == tir::PlaceKind::Field &&
              assign.target->base->kind == tir::PlaceKind::WithBinding && assign.target->base->with == &with,
          "a write through the with binding is a field of the with's place");
    check(assign.target->field == 1, "FY is field 1");
    const tir::Expr& call = *with.body[1]->value;
    check(call.kind == tir::ExprKind::Call && call.routine == &move && call.operands.size() == 2,
          "the method call is a call of TPoint.Move with its receiver first");
    const tir::Expr& put = *main.body[2]->value;
    check(put.kind == tir::ExprKind::Builtin && put.op == BuiltinOp::PutLn, "PutLn is an operation, not a name");

    std::ostringstream printed;
    tir::printProgram(*typedIr, printed);
    check(printed.str().find("(field (with __with_0) FY) := (read Int64 (field (with __with_0) FX))") !=
              std::string::npos,
          "the printer writes the place of the binding");
}

// Negative tests: null routines and Construct types are reported, not dereferenced.
void testMalformedTir()
{
    // Keep the surrounding routine valid, so a different error cannot hide
    // the defect under test (the verifier returns its first diagnostic).
    ast::FunctionDeclaration declaration("Main", {}, {});
    semantic::FunctionSignature signature;
    signature.name = "Main";
    signature.declaration = &declaration;

    tir::Program program;
    auto entry = program.arena.make<tir::Routine>();
    entry->name = "Main";
    entry->origin = &declaration;
    entry->signature = &signature;
    entry->isEntry = true;
    program.entry = entry;
    auto routines = program.arena.array<tir::Routine*>(2);
    routines[0] = nullptr;
    routines[1] = entry;
    program.routines = routines;
    std::string diagnostic = tir::verifyProgram(program);
    check(diagnostic.find("a missing routine") != std::string::npos,
          "a null routine is diagnosed even when the program has an entry");

    tir::Program program2;
    auto expr = program2.arena.make<tir::Expr>();
    expr->kind = tir::ExprKind::Construct;
    expr->type = nullptr;
    auto routine = program2.arena.make<tir::Routine>();
    routine->name = "Main";
    routine->origin = &declaration;
    routine->signature = &signature;
    routine->isEntry = true;
    auto body = program2.arena.array<const tir::Stmt*>(1);
    auto stmt = program2.arena.make<tir::Stmt>();
    stmt->kind = tir::StmtKind::Evaluate;
    stmt->value = expr;
    body[0] = stmt;
    routine->body = body;
    auto routines2 = program2.arena.array<tir::Routine*>(1);
    routines2[0] = routine;
    program2.routines = routines2;
    program2.entry = routine;
    diagnostic = tir::verifyProgram(program2);
    check(diagnostic.find("an expression without a type: construct") != std::string::npos,
          "a Construct with null type is diagnosed without a crash");
}

// The Debug agreement accepts only the two differences the TIR makes on
// purpose; anything else, and a node the analysis typed otherwise, fails.
void testTypeAgreement()
{
    types::TypeContext types;
    tir::Program program;
    tir::AstNodeMap index;
    const auto make = [&](tir::ExprKind kind, const types::Type* type) {
        tir::Expr* expression = program.arena.make<tir::Expr>();
        expression->kind = kind;
        expression->type = type;
        return expression;
    };
    const ast::IdentifierExpression call("Report");
    index.recordValue(call, make(tir::ExprKind::Call, types.voidType()));
    check(index.typeAgrees(call, nullptr), "a call without a result is Void");
    check(!index.typeAgrees(call, types.int64Type()), "a call the analysis typed agrees only with that type");

    const ast::IdentifierExpression literal("Literal");
    index.recordValue(literal, make(tir::ExprKind::Literal, types.voidType()));
    check(!index.typeAgrees(literal, nullptr), "Void stands for no type only on an operation");

    const ast::IdentifierExpression value("Value");
    index.recordValue(value, make(tir::ExprKind::Read, types.int64Type()));
    check(!index.typeAgrees(value, nullptr), "a value the analysis did not type disagrees");
    check(!index.typeAgrees(value, types.stringType()), "a value of another type disagrees");

    const ast::IdentifierExpression name("__member");
    index.recordName(name);
    check(!index.typeAgrees(name, types.int64Type()), "a name has no type, whatever the analysis recorded");
    check(index.typeAgrees(name, nullptr), "a name agrees with no type");

    tir::Place* field = program.arena.make<tir::Place>();
    field->kind = tir::PlaceKind::Field;
    field->type = types.int64Type();
    const ast::IdentifierExpression target("Target");
    index.recordPlace(target, field);
    check(index.typeAgrees(target, nullptr), "a written field the analysis did not type has its place's type");
    check(!index.typeAgrees(target, types.stringType()), "a written field of another type disagrees");

    tir::Place* local = program.arena.make<tir::Place>();
    local->kind = tir::PlaceKind::Local;
    local->type = types.int64Type();
    const ast::IdentifierExpression variable("Variable");
    index.recordPlace(variable, local);
    check(!index.typeAgrees(variable, nullptr), "a variable place the analysis did not type disagrees");
}

// The builtin a call is comes from its TIR node: a Builtin's operation, Value
// for the object a pointer reaches; any other node is no builtin, and a node
// the builder never met is an error.
void testBuiltinOf()
{
    types::TypeContext types;
    tir::Program program;
    tir::AstNodeMap index;
    tir::Expr* sqrt = program.arena.make<tir::Expr>();
    sqrt->kind = tir::ExprKind::Builtin;
    sqrt->op = BuiltinOp::Sqrt;
    sqrt->type = types.float64Type();
    const ast::IdentifierExpression builtin("Sqrt");
    index.recordValue(builtin, sqrt);
    check(index.builtinOf(builtin) == BuiltinOp::Sqrt, "a Builtin is its operation");

    tir::Place* pointer = program.arena.make<tir::Place>();
    pointer->kind = tir::PlaceKind::Local;
    tir::Place* target = program.arena.make<tir::Place>();
    target->kind = tir::PlaceKind::Through;
    target->base = pointer;
    const ast::IdentifierExpression value("Value");
    index.recordPlace(value, target);
    check(index.builtinOf(value) == BuiltinOp::Value, "the object a pointer reaches is Value(P)");
    const ast::IdentifierExpression variable("P");
    index.recordPlace(variable, pointer);
    check(index.builtinOf(variable) == BuiltinOp::None, "a variable is no builtin");

    const ast::IdentifierExpression stranger("Stranger");
    bool refused = false;
    try {
        (void)index.builtinOf(stranger);
    } catch (const tir::InvariantError&) {
        refused = true;
    }
    check(refused, "the builtin of a node without its TIR node is an internal error");
}

// Valid Inox the typed IR cannot express yet is a gap, not an internal error.
void testUnsupportedConstruct()
{
    parser::Parser parser{std::string_view("Const Mask := UInt8($FF)\n\nMain :\n    V UInt8 := Mask\n    PutLn(V)\n;\n")};
    auto module = parser.parseModule("Gap");
    normalization::normalizeModule(*module);
    std::vector<std::unique_ptr<ast::ModuleNode>> modules;
    modules.push_back(std::move(module));
    auto program = semantic::resolveModules(std::move(modules));
    semantic::lowerContracts(*program);
    semantic::SemanticAnalyzer analyzer;
    const semantic::SemanticResult& semantics = analyzer.analyze(*program);
    bool gap = false;
    try {
        (void)tir::buildProgram(*program, semantics);
    } catch (const tir::UnsupportedConstructError&) {
        gap = true;
    } catch (const tir::InvariantError&) {
        gap = false;
    }
    check(gap, "a Const without its value is a gap of the backend, not an internal error");
}

} // namespace

int runTypedIrTests()
{
    const int before = failureCount();
    testBuiltinNames();
    testBuiltProgram();
    testMalformedTir();
    testTypeAgreement();
    testBuiltinOf();
    testUnsupportedConstruct();
    return failureCount() - before;
}

} // namespace inox::compiler::unit
