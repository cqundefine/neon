cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM 15 REQUIRED CONFIG)

project(Neon)

include_directories(src)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(Neon ${SOURCES})

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

target_link_libraries(Neon LLVM)
