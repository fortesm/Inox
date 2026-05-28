cmake_minimum_required(VERSION 3.25)

project(InoxCompiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(inox
    src/compiler/main/main.cpp
    src/compiler/lexer/Lexer.cpp
)
