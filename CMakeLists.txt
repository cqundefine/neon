cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(LLVM REQUIRED CONFIG)

project(Neon)

include_directories(src)

add_compile_options(-fdiagnostics-color=always)

file(GLOB_RECURSE SOURCES src/*.cpp src/AST/*.cpp)
add_executable(Neon ${SOURCES})

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

target_link_libraries(Neon LLVM)
