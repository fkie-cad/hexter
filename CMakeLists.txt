cmake_minimum_required(VERSION 3.10)
project(Hexter VERSION 1.0.2 DESCRIPTION "A command line hex viewer." LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_FLAGS_DEBUG "${CMAKE_FLAGS_DEBUG} -Wall")
set(CMAKE_FLAGS_RELEASE "${CMAKE_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
if (UNIX)
	set(CMAKE_FLAGS_DEBUG "${CMAKE_FLAGS_DEBUG} -pedantic -fsanitize=address -fno-omit-frame-pointer -Werror=return-type")
	set(CMAKE_FLAGS_RELEASE "${CMAKE_FLAGS_RELEASE} -fno-builtin -Werror=return-type")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pedantic -fsanitize=address -fno-omit-frame-pointer -Werror=return-type")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-builtin -Werror=return-type")
endif (UNIX)

message("CMAKE_FLAGS_DEBUG is ${CMAKE_FLAGS_DEBUG}")
message("CMAKE_C_FLAGS_DEBUG} is ${CMAKE_C_FLAGS_DEBUG}")
message("CMAKE_C_FLAGS_RELEASE is ${CMAKE_C_FLAGS_RELEASE}")
message("CMAKE_FLAGS_RELEASE is ${CMAKE_FLAGS_RELEASE}")

include(cmake.inc/Libs.cmake)
include(cmake.inc/Sources.cmake)
include(cmake.inc/Main.cmake)

# tests
if ( ${GTEST_FOUND} )
	enable_language(CXX)
	include(cmake.inc/Tests.cmake)
endif()
