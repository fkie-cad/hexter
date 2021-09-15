set(HEXTER hexter)

add_executable(${HEXTER}
	${CMAKE_CURRENT_SOURCE_DIR}/src/hexter.c
	${HEXTER_FILES}
	)

set(HEXTER_LIB_NAME hexter) # see Libs.cmake
set(HEXTER_SHARED_LIB hexter_shared)
add_library(${HEXTER_SHARED_LIB} SHARED ${HEXTER_LIB_FILES} )
set_target_properties(${HEXTER_SHARED_LIB} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${HEXTER_SHARED_LIB} PROPERTIES OUTPUT_NAME ${HEXTER_LIB_NAME})
set_target_properties(${HEXTER_SHARED_LIB} PROPERTIES POSITION_INDEPENDENT_CODE ON)



set(TEST_HEXTER_LIB test_hexter_dll)
add_executable(${TEST_HEXTER_LIB}
	${CMAKE_CURRENT_SOURCE_DIR}/tests/test_hexter_dll.c
	 )
target_link_libraries(${TEST_HEXTER_LIB}
	${CMAKE_CURRENT_BINARY_DIR}/${HEXTER_LIB_FULL_NAME}
	)
add_dependencies(${TEST_HEXTER_LIB}
	${HEXTER_SHARED_LIB}
	)
