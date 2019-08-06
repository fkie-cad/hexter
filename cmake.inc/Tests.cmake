if ( ${GTEST_FOUND} )

	set(TESTS_DIR tests)

	set(HEXTER_TEST_MISC_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/misc/Misc.h
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/misc/Misc.cpp
		)

	set(HEXTER_TEST_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/utils/ConverterTest.h
		${CMAKE_CURRENT_SOURCE_DIR}/${TESTS_DIR}/utils/HelperTest.h
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

	set_target_properties(${UNIT_TEST_SUITE} PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
		LANGUAGES CXX
		COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror=return-type -fsanitize=address -fno-omit-frame-pointer"
		LINK_FLAGS "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address"
		)

	target_link_libraries(${UNIT_TEST_SUITE} PRIVATE
		${GTEST_BOTH_LIBRARIES}
		)

	add_test(
		cTests
		${UNIT_TEST_SUITE}
	)

endif()

