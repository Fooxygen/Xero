
cmake_minimum_required(VERSION 3.21)
project(xero LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_executable(xero
    xero.cpp
    lexer/lexer.cpp
    parser/parser.cpp
    parser/rule.cpp
    runtime/xengine.cpp
    runtime/register.cpp
    runtime/table/type.cpp
    runtime/obj/impl/array.cpp
    runtime/obj/impl/string.cpp
)

target_include_directories(xero PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
