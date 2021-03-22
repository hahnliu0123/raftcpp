cmake_minimum_required(VERSION 2.8)
project(raftcpp)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "DEBUG")
endif()

set(CXX_FLAGS 
     -ggdb 
     -Wall
     -Werror
     -Wno-unused-result
     -Wno-maybe-uninitialized
     -std=c++11
     -rdynamic
     -fPIC
     -O0  
)

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

include_directories(.)
include_directories(third_party/reyao)

find_package(Protobuf REQUIRED)
if (Protobuf_FOUND)
    include_directories(${Protobuf_INCLUDE_DIRS})
endif()

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
endif()

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(third_party/reyao)
add_subdirectory(kvraft)