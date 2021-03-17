cmake_minimum_required(VERSION 3.3)
project(os161_C)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu99")

include_directories(kern/include)
include_directories(kern/dev)
include_directories(kern/compile/DUMBVM/includelinks)
include_directories(kern/compile/DUMBVM)
include_directories(kern/compile)

include_directories(userland/include)

file(GLOB_RECURSE INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE SOURCE_FILES "${PROJECT_SOURCE_DIR}/*.c")

add_executable (os161_C ${INCLUDE_DIRECTORIES} ${SOURCE_FILES})