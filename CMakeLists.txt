cmake_minimum_required(VERSION 3.10)
project(Hexter VERSION 1.5.4 DESCRIPTION "A command line hex viewer." LANGUAGES C)

if (WIN32)
    message("No cmake support on Windows. Use msbuild and winBuild.bat.")
    return()
endif()

set(LANGUAGE C)
set(CMAKE_C_STANDARD 99)
if (UNIX)
    # D_FILE_OFFSET_BITS using 64 bit version of file functions
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -pedantic -Werror=return-type -Werror=overflow -D_FILE_OFFSET_BITS=64")
    set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -Wl,-z,relro,-z,now -fno-builtin -Werror=return-type -Werror=overflow -D_FILE_OFFSET_BITS=64 -Ofast")

    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fsanitize=leak -fsanitize=undefined -fno-omit-frame-pointer")

    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak -fsanitize=undefined -fsanitize=address")
endif (UNIX)

message("DEBUG_PRINT : ${DEBUG_PRINT}")
if (DEBUG_PRINT)
    add_definitions(-DDEBUG_PRINT=${DEBUG_PRINT})
endif()
message("ERROR_PRINT : ${ERROR_PRINT}")
if (ERROR_PRINT)
    add_definitions(-DERROR_PRINT=${ERROR_PRINT})
endif()

message("CMAKE_FLAGS_DEBUG : ${CMAKE_FLAGS_DEBUG}")
message("CMAKE_C_FLAGS_DEBUG : ${CMAKE_C_FLAGS_DEBUG}")
message("CMAKE_C_FLAGS_RELEASE : ${CMAKE_C_FLAGS_RELEASE}")
message("CMAKE_FLAGS_RELEASE : ${CMAKE_FLAGS_RELEASE}")
message("CMAKE_EXE_LINKER_FLAGS : ${CMAKE_EXE_LINKER_FLAGS}")

include(cmake.inc/Libs.cmake)
include(cmake.inc/Sources.cmake)
include(cmake.inc/Main.cmake)

# tests
if ( ${GTEST_FOUND} )
    enable_language(CXX)
    include(cmake.inc/Tests.cmake)
endif()
