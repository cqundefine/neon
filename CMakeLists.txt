cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-missing-template-keyword)
endif()

find_package(LLVM 18.1.4 REQUIRED CONFIG PATHS /opt/llvm-18.1.4-debug/)
add_link_options("-Wl,-rpath=/opt/llvm-18.1.4-debug/lib")

project(Neon)

include_directories(src)

add_compile_options(-fdiagnostics-color=always)

file(GLOB_RECURSE SOURCES src/*.cpp src/AST/*.cpp)
add_executable(Neon ${SOURCES})

target_include_directories(Neon PRIVATE external)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

target_link_libraries(Neon LLVM)
