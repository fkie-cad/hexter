set(UTILS_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/common_fileio.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/common_fileio.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Converter.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Converter.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Helper.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Helper.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Strings.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Strings.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/TerminalUtil.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/TerminalUtil.c
)

set(HEXTER_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/src/Bool.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Globals.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Finder.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Finder.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/Printer.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Printer.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/Writer.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Writer.c
	${UTILS_FILES}
	${CMAKE_CURRENT_SOURCE_DIR}/src/ProcessHandlerLinux.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ProcessHandlerLinux.c
)

set(HEXTER_LIB_FILES
	${HEXTER_FILES}
	${CMAKE_CURRENT_SOURCE_DIR}/src/hexter.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/hexter.c
	${UTILS_FILES}
	)

