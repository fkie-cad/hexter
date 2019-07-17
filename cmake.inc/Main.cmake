set(HEXTER hexter)

add_executable(${HEXTER}
	${CMAKE_CURRENT_SOURCE_DIR}/src/hexter.c
	${HEXTER_FILES}
	${UTILS_FILES}
	)

#set(LIB_NAME hexter)
#set(SHARED_LIB hexter_shared)
#add_library(${SHARED_LIB} SHARED ${HEXTER_LIB_FILES})
#set_target_properties(${SHARED_LIB} PROPERTIES VERSION ${PROJECT_VERSION})
#set_target_properties(${SHARED_LIB} PROPERTIES OUTPUT_NAME ${LIB_NAME})
#set_target_properties(${SHARED_LIB} PROPERTIES POSITION_INDEPENDENT_CODE ON)