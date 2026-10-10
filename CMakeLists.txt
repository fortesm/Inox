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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(InoxPlatform)
include(InoxCompilerOptions)
include(InoxWarnings)
include(InoxSanitizers)

set(INOX_COMPILER_SOURCES
    src/compiler/main/main.cpp
    src/compiler/ast/Ast.cpp
    src/compiler/codegen/LlvmIrEmitter.cpp
    src/compiler/lexer/Lexer.cpp
    src/compiler/parser/Parser.cpp
    src/compiler/semantic/SemanticAnalyzer.cpp
    src/compiler/semantic/Symbol.cpp
    src/compiler/support/Environment.cpp
    src/compiler/support/FileSystem.cpp
    src/compiler/support/Platform.cpp
    src/compiler/support/Process.cpp
)

add_library(inoxrt STATIC
    runtime/ExceptionRuntime.cpp
)

inox_apply_compiler_options(inoxrt)
inox_apply_warnings(inoxrt)

# MSVC ABI: an Inox program is linked by `clang++ program.ll inoxrt.lib`, which
# uses the static release CRT (libcmt/libvcruntime). The runtime must use the
# same CRT, whatever the compiler's build configuration: with two C++ runtimes
# in one process, the exception caught by the program's catchpad is not the
# "current exception" seen by inoxrt, so std::current_exception() is empty
# (typed handlers see every Inox exception as foreign, rethrow terminates).
if(CMAKE_CXX_COMPILER_TARGET MATCHES "msvc" OR MSVC OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC" OR
   (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    set_property(TARGET inoxrt PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    target_compile_options(inoxrt PRIVATE -U_DEBUG)
    target_compile_definitions(inoxrt PRIVATE _ITERATOR_DEBUG_LEVEL=0)
endif()

add_executable(inox ${INOX_COMPILER_SOURCES})

inox_apply_compiler_options(inox)
inox_apply_warnings(inox)
inox_apply_sanitizers(inox)

target_include_directories(inox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include(InoxInstall)
