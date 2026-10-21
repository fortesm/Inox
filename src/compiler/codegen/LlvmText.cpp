// SPDX-License-Identifier: MPL-2.0
// Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "LlvmText.h"
#include "../support/RealLiteral.h"

#include <cctype>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace inox::compiler::codegen::detail {

bool equalsIgnoreCase(std::string_view left, std::string_view right)
{
    if (left.size() != right.size()) {
        return false;
    }

    for (std::size_t index = 0; index < left.size(); ++index) {
        if (std::tolower(static_cast<unsigned char>(left[index])) !=
            std::tolower(static_cast<unsigned char>(right[index]))) {
            return false;
        }
    }

    return true;
}

std::string normalize(std::string_view name)
{
    std::string normalized;
    normalized.reserve(name.size());
    for (const char ch : name) {
        normalized.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

// LLVM shares one namespace between local values and basic-block labels, and the
// emitter's own temporaries (`%tmpN`) and labels (`thenN`, `forcondN`, ...) always
// end in a digit. A user identifier must therefore never be emitted verbatim when
// it could collide with one of those names.
std::string safeLlvmName(const std::string& name)
{
    if (name.empty()) {
        return name;
    }
    const bool endsWithDigit = std::isdigit(static_cast<unsigned char>(name.back())) != 0;
    const bool reserved = name == "entry" || (name.size() >= 2 && name[0] == '_' && name[1] == '_');
    return (endsWithDigit || reserved) ? name + ".v" : name;
}

std::string llvmIntegerLiteral(std::string_view value)
{
    if (value.empty() || value.front() != '$') {
        return std::string(value);
    }

    return std::to_string(std::stoull(std::string(value.substr(1)), nullptr, 16));
}

// A struct field default (CANON-9) is one literal, possibly negative; the
// semantic analyzer has checked its type and range.
// LLVM writes a real constant as the 64-bit pattern of the double holding
// it; a float constant is the double of the rounded float value, so every
// value is exact (change float-conversions).
std::string llvmRealConstant(double value, bool float32)
{
    const double exact = float32 ? static_cast<double>(static_cast<float>(value)) : value;
    std::uint64_t bits = 0;
    std::memcpy(&bits, &exact, sizeof bits);
    std::ostringstream hex;
    hex << "0x" << std::uppercase << std::hex << std::setw(16) << std::setfill('0') << bits;
    return hex.str();
}

std::string llvmDefaultLiteral(std::string_view value, std::string_view llvmType)
{
    if (llvmType == "i1") {
        return equalsIgnoreCase(value, "true") ? "1" : "0";
    }
    if (llvmType == "double" || llvmType == "float") {
        return llvmRealConstant(support::parseRealLiteral(value, llvmType == "float"), llvmType == "float");
    }
    const bool negative = !value.empty() && value.front() == '-';
    std::string magnitude(negative ? value.substr(1) : value);
    int base = 10;
    if (!magnitude.empty() && magnitude.front() == '$') {
        magnitude.erase(0, 1);
        base = 16;
    } else if (magnitude.size() > 2 && magnitude[0] == '0' && (magnitude[1] == 'x' || magnitude[1] == 'X')) {
        magnitude.erase(0, 2);
        base = 16;
    }
    const std::uint64_t bits = std::stoull(magnitude, nullptr, base);
    if (!negative) {
        return std::to_string(bits);
    }
    return bits == (std::uint64_t{1} << 63) ? "-9223372036854775808"
                                             : "-" + std::to_string(bits);
}

namespace {

void appendEscapedLlvmByte(std::ostringstream& output, unsigned char byte)
{
    if (byte >= 0x20 && byte <= 0x7e && byte != '"' && byte != '\\') {
        output << static_cast<char>(byte);
        return;
    }

    output << '\\'
           << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(byte)
           << std::nouppercase << std::dec << std::setfill(' ');
}

} // namespace

LlvmStringConstant llvmStringConstant(std::string_view value)
{
    std::ostringstream bytes;
    std::size_t size = 0;

    for (std::size_t index = 0; index < value.size(); ++index) {
        unsigned char byte = static_cast<unsigned char>(value[index]);
        if (byte == '\\' && index + 1 < value.size()) {
            const char escape = value[++index];
            switch (escape) {
            case 'n':
                byte = '\n';
                break;
            case 'r':
                byte = '\r';
                break;
            case 't':
                byte = '\t';
                break;
            case '\\':
                byte = '\\';
                break;
            case '"':
                byte = '"';
                break;
            case '0':
                byte = '\0';
                break;
            default:
                appendEscapedLlvmByte(bytes, '\\');
                ++size;
                byte = static_cast<unsigned char>(escape);
                break;
            }
        }

        appendEscapedLlvmByte(bytes, byte);
        ++size;
    }

    appendEscapedLlvmByte(bytes, '\0');
    ++size;
    return LlvmStringConstant{size, bytes.str()};
}

std::string llvmByteString(const std::string& text)
{
    std::string encoded;
    for (const unsigned char ch : text) {
        if (ch == '\n') {
            encoded += "\\0A";
        } else if (ch == '"' || ch == '\\' || ch < 0x20 || ch >= 0x7f) {
            constexpr char hex[] = "0123456789ABCDEF";
            encoded += '\\';
            encoded += hex[ch >> 4];
            encoded += hex[ch & 0x0f];
        } else {
            encoded += static_cast<char>(ch);
        }
    }
    return encoded;
}

std::string llvmGlobalName(const std::string& name)
{
    // LLVM: [-a-zA-Z$._][-a-zA-Z$._0-9]* is written bare.
    bool bare = !name.empty() && std::isdigit(static_cast<unsigned char>(name.front())) == 0;
    for (const char ch : name) {
        bare = bare && (std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '-' || ch == '$' || ch == '.' ||
                        ch == '_');
    }
    return bare ? name : "\"" + name + "\"";
}

} // namespace inox::compiler::codegen::detail
