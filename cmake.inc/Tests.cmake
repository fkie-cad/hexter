if ( ${GTEST_FOUND} )

	set(G_TESTS_DIR tests)

	set(HEXTER_TEST_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/${G_TESTS_DIR}/HexterTest.h
		)

	set(UNIT_TEST_SUITE hexter_tests)

	add_executable(
		${UNIT_TEST_SUITE}
		${G_TESTS_DIR}/unitTests.cpp
		${UTILS_FILES}
		${HEXTER_FILES}
		${HEXTER_TEST_FILES}
	)

	target_link_libraries(${UNIT_TEST_SUITE} PRIVATE
		${GTEST_BOTH_LIBRARIES}
		)

	add_test(
		cTests
		${UNIT_TEST_SUITE}
	)

endif()