// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Unit tests of the type module (OpenSpec change type-representation).

#include "UnitTest.h"

#include "compiler/types/TypeContext.h"
#include "compiler/types/TypeSyntax.h"

#include <limits>
#include <stdexcept>

namespace inox::compiler::unit {

namespace {

using types::Type;
using types::TypeContext;
using types::TypeKind;

void testBuiltins()
{
    TypeContext context;
    const Type* int8 = context.builtin("Int8");
    check(int8 != nullptr && int8->isInteger() && int8->bits() == 8 && int8->isSigned(), "Int8 is a signed 8-bit integer");
    check(int8->minValue() == -128 && int8->maxValue() == 127, "Int8 range");
    check(int8->isNarrowInteger(), "Int8 is narrow");
    check(!context.int64Type()->isNarrowInteger(), "Int64 is not narrow");
    check(context.builtin("uint32")->maxValue() == 4294967295LL, "UInt32 range, case-insensitive lookup");
    check(!context.builtin("UInt64")->rangeIsExact(), "UInt64 range is clipped to Int64");
    const Type* natural = context.naturalType();
    check(natural != context.int64Type() && natural->minValue() == 0 &&
              natural->maxValue() == std::numeric_limits<std::int64_t>::max(),
          "Natural is a distinct non-negative Int64");
    check(context.builtin("Float32")->isFloat() && context.builtin("Float32")->bits() == 32, "Float32");
    check(context.float64Type()->bits() == 64, "Float64");
    check(context.stringType()->isString() && context.fileType()->isFile(), "String and File");
    check(context.builtin("Currency")->kind() == TypeKind::Decimal, "Currency is a decimal type");
    check(context.builtin("TNothing") == nullptr, "unknown built-in");
    check(TypeContext::coreBuiltinNames().size() == 16, "16 core built-in names");
}

void testNamesAndAliases()
{
    TypeContext context;
    check(context.lookup("Int64") == nullptr, "names are declared by the analyzer, not implicitly");
    context.declareName("Int64", context.int64Type());
    context.declareName("Integer", context.int64Type());
    check(context.lookup("integer") == context.int64Type(), "an alias is the same object");
    checkEqual(context.lookup("INTEGER")->name(), "Int64", "an alias prints its canonical name");
}

void testInterning()
{
    TypeContext context;
    const Type* byte = context.builtin("UInt8");
    const Type* first = context.array(1, 10, byte);
    const Type* second = context.array(1, 10, byte);
    check(first == second, "equal arrays are one object");
    check(context.array(0, 10, byte) != first, "different bounds are different types");
    checkEqual(first->name(), "Array[1..10] UInt8", "array spelling");
    check(first->length() == 10 && first->low() == 1 && first->high() == 10 && first->element() == byte,
          "array bounds and element");
    const Type* matrix = context.array(1, 3, context.array(1, 4, context.int64Type()));
    checkEqual(matrix->name(), "Array[1..3] Array[1..4] Int64", "nested array spelling");
    check(context.slice(context.int64Type()) == context.slice(context.int64Type()), "slices are interned");
    checkEqual(context.slice(context.int64Type())->name(), "Slice<Int64>", "slice spelling");
}

void testPointersAndStructs()
{
    TypeContext context;
    const Type* node = context.declareStruct("TNo");
    const Type* owner = context.own(node);
    const Type* option = context.optionOwn(owner);
    const Type* link = context.link(node);
    checkEqual(owner->name(), "Own<TNo>", "Own spelling");
    checkEqual(option->name(), "Option<Own<TNo>>", "Option spelling");
    checkEqual(link->name(), "Link<TNo>", "Link spelling");
    check(owner->isPointer() && option->isPointer() && link->isPointer(), "the three pointers");
    check(owner->pointerTarget() == node && option->pointerTarget() == node && link->pointerTarget() == node,
          "every pointer reaches TNo");
    check(context.own(node) == owner && context.link(node) == link, "pointers are interned");
    check(context.lookup("tno") == node, "a struct declares its name");

    // A struct may reach itself through a pointer, so its fields come later.
    context.setStructFields(node, {types::StructField{"FValor", context.int64Type(), false, true, "0"},
                                   types::StructField{"FProximo", option, false, false, ""}});
    check(node->fields().size() == 2, "two fields");
    const types::StructField* next = node->field("fproximo");
    check(next != nullptr && next->type == option && node->fieldIndex(*next) == 1,
          "a field by name, case-insensitive, with its index");
    context.setStructFieldType(node, 0, context.naturalType());
    check(node->fields()[0].type == context.naturalType(), "a field's type completed later");
    bool refused = false;
    try {
        context.setStructFields(context.int64Type(), {});
    } catch (const std::logic_error&) {
        refused = true;
    }
    check(refused, "only a struct gets its fields later");
}

void testSyntax()
{
    const auto named = types::readTypeSyntax("TContato");
    check(named && named->form == types::TypeSyntax::Form::Named && named->name == "TContato", "a named type");
    const auto array = types::readTypeSyntax("Array[1..Tamanho] TContato");
    check(array && array->form == types::TypeSyntax::Form::Array && array->low == "1" &&
              array->high == "Tamanho" && array->argument->name == "TContato",
          "an array with a Const bound");
    const auto link = types::readTypeSyntax("Link<Array[1..5] TContato>");
    check(link && link->form == types::TypeSyntax::Form::Generic && link->name == "Link" &&
              link->argument->form == types::TypeSyntax::Form::Array,
          "a Link to an array");
    const auto nested = types::readTypeSyntax("Option<Own<TNo>>");
    check(nested && nested->argument->name == "Own" && nested->argument->argument->name == "TNo",
          "Option<Own<TNo>>");
    checkEqual(nested->spelling(), "Option<Own<TNo>>", "spelling round trip");
    checkEqual(types::spellArrayType("1", "3", types::spellArrayType("1", "4", "Int64")),
               "Array[1..3] Array[1..4] Int64", "the parser's array spelling");
    check(!types::readTypeSyntax("Array[1..] Int64"), "an array without a bound");
    check(!types::readTypeSyntax(""), "an empty type");
    check(types::isGenericSpelling("own<TNo>", "Own") && !types::isGenericSpelling("Owner"),
          "generic shape tests");
    check(types::isArraySpelling("array[1..2] Bool"), "array shape test");
}

} // namespace

int runTypesTests()
{
    const int before = failureCount();
    testBuiltins();
    testNamesAndAliases();
    testInterning();
    testPointersAndStructs();
    testSyntax();
    return failureCount() - before;
}

} // namespace inox::compiler::unit
