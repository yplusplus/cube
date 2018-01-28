cmake_minimum_required(VERSION 2.8)

set(PROJECT redis_cli)
project(${PROJECT})

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "-std=c++0x -Wall -g")
set(CMAKE_CXX_FLAGS_RELEASE "-std=c++0x -Wall -g")

message($ENV{HIREDIS_ROOT})
include_directories($ENV{HIREDIS_ROOT})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../redis)
link_directories($ENV{HIREDIS_ROOT})

add_executable(${PROJECT} main.cpp)

target_link_libraries(${PROJECT} cuberedis cube libhiredis.a)
