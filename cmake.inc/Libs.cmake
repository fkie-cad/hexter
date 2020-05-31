enable_testing()
find_package(GTest QUIET)
#include_directories(${GTEST_INCLUDE_DIRS})
message("-- GTEST_FOUND: ${GTEST_FOUND} ${GTEST_BOTH_LIBRARIES}")

if (UNIX)
	set(LIB_EXTENSION so)
elseif (WIN32)
	set(LIB_EXTENSION lib)
endif ()

# release
set(HEXTER_LIB_NAME hexter)
set(HEXTER_LIB_FULL_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}${HEXTER_LIB_NAME}.${LIB_EXTENSION})
