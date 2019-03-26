set(HEXTER hexter)

add_executable(${HEXTER}
	${CMAKE_CURRENT_SOURCE_DIR}/src/hexter.c
	${HEXTER_FILES}
	${UTILS_FILES}
	)
