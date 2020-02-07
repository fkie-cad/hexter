enable_testing()
find_package(GTest QUIET)
#include_directories(${GTEST_INCLUDE_DIRS})
message("-- GTEST_FOUND: ${GTEST_FOUND} ${GTEST_BOTH_LIBRARIES}")

if (UNIX)
	set(LIB_EXTENSION so)
elseif (WIN32)
	set(LIB_EXTENSION lib)
endif ()

# cad filescanner
set(LIB_NAME hexter)
set(LIB_DIR ${CMAKE_SOURCE_DIR}/build)
set(HEXTER_LIB ${LIB_DIR}/lib${LIB_NAME}.so)

# cad filescanner debug
set(LIB_NAME hexter)
set(LIB_DIR ${CMAKE_SOURCE_DIR}/build/debug)
set(HEXTER_DEBUG_LIB ${LIB_DIR}/lib${LIB_NAME}.so)
