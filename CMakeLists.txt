cmake_minimum_required(VERSION 3.3)
project(os161_C)

# get current kernel version
get_filename_component(KERN_VERS /home/bit/os161/root/kernel REALPATH)
get_filename_component(KERN_VERS ${KERN_VERS} NAME)
string(REPLACE kernel- "" KERN_VERS ${KERN_VERS})
message("kernel-${KERN_VERS} version")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu99")

include_directories(kern/include)
include_directories(kern/dev)
include_directories(kern/compile)
include_directories(kern/compile/${KERN_VERS}/includelinks)
include_directories(kern/compile/${KERN_VERS})

include_directories(userland/include)

file(GLOB_RECURSE INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE SOURCE_FILES "${PROJECT_SOURCE_DIR}/*.c")

add_executable (os161_C ${INCLUDE_DIRECTORIES} ${SOURCE_FILES} kern/lib/linked_list.c)