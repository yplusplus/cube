cmake_minimum_required(VERSION 2.8)

project(http_cli)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "-std=c++0x -Wall -g")
set(CMAKE_CXX_FLAGS_RELEASE "-std=c++0x -Wall -g")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

add_executable(http_cli main.cpp)

target_link_libraries(http_cli cube)
