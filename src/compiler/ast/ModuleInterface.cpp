// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ModuleInterface.h"

#include <sstream>
#include <vector>

namespace inox::compiler::ast {
namespace {

std::string typeParameterList(const std::vector<std::string>& parameters)
{
    if (parameters.empty()) {
        return {};
    }
    std::string text = "<";
    for (std::size_t index = 0; index < parameters.size(); ++index) {
        text += (index == 0 ? "" : ", ") + parameters[index];
    }
    return text + ">";
}

// A routine header as a program writes it: `(Self mut, ref A T) R`.
std::string headerText(const RoutineHeader& header)
{
    std::string text;
    for (const ParameterDeclaration& parameter : header.parameters) {
        text += text.empty() ? "(" : ", ";
        if (parameter.isReceiver) {
            text += parameter.name;
            text += parameter.mode == ParameterDeclaration::Mode::RefMut ? " mut" : "";
            continue;
        }
        text += parameter.mode == ParameterDeclaration::Mode::Ref      ? "ref "
                : parameter.mode == ParameterDeclaration::Mode::RefMut ? "ref mut "
                                                                       : "";
        text += parameter.name + " " + parameter.typeName;
    }
    if (!text.empty()) {
        text += ")";
    }
    if (!header.resultTypeName.empty()) {
        text += " " + header.resultTypeName;
    }
    return text;
}

// A value as the parser spelled it, `- 1 .. 9`, written `-1..9`.
std::string tidyValue(const std::string& text)
{
    std::string result;
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (text.compare(index, 4, " .. ") == 0) {
            result += "..";
            index += 3;
            continue;
        }
        const bool unaryMinus = text[index] == '-' && index + 1 < text.size() && text[index + 1] == ' ' &&
                                (result.empty() || result.back() == '.' || result.back() == '(' ||
                                 (result.size() >= 2 && result.compare(result.size() - 2, 2, ", ") == 0));
        result += text[index];
        if (unaryMinus) {
            ++index;
        }
    }
    return result;
}

std::string contractLine(const ContractClause& clause)
{
    const char* word = "pre";
    switch (clause.kind) {
    case ContractClause::Kind::Pre: word = "pre"; break;
    case ContractClause::Kind::PreElse: word = "pre else"; break;
    case ContractClause::Kind::Pos: word = "pos"; break;
    case ContractClause::Kind::PosThen: word = "pos then"; break;
    case ContractClause::Kind::Invariant: word = "invariant"; break;
    case ContractClause::Kind::LoopInvariant: word = "invariant"; break;
    case ContractClause::Kind::LoopVariant: word = "variant"; break;
    }
    std::string line = std::string(word) + " " + clause.text;
    if (!clause.message.empty()) {
        line += ", \"" + clause.message + "\"";
    }
    return line;
}

std::string fieldLine(const StructFieldDeclaration& field)
{
    std::string line = field.name + " " + field.typeName;
    if (field.hasDefault) {
        line += field.fillDefault ? " := Fill(" + field.defaultValue + ")" : " := " + field.defaultValue;
    }
    return line;
}

void writeType(std::ostringstream& out, const TypeDeclaration& type, const std::string& moduleName)
{
    const std::string name = type.name + typeParameterList(type.typeParameters);
    switch (type.kind) {
    case TypeDeclarationKind::Range:
        out << "    " << name << " Range " << tidyValue(type.boundsText) << " Public\n";
        return;
    case TypeDeclarationKind::Enum: {
        out << "    " << name << " (";
        for (std::size_t index = 0; index < type.literals.size(); ++index) {
            out << (index == 0 ? "" : ", ") << type.literals[index];
        }
        out << ") Public\n";
        return;
    }
    case TypeDeclarationKind::Struct:
        break;
    }
    std::vector<std::string> parts;
    std::vector<const StructFieldDeclaration*> publicFields;
    bool builtOnlyHere = false;
    for (const StructFieldDeclaration& field : type.fields) {
        if (field.isPart) {
            parts.push_back(field.typeName);
        } else if (field.isPublic) {
            publicFields.push_back(&field);
        } else if (!field.hasDefault) {
            builtOnlyHere = true;  // C2: a private field without a default
        }
    }
    out << "    " << name << " Struct";
    if (!parts.empty()) {
        out << '(';
        for (std::size_t index = 0; index < parts.size(); ++index) {
            out << (index == 0 ? "" : ", ") << parts[index];
        }
        out << ')';
    }
    out << " Public\n";
    if (builtOnlyHere) {
        out << "        == built only by " << moduleName << ": it has a private field without a default\n";
    }
    for (const ContractClause& invariant : type.invariants) {
        out << "        " << contractLine(invariant) << '\n';
    }
    if (!publicFields.empty()) {
        out << "    Public\n";
        for (const StructFieldDeclaration* field : publicFields) {
            out << "        " << fieldLine(*field) << '\n';
        }
    }
    out << "    ;\n";
}

} // namespace

std::string moduleInterface(const ModuleNode& module)
{
    std::ostringstream uses;
    std::ostringstream types;
    std::ostringstream consts;
    std::ostringstream routines;
    for (const AstNodePtr& item : module.items()) {
        switch (item->kind()) {
        case AstNodeKind::UseDeclaration: {
            const auto& use = static_cast<const UseDeclaration&>(*item);
            if (use.isExported()) {
                uses << "Export Use " << use.path().front() << '\n';
            }
            break;
        }
        case AstNodeKind::SectionDeclaration: {
            const auto& section = static_cast<const SectionDeclaration&>(*item);
            if (section.sectionKind() == SectionKind::Type) {
                for (const TypeDeclaration& type : section.types()) {
                    if (type.isPublic) {
                        writeType(types, type, module.name());
                    }
                }
            } else if (section.sectionKind() == SectionKind::Const) {
                for (const VariableDeclaration& constant : section.variables()) {
                    if (constant.isPublic) {
                        consts << "Const " << constant.name;
                        if (!constant.typeName.empty()) {
                            consts << ' ' << constant.typeName;
                        }
                        consts << " Public := " << tidyValue(constant.valueText) << '\n';
                    }
                }
            }
            break;
        }
        case AstNodeKind::FunctionDeclaration: {
            const auto& function = static_cast<const FunctionDeclaration&>(*item);
            if (!function.isPublic()) {
                break;
            }
            routines << '\n'
                     << function.name() << typeParameterList(function.typeParameters())
                     << headerText(function.header()) << (function.isOverride() ? " Override" : "")
                     << " Public\n";
            for (const ContractClause& clause : function.contracts()) {
                routines << "    " << contractLine(clause) << '\n';
            }
            break;
        }
        default:
            break;
        }
    }

    std::ostringstream out;
    out << "== interface of " << module.name() << ": what the modules that use it see (r3.99)\n";
    if (!uses.str().empty()) {
        out << '\n' << uses.str();
    }
    if (!types.str().empty()) {
        out << "\nType\n" << types.str();
    }
    if (!consts.str().empty()) {
        out << '\n' << consts.str();
    }
    out << routines.str();
    return out.str();
}

} // namespace inox::compiler::ast
