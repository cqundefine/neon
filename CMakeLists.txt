cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-missing-template-keyword)
endif()

find_package(LLVM REQUIRED CONFIG)

project(Neon)

include_directories(src)

add_compile_options(-Wno-deprecated-declarations)

file(GLOB_RECURSE SOURCES src/*.cpp src/AST/*.cpp)
add_executable(Neon ${SOURCES})

target_include_directories(Neon PRIVATE external)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

target_link_libraries(Neon LLVM)
