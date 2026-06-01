# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.25)

project(InoxCompiler
    VERSION 0.1.0
    LANGUAGES CXX
)

set(INOX_COMPILER_SOURCES
    src/compiler/main/main.cpp
    src/compiler/ast/Ast.cpp
    src/compiler/codegen/LlvmIrEmitter.cpp
    src/compiler/lexer/Lexer.cpp
    src/compiler/parser/Parser.cpp
    src/compiler/semantic/SemanticAnalyzer.cpp
    src/compiler/semantic/Symbol.cpp
)

add_executable(inox ${INOX_COMPILER_SOURCES})

target_compile_features(inox PRIVATE cxx_std_20)
set_target_properties(inox PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(inox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(inox PRIVATE /W4 /permissive-)
else()
    target_compile_options(inox PRIVATE -Wall -Wextra -Wpedantic)
endif()
