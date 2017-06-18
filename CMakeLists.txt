cmake_minimum_required(VERSION 2.8)

project(cube)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++0x -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -std=c++0x -O2 -Wall")

include_directories(.)

aux_source_directory(./base BASE_SOURCES)
aux_source_directory(./net NET_SOURCES)
aux_source_directory(./net/http NET_HTTP_SOURCES)

add_library(cube ${BASE_SOURCES} ${NET_SOURCES} ${NET_HTTP_SOURCES})
