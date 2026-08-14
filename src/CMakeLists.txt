
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
    sema/semantics.cpp
    runtime/runtime.cpp
    runtime/xengine.cpp
    runtime/register.cpp
    runtime/obj/impl/range.cpp
    runtime/obj/impl/array.cpp
    runtime/obj/impl/string.cpp
)

target_include_directories(xero PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(xero PRIVATE
    $<$<CONFIG:Debug>:   -g -Wall -Wextra -Werror>
    $<$<CONFIG:Release>: -O3 -flto -s -Wall -Wextra -Werror>
)
target_link_options(xero PRIVATE
    $<$<CONFIG:Release>:-flto -s>
)
