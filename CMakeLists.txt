# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.25)

project(InoxCompiler
    VERSION 0.2.0
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(InoxPlatform)
include(InoxCompilerOptions)
include(InoxWarnings)
include(InoxSanitizers)

set(INOX_COMPILER_SOURCES
    src/compiler/ast/Ast.cpp
    src/compiler/ast/AstClone.cpp
    src/compiler/codegen/AstQueries.cpp
    src/compiler/codegen/ExceptionAbi.cpp
    src/compiler/codegen/FunctionEmitter.cpp
    src/compiler/codegen/FunctionEmitterAggregates.cpp
    src/compiler/codegen/FunctionEmitterArrays.cpp
    src/compiler/codegen/FunctionEmitterControlFlow.cpp
    src/compiler/codegen/FunctionEmitterExceptions.cpp
    src/compiler/codegen/FunctionEmitterExpressions.cpp
    src/compiler/codegen/FunctionEmitterFiles.cpp
    src/compiler/codegen/FunctionEmitterTerminal.cpp
    src/compiler/codegen/FunctionEmitterDebug.cpp
    src/compiler/codegen/FunctionEmitterContracts.cpp
    src/compiler/codegen/FunctionEmitterPointers.cpp
    src/compiler/codegen/FunctionEmitterIo.cpp
    src/compiler/codegen/FunctionEmitterStatements.cpp
    src/compiler/codegen/FunctionEmitterStrings.cpp
    src/compiler/codegen/FunctionEmitterStringFunctions.cpp
    src/compiler/codegen/FunctionEmitterEnums.cpp
    src/compiler/codegen/FunctionEmitterSets.cpp
    src/compiler/codegen/LlvmIrEmitter.cpp
    src/compiler/codegen/LlvmText.cpp
    src/compiler/codegen/MathBuiltins.cpp
    src/compiler/codegen/Operators.cpp
    src/compiler/codegen/RuntimeAbi.cpp
    src/compiler/codegen/RuntimeHelpersIr.cpp
    src/compiler/codegen/TypeLayout.cpp
    src/compiler/lexer/Lexer.cpp
    src/compiler/parser/Parser.cpp
    src/compiler/semantic/ConstantEvaluator.cpp
    src/compiler/semantic/SemanticAnalyzer.cpp
    src/compiler/semantic/SemanticBuiltins.cpp
    src/compiler/semantic/SemanticCalls.cpp
    src/compiler/semantic/SemanticDeclarations.cpp
    src/compiler/semantic/SemanticExpressions.cpp
    src/compiler/semantic/SemanticFiles.cpp
    src/compiler/semantic/SemanticTerminal.cpp
    src/compiler/semantic/SemanticGenerics.cpp
    src/compiler/semantic/SemanticDebug.cpp
    src/compiler/semantic/SemanticReuse.cpp
    src/compiler/semantic/ContractLowering.cpp
    src/compiler/semantic/SemanticContracts.cpp
    src/compiler/semantic/SemanticPointers.cpp
    src/compiler/semantic/SemanticStatements.cpp
    src/compiler/semantic/SemanticStrings.cpp
    src/compiler/semantic/SemanticEnums.cpp
    src/compiler/semantic/SemanticArrays.cpp
    src/compiler/semantic/SemanticSets.cpp
    src/compiler/semantic/SemanticSupport.cpp
    src/compiler/semantic/SemanticTypes.cpp
    src/compiler/semantic/Symbol.cpp
    src/compiler/types/TypeContext.cpp
    src/compiler/types/TypeSyntax.cpp
    src/compiler/support/Environment.cpp
    src/compiler/support/FileSystem.cpp
    src/compiler/support/Platform.cpp
    src/compiler/support/Process.cpp
)

add_library(inoxrt STATIC
    runtime/ConsoleRuntime.cpp
    runtime/DecimalRuntime.cpp
    runtime/ExceptionRuntime.cpp
    runtime/FileRuntime.cpp
    runtime/NumberText.cpp
    runtime/OwnRuntime.cpp
    runtime/StringRuntime.cpp
    runtime/StringFunctions.cpp
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

# Unit tests of the runtime library, run by scripts/run-tests.sh and
# scripts/run-tests.ps1 when found next to the compiler.
add_executable(inoxrt-tests tests/runtime-unit/StringRuntimeTests.cpp tests/runtime-unit/FileRuntimeTests.cpp
    tests/runtime-unit/OwnRuntimeTests.cpp tests/runtime-unit/StringFunctionsTests.cpp)
target_link_libraries(inoxrt-tests PRIVATE inoxrt)
inox_apply_compiler_options(inoxrt-tests)
inox_apply_warnings(inoxrt-tests)
inox_apply_sanitizers(inoxrt-tests)
if(CMAKE_CXX_COMPILER_TARGET MATCHES "msvc" OR MSVC OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC" OR
   (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    # Same C runtime as inoxrt, which it links.
    set_property(TARGET inoxrt-tests PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    target_compile_options(inoxrt-tests PRIVATE -U_DEBUG)
    target_compile_definitions(inoxrt-tests PRIVATE _ITERATOR_DEBUG_LEVEL=0)
endif()

# The BUILD number of the version (canon SECTION 02), regenerated at every
# build into build/generated/InoxBuildNumber.h.
set(INOX_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
add_custom_target(inox-build-number ALL
    COMMAND "${CMAKE_COMMAND}"
            "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            "-DOUTPUT=${INOX_GENERATED_DIR}/InoxBuildNumber.h"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/InoxBuildNumber.cmake"
    BYPRODUCTS "${INOX_GENERATED_DIR}/InoxBuildNumber.h"
    COMMENT "Inox build number"
    VERBATIM)

# The compiler as a library: the `inox` driver and the compiler unit tests
# (tests/compiler-unit) link the same objects.
add_library(inox-compiler STATIC ${INOX_COMPILER_SOURCES})
add_dependencies(inox-compiler inox-build-number)
inox_apply_compiler_options(inox-compiler)
inox_apply_warnings(inox-compiler)
inox_apply_sanitizers(inox-compiler)
target_include_directories(inox-compiler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${INOX_GENERATED_DIR}
)

add_executable(inox src/compiler/main/main.cpp)
target_link_libraries(inox PRIVATE inox-compiler)
inox_apply_compiler_options(inox)
inox_apply_warnings(inox)
inox_apply_sanitizers(inox)

add_executable(inox-compiler-tests
    tests/compiler-unit/main.cpp
    tests/compiler-unit/TypesTests.cpp
    tests/compiler-unit/TypeLayoutTests.cpp
    tests/compiler-unit/AstCloneTests.cpp
)
target_link_libraries(inox-compiler-tests PRIVATE inox-compiler)
inox_apply_compiler_options(inox-compiler-tests)
inox_apply_warnings(inox-compiler-tests)
inox_apply_sanitizers(inox-compiler-tests)

include(InoxInstall)
