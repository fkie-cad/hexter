if ( ${GTEST_FOUND} )

	set(TESTS_DIR tests)

	file(COPY ${TESTS_DIR}/files DESTINATION ./${TESTS_DIR}/)

	set(HEXTER_TEST_MISC_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/misc/Misc.h
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/misc/Misc.cpp
		)

	set(HEXTER_TEST_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/utils/ConverterTest.h
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/utils/HelperTest.h
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/utils/StringsTest.h
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/FinderTest.h
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/KeyStrokeTest.h
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/WriterTest.h
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/HexterTest.h
		)

	set(UNIT_TEST_SUITE hexter_tests)

	add_executable(
		${UNIT_TEST_SUITE}
		${TESTS_DIR}/unitTests.cpp
		${UTILS_FILES}
		${HEXTER_FILES}
		${HEXTER_TEST_MISC_FILES}
		${HEXTER_TEST_FILES}
	)
	
if (UNIX)
	set_target_properties(${UNIT_TEST_SUITE} PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
		LANGUAGES CXX
		COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror=return-type -fsanitize=address -fsanitize=leak -fsanitize=undefined -fno-omit-frame-pointer"
		LINK_FLAGS "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address -fsanitize=leak -fsanitize=undefined -fno-omit-frame-pointer"
		)
endif (UNIX)
#if (WIN32)
#	set_target_properties(${UNIT_TEST_SUITE} PROPERTIES
#		CXX_STANDARD 17
#		CXX_STANDARD_REQUIRED YES
#		CXX_EXTENSIONS NO
#		LANGUAGES CXX
#		COMPILE_FLAGS "${CMAKE_CXX_FLAGS} /W4 /nologo /Zi /GS /INCREMENTAL /MP /GS /Gy /guard:cf /we4715 /we4716 /DEBUG /Od /MDd"
#		LINK_FLAGS "${CMAKE_LINKER_FLAGS_DEBUG} /DEBUG ${CMAKE_EXE_LINKER_FLAGS_RELEASE}"
#		)
#endif (WIN32)
	target_link_libraries(${UNIT_TEST_SUITE} PRIVATE
		${GTEST_BOTH_LIBRARIES}
		)

	add_test(
		cTests
		${UNIT_TEST_SUITE}
	)



	set(TEST_SUITE hexter_lib_tests)

	add_executable(${TEST_SUITE} "")
	target_sources(${TEST_SUITE} PRIVATE
		${TESTS_DIR}/unitLibTests.cpp
		${UTILS_FILES}
		${HEXTER_FILES}
		${HEXTER_TEST_MISC_FILES}
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/HexterLibTest.h
		)
	add_dependencies(${TEST_SUITE}
		${HEXTER_SHARED_LIB}
		)
	set_target_properties(${TEST_SUITE} PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
		LANGUAGES CXX
		COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror=return-type -fsanitize=address -fsanitize=leak -fsanitize=undefined -fno-omit-frame-pointer"
		LINK_FLAGS "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address -fsanitize=leak -fsanitize=undefined -fno-omit-frame-pointer"
		)
	target_link_libraries(${TEST_SUITE} PRIVATE
		${GTEST_BOTH_LIBRARIES}
		${CMAKE_CURRENT_BINARY_DIR}/${HEXTER_LIB_FULL_NAME}
		)

endif()



add_executable(createRandBin
	${CMAKE_CURRENT_SOURCE_DIR}/tests/createRandBin.c
	)
	