// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Unit tests of the LLVM layout of types (OpenSpec change type-representation).

#include "UnitTest.h"

#include "compiler/ast/Ast.h"
#include "compiler/codegen/TypeLayout.h"
#include "compiler/semantic/SemanticResult.h"
#include "compiler/types/TypeContext.h"

#include <string>
#include <vector>

namespace inox::compiler::unit {

namespace {

using codegen::detail::arrayLayout;
using codegen::detail::fieldLayout;
using codegen::detail::flattenArray;
using codegen::detail::llvmStorageType;
using codegen::detail::llvmValueType;
using codegen::detail::narrowIntegerRange;
using types::Type;
using types::TypeContext;

void testValueTypes()
{
    TypeContext context;
    checkEqual(llvmValueType(context.boolType()), "i1", "Bool is i1");
    checkEqual(llvmValueType(context.int64Type()), "i64", "Int64 is i64");
    checkEqual(llvmValueType(context.builtin("Int8")), "i64", "a narrow integer is computed as i64");
    checkEqual(llvmValueType(context.naturalType()), "i64", "Natural is i64");
    checkEqual(llvmValueType(context.float64Type()), "double", "Float64 is double");
    checkEqual(llvmValueType(context.builtin("Float32")), "float", "Float32 is float");
    checkEqual(llvmValueType(context.stringType()), "ptr", "String is a pointer");
    checkEqual(llvmValueType(context.fileType()), "ptr", "File is a pointer");
    checkEqual(llvmValueType(context.slice(context.int64Type())), "{ ptr, i64, i64 }", "a slice");
    checkEqual(llvmValueType(context.charType()), "i64", "a Char is its code point, an i64");
    checkEqual(llvmValueType(context.builtin("Currency")), "", "a decimal is not lowered");
    checkEqual(llvmValueType(nullptr), "", "an unknown type has no LLVM type");
}

void testStorage()
{
    TypeContext context;
    checkEqual(llvmStorageType(context.builtin("UInt8")), "i8", "UInt8 is stored in 8 bits");
    checkEqual(llvmStorageType(context.builtin("Int16")), "i16", "Int16 is stored in 16 bits");
    checkEqual(llvmStorageType(context.builtin("UInt32")), "i32", "UInt32 is stored in 32 bits");
    checkEqual(llvmStorageType(context.int64Type()), "i64", "Int64 storage");
    const auto range = narrowIntegerRange(context.builtin("Int8"));
    check(range && range->low == -128 && range->high == 127, "the range of Int8");
    check(!narrowIntegerRange(context.int64Type()), "Int64 is not narrow");
    check(!narrowIntegerRange(nullptr), "an unknown type is not narrow");
}

void testArrays()
{
    TypeContext context;
    const Type* bytes = context.array(1, 10, context.builtin("UInt8"));
    checkEqual(llvmValueType(bytes), "[10 x i8]", "an array of bytes keeps them in 8 bits");
    const auto layout = arrayLayout(bytes);
    check(layout && layout->low == 1 && layout->high == 10 && layout->elementStorage == "i8", "array layout");
    const Type* matrix = context.array(1, 3, context.array(0, 3, context.int64Type()));
    checkEqual(llvmValueType(matrix), "[3 x [4 x i64]]", "a nested array");
    const auto flat = flattenArray(matrix);
    check(flat.count == 12 && flat.element == context.int64Type(), "a nested array is one run of 12 values");
    check(!arrayLayout(context.int64Type()), "only an array has an array layout");
}

void testPointersAndStructs()
{
    TypeContext context;
    const Type* node = context.declareStruct("TNode");
    const Type* owner = context.own(node);
    context.setStructFields(node, {types::StructField{"FValue", context.builtin("Int16"), false, false, ""},
                                   types::StructField{"FName", context.stringType(), false, false, ""},
                                   types::StructField{"FNext", context.optionOwn(owner), false, false, ""},
                                   types::StructField{"FParent", context.link(node), false, false, ""}});
    checkEqual(llvmValueType(owner), "ptr", "an owner is its object's address");
    checkEqual(llvmValueType(context.optionOwn(owner)), "ptr", "an Option<Own> is a nullable address");
    checkEqual(llvmValueType(context.link(node)), "{ ptr, ptr, i64 }", "a Link carries header and generation");
    checkEqual(llvmValueType(node), "%tnode", "a struct is its named LLVM type");
    const auto value = fieldLayout(node, "fvalue");
    check(value && value->index == 0 && value->llvmType == "i64", "a narrow field is an i64 in its struct");
    const auto parent = fieldLayout(node, "FParent");
    check(parent && parent->index == 3 && parent->owner == node && parent->llvmType == "{ ptr, ptr, i64 }",
          "a field by name, case-insensitive, with its index");
    check(!fieldLayout(node, "FNothing"), "an unknown field");
    check(!fieldLayout(context.int64Type(), "FValue"), "only a struct has fields");
}

void testSignatures()
{
    TypeContext context;
    const Type* point = context.declareStruct("TPoint");
    context.setStructFields(point, {types::StructField{"FX", context.int64Type(), false, false, ""}});

    const ast::FunctionDeclaration moveBy("TPoint.Move", {}, {});
    semantic::FunctionSignature analyzed;
    analyzed.name = "TPoint.Move";
    analyzed.parameters = {{"Self", point, semantic::PassMode::RefMut},
                           {"Step", context.int64Type(), semantic::PassMode::Value},
                           {"Total", context.int64Type(), semantic::PassMode::RefMut}};
    const auto lowered = codegen::detail::lowerSignature(moveBy, analyzed);
    checkEqual(lowered.llvmName, "inox_tpoint.move", "the LLVM name of a method");
    checkEqual(lowered.llvmReturnType, "void", "a subroutine returns void");
    check(lowered.parameters.size() == 3, "three parameters");
    check(lowered.parameters[0].llvmType == "ptr" && !lowered.parameters[0].byReference,
          "a struct receiver is passed by address, not as a reference");
    check(lowered.parameters[1].llvmType == "i64" && !lowered.parameters[1].byReference, "a value parameter");
    check(lowered.parameters[2].llvmType == "ptr" && lowered.parameters[2].byReference &&
              lowered.parameters[2].valueType == "i64",
          "ref mut is a pointer to an i64");

    const ast::FunctionDeclaration twice("Integer.Twice", {}, {});
    semantic::FunctionSignature builtin;
    builtin.name = "Integer.Twice";
    builtin.parameters = {{"Self", context.int64Type(), semantic::PassMode::RefMut}};
    builtin.returnType = context.int64Type();
    const auto method = codegen::detail::lowerSignature(twice, builtin);
    check(method.parameters[0].byReference && method.parameters[0].valueType == "i64",
          "Self mut of a built-in type is the caller's variable");
    check(method.llvmReturnType == "i64" && method.returnType == context.int64Type(), "the result type");
}

} // namespace

int runTypeLayoutTests()
{
    const int before = failureCount();
    testValueTypes();
    testStorage();
    testArrays();
    testPointersAndStructs();
    testSignatures();
    return failureCount() - before;
}

} // namespace inox::compiler::unit
