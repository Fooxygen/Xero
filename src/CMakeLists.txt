
cmake_minimum_required(VERSION 3.21)
project(xero LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
find_package(LLVM REQUIRED CONFIG)
llvm_map_components_to_libnames(XERO_LLVM_LIBS
    core support native passes scalaropts vectorize ipo aggressiveinstcombine analysis
)

add_executable(xero
    xero.cpp
    lexer/lexer.cpp
    parser/parser.cpp
    parser/rule.cpp
    sema/type.cpp
    sema/sema.cpp
    sema/builtin_register.cpp
    xcompiler/ir/ir.cpp
    xcompiler/ir/type.cpp
    xcompiler/optimizer/optimizer.cpp
    xengine/type.cpp
    xengine/xengine.cpp
    xengine/builtin_impl_register.cpp
    xengine/obj/impl/range.cpp
    xengine/obj/impl/array.cpp
    xengine/obj/impl/string.cpp
)

target_include_directories(xero PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(xero SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(xero PRIVATE ${XERO_LLVM_LIBS} ${LLVM_SYSTEM_LIBS})
target_compile_options(xero PRIVATE
    $<$<CONFIG:Debug>:   -g  -Wall -Wextra  -Werror>
    $<$<CONFIG:Release>: -O3 -flto -s -Wall -Wextra -Werror>
)
target_link_options(xero PRIVATE
    $<$<CONFIG:Release>: -flto -s>
)
