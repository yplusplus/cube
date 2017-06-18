cmake_minimum_required(VERSION 2.8)

project(http_svr)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "-std=c++0x -Wall -g")
set(CMAKE_CXX_FLAGS_RELEASE "-std=c++0x -Wall -g")

include_directories(../..)
link_directories(../..)

add_executable(http_svr main.cpp)

target_link_libraries(http_svr cube)
