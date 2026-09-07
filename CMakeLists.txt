cmake_minimum_required(VERSION 3.20)

project(kaleidoscope LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)

add_executable(kaleidoscope
    src/main.cpp
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/codegen.cpp
)

target_include_directories(kaleidoscope PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

add_definitions(${LLVM_DEFINITIONS})

target_compile_options(kaleidoscope PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(kaleidoscope PRIVATE
    LLVM
)
