// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Unit tests of AST copies (OpenSpec change type-representation): a copy has
// the same shape as its original, shares no node with it, and passes every
// type name and expression through the rewriters.

// F4 (change structured-types): the written type the parser builds. The
// canonical spelling (design.md, table), the invariants of TypeExpr, and a
// round trip over every written type of tests/, examples/ and stdlib/.

#include "UnitTest.h"

#include "compiler/ast/AstClone.h"
#include "compiler/ast/TypeExpr.h"
#include "compiler/parser/Parser.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace inox::compiler::unit {

namespace {

using namespace ast;

// The written type of one struct field, `F <text>`.
TypeExpr parseType(const std::string& text)
{
    const std::string source = "Type\n    TT Struct\n        F " + text + "\n    ;\n";
    parser::Parser parser{std::string_view(source)};
    const auto module = parser.parseModule("TypeExprTest");
    for (const auto& item : module->items()) {
        if (item->kind() == AstNodeKind::SectionDeclaration) {
            return static_cast<const SectionDeclaration&>(*item).types().front().fields.front().type;
        }
    }
    throw std::logic_error("no field in " + text);
}

// Equal trees, positions aside.
bool sameShape(const TypeExpr& left, const TypeExpr& right)
{
    if (left.form != right.form || left.name != right.name || left.low.text != right.low.text ||
        left.low.isName != right.low.isName || left.high.text != right.high.text ||
        left.high.isName != right.high.isName || left.arguments.size() != right.arguments.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.arguments.size(); ++index) {
        if (!sameShape(left.arguments[index], right.arguments[index])) {
            return false;
        }
    }
    return true;
}

bool positioned(const TypeExpr& type)
{
    bool known = type.position.known();
    for (const TypeExpr& argument : type.arguments) {
        known = known && positioned(argument);
    }
    return known;
}

void testCanonicalSpelling()
{
    const char* const table[][2] = {
        {"Integer", "Integer"},
        {"TPoint", "TPoint"},
        {"Bank.Accounts.TAccount", "Bank.Accounts.TAccount"},
        {"Array[1..3] Int64", "Array[1..3] Int64"},
        {"Array[ -1 .. N ] T", "Array[-1..N] T"},
        {"Array[1..2, 1..3] T", "Array[1..2] Array[1..3] T"},
        {"Array[$10..$1F] T", "Array[$10..$1F] T"},
        {"Array[0x10..0x1F] T", "Array[$10..$1F] T"},
        {"Array[1_000..1_009] T", "Array[1000..1009] T"},
        {"slice<int8>", "Slice<int8>"},
        {"own<T>", "Own<T>"},
        {"LINK<TNode>", "Link<TNode>"},
        {"option<own<TNode>>", "Option<Own<TNode>>"},
        {"TPair<String,Own<TNode>>", "TPair<String, Own<TNode>>"},
        {"Option<Own<Array[1..2, 1..3] TItem>>", "Option<Own<Array[1..2] Array[1..3] TItem>>"},
        {"Set<TColor>", "Set<TColor>"},
        {"set<TColor>", "set<TColor>"},
        {"Array[1..2, 1..3, -4..4] TPair<Bank.TKey, Array[1..2] Own<TNode>>",
         "Array[1..2] Array[1..3] Array[-4..4] TPair<Bank.TKey, Array[1..2] Own<TNode>>"},
        {"TTriple<A.B.TX, Slice<Int8>, Link<Array[1..9] TPair<K, V>>>",
         "TTriple<A.B.TX, Slice<Int8>, Link<Array[1..9] TPair<K, V>>>"},
    };
    for (const auto& row : table) {
        const TypeExpr type = parseType(row[0]);
        checkEqual(spell(type), row[1], std::string("the spelling of ") + row[0]);
        check(positioned(type), std::string("every node of ") + row[0] + " has a position");
        bool valid = true;
        try {
            checkTypeExpr(type);
        } catch (const std::logic_error&) {
            valid = false;
        }
        check(valid, std::string("the invariants hold for ") + row[0]);
    }
    const TypeExpr bounds = parseType("Array[-1..N] T");
    check(!bounds.low.isName && bounds.low.text == "-1" && bounds.high.isName && bounds.high.text == "N",
          "a literal bound and a Const bound");
    const TypeExpr cube = parseType("Array[1..2, 1..3] T");
    check(cube.isArray() && cube.element().isArray() && cube.element().low.text == "1" &&
              cube.element().high.text == "3" && cube.element().element().isNamed(),
          "one Array node per dimension, the first outermost");
}

void testInvariantsAreChecked()
{
    const auto rejects = [](const TypeExpr& type) {
        try {
            checkTypeExpr(type);
        } catch (const std::logic_error&) {
            return true;
        }
        return false;
    };
    TypeExpr empty;
    check(rejects(empty), "1: a Named type has a name");
    TypeExpr spaced = namedType("A B");
    check(rejects(spaced), "1: a Named type's name is a qualified name");
    TypeExpr array;
    array.form = TypeExpr::Form::Array;
    array.low.text = "1";
    array.high.text = "3";
    check(rejects(array), "2: an Array has its element");
    array.arguments.push_back(namedType("Int64"));
    array.high.text = "";
    check(rejects(array), "2: an Array has both bounds");
    TypeExpr generic;
    generic.form = TypeExpr::Form::Generic;
    generic.name = "Own";
    check(rejects(generic), "3: a Generic has an argument");
    generic.arguments.push_back(namedType("TNode"));
    generic.low.text = "1";
    check(rejects(generic), "3: a Generic has no bounds");
}

// Every written type of the corpus: its invariants, a position on each
// node, and parse(spell(t)) == t (design.md, invariant 6).
void collect(const std::vector<StatementPtr>& body, std::vector<TypeExpr>& found)
{
    CloneRewriters visitor;
    visitor.type = [&](TypeExpr& written) { found.push_back(written); };
    (void)cloneStatements(body, visitor);
}

void testCorpusRoundTrip()
{
    const std::filesystem::path root = INOX_SOURCE_DIR;
    std::size_t files = 0;
    std::size_t types = 0;
    std::vector<std::string> failures;
    for (const char* folder : {"tests", "examples", "stdlib"}) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root / folder)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".inox") {
                continue;
            }
            std::ifstream input(entry.path(), std::ios::binary);
            std::stringstream text;
            text << input.rdbuf();
            const std::string source = text.str();
            std::vector<TypeExpr> found;
            try {
                parser::Parser parser{std::string_view(source)};
                const auto module = parser.parseModule(entry.path().stem().string());
                for (const auto& item : module->items()) {
                    if (item->kind() == AstNodeKind::FunctionDeclaration) {
                        const auto& function = static_cast<const FunctionDeclaration&>(*item);
                        for (const ParameterDeclaration& parameter : function.parameters()) {
                            if (parameter.type) {
                                found.push_back(*parameter.type);
                            }
                        }
                        if (function.resultType()) {
                            found.push_back(*function.resultType());
                        }
                        collect(function.body(), found);
                    } else if (item->kind() == AstNodeKind::SectionDeclaration) {
                        const auto& section = static_cast<const SectionDeclaration&>(*item);
                        for (const TypeDeclaration& type : section.types()) {
                            for (const StructFieldDeclaration& field : type.fields) {
                                found.push_back(field.type);
                            }
                        }
                        for (const VariableDeclaration& variable : section.variables()) {
                            if (variable.type) {
                                found.push_back(*variable.type);
                            }
                        }
                    }
                }
            } catch (const std::exception&) {
                continue;  // a program the parser rejects writes no type to check
            }
            ++files;
            for (const TypeExpr& type : found) {
                ++types;
                const std::string spelled = spell(type);
                try {
                    checkTypeExpr(type);
                    if (!positioned(type)) {
                        failures.push_back(entry.path().string() + ": no position: " + spelled);
                    }
                    const TypeExpr again = parseType(spelled);
                    if (!sameShape(type, again) || spell(again) != spelled) {
                        failures.push_back(entry.path().string() + ": round trip: " + spelled);
                    }
                } catch (const std::exception& error) {
                    failures.push_back(entry.path().string() + ": " + error.what());
                }
            }
        }
    }
    for (const std::string& failure : failures) {
        check(false, failure);
    }
    check(files > 500 && types > 1000,
          "the corpus was read (" + std::to_string(files) + " files, " + std::to_string(types) + " types)");
}

} // namespace

int runTypeExprTests()
{
    const int before = failureCount();
    testCanonicalSpelling();
    testInvariantsAreChecked();
    testCorpusRoundTrip();
    return failureCount() - before;
}

} // namespace inox::compiler::unit
