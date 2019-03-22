cmake_minimum_required(VERSION 3.10)
project(Hexter VERSION 1.0.0 DESCRIPTION "A command line hex viewer.")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wextra -ggdb")

message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")

# find libraries
include(cmake.inc/Libs.cmake)

# get variables
include(cmake.inc/Sources.cmake)

# create libs
include(cmake.inc/Main.cmake)

# tests
include(cmake.inc/Tests.cmake)