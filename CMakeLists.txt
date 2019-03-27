cmake_minimum_required(VERSION 3.10)
project(Hexter VERSION 1.0.2 DESCRIPTION "A command line hex viewer." LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_FLAGS_DEBUG "${CMAKE_FLAGS_DEBUG} -Wall -pedantic")
#set(CMAKE_FLAGS_DEBUG "${CMAKE_FLAGS_DEBUG} -Wall -pedantic -Wextra -ggdb")

message("CMAKE_FLAGS_DEBUG is ${CMAKE_FLAGS_DEBUG}")

# find libraries
include(cmake.inc/Libs.cmake)

# get variables
include(cmake.inc/Sources.cmake)

# create libs
include(cmake.inc/Main.cmake)

# tests
if ( ${GTEST_FOUND} )
	include(cmake.inc/Tests.cmake)
endif()
