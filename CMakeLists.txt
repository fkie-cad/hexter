cmake_minimum_required(VERSION 3.10)
project(Hexter VERSION 1.5.4 DESCRIPTION "A command line hex viewer." LANGUAGES C)

set(CMAKE_C_STANDARD 99)
if (UNIX)
	# D_FILE_OFFSET_BITS using 64 bit version of file functions
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -pedantic -Werror=return-type -D_FILE_OFFSET_BITS=64")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wl,-z,relro,-z,now -fno-builtin -Werror=return-type -D_FILE_OFFSET_BITS=64")

	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fsanitize=leak -fsanitize=undefined -fno-omit-frame-pointer")

	set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak -fsanitize=undefined -fsanitize=address")
endif (UNIX)
if (WIN32)
	SET(CMAKE_C_FLAGS "/nologo /Zi /GS /INCREMENTAL /MP /Gy /guard:cf /we4715 /we4716 ${CMAKE_C_FLAGS}")
	SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} /DEBUG /Od /Wall")
	SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /DNDEBUG /O2 /W1")

	#	SET(CMAKE_EXE_LINKER_FLAGS "/INCREMENTAL ${CMAKE_EXE_LINKER_FLAGS}")
	SET(CMAKE_EXE_LINKER_FLAGS "/NXCOMPAT /guard:cf ${CMAKE_EXE_LINKER_FLAGS}")
	SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
	SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DYNAMICBASE ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

	IF (CMAKE_CL_32)
		SET(CMAKE_EXE_LINKER_FLAGS "/SAFESEH ${CMAKE_EXE_LINKER_FLAGS}")
	ENDIF (CMAKE_CL_32)
	
	message("CMAKE_C_FLAGS : ${CMAKE_C_FLAGS_RELEASE}")
	message("CMAKE_EXE_LINKER_FLAGS : ${CMAKE_EXE_LINKER_FLAGS}")
	message("CMAKE_EXE_LINKER_FLAGS_DEBUG : ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
	message("CMAKE_EXE_LINKER_FLAGS_RELEASE : ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
endif (WIN32)


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
