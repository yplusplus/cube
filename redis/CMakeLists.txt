cmake_minimum_required(VERSION 2.8)

set(PROJECT cuberedis)
set(HIREDIS_ROOT $ENV{HIREDIS_ROOT})
project(${PROJECT})

include_directories($ENV{HIREDIS_ROOT})

aux_source_directory(. SOURCES)

add_library(${PROJECT} ${SOURCES})
