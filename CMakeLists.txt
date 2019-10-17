cmake_minimum_required(VERSION 3.10)
project(Hexter VERSION 1.5.4 DESCRIPTION "A command line hex viewer." LANGUAGES C)

set(CMAKE_C_STANDARD 99)
if (UNIX)
	set(CMAKE_FLAGS_DEBUG "${CMAKE_FLAGS_DEBUG} -Wall -pedantic -Werror=return-type")
	set(CMAKE_FLAGS_RELEASE "${CMAKE_FLAGS_RELEASE} -fno-builtin -Werror=return-type")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -pedantic -Werror=return-type")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-builtin -Werror=return-type")

	set(CMAKE_FLAGS_DEBUG "${CMAKE_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")

	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif (UNIX)
if (WIN32)
	SET(CMAKE_C_FLAGS "/nologo /Zi /DNDEBUG /GS /O2 /INCREMENTAL /MP /Gy /guard:cf /W3 ${CMAKE_C_FLAGS}")
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS}")
	SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} /we4715 /we4716")

	SET(CMAKE_EXE_LINKER_FLAGS "/INCREMENTAL /DYNAMICBASE /NXCOMPAT /LARGEADDRESSAWARE /guard:cf ${CMAKE_EXE_LINKER_FLAGS}")

	IF (CMAKE_CL_32)
		SET(CMAKE_EXE_LINKER_FLAGS "/SAFESEH ${CMAKE_EXE_LINKER_FLAGS}")
	ENDIF (CMAKE_CL_32)
	
	message("CMAKE_C_FLAGS is ${CMAKE_C_FLAGS_RELEASE}")
	message("CMAKE_EXE_LINKER_FLAGS is ${CMAKE_EXE_LINKER_FLAGS}")
endif (WIN32)


message("CMAKE_FLAGS_DEBUG : ${CMAKE_FLAGS_DEBUG}")
message("CMAKE_C_FLAGS_DEBUG} : ${CMAKE_C_FLAGS_DEBUG}")
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
