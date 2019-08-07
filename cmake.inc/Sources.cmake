set(UTILS_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/common_fileio.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/common_fileio.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Converter.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Converter.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Helper.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/utils/Helper.c
	)
if (UNIX)
    set(UTILS_FILES
    	${UTILS_FILES}
		${CMAKE_CURRENT_SOURCE_DIR}/src/utils/TerminalUtil.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/utils/TerminalUtil.c
    )
endif (UNIX)

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
	)
if (UNIX)
    set(HEXTER_FILES
    	${HEXTER_FILES}
		${CMAKE_CURRENT_SOURCE_DIR}/src/ProcessHandlerLinux.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/ProcessHandlerLinux.c
    )
endif (UNIX)
if (WIN32)
    set(HEXTER_FILES
    	${HEXTER_FILES}
		${CMAKE_CURRENT_SOURCE_DIR}/src/ProcessHandlerWin.h
		${CMAKE_CURRENT_SOURCE_DIR}/src/ProcessHandlerWin.c
    )
endif (WIN32)

set(HEXTER_LIB_FILES
	${HEXTER_FILES}
	${CMAKE_CURRENT_SOURCE_DIR}/src/hexter.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/hexter.c
	${UTILS_FILES}
	)

