
cmake_minimum_required(VERSION 3.15)
project(xero LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_COMPILER "P:/Application/MinGW/mingw64-x86_64-16.1.0-release-posix-seh-ucrt-rt_v14-rev1/bin/g++.exe")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(xero
    xero.cpp
    lexer/lexer.cpp
    parser/parser.cpp
    parser/rule.cpp
    runtime/engine.cpp
)

target_include_directories(xero PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
