#ifndef HEXTER_SRC_HEXTER_H
#define HEXTER_SRC_HEXTER_H

#if defined(__linux__) || defined(__linux) || defined(linux)
	#define HEXTER_API
#elif defined(HEXTER_EXPORTS)
	#define HEXTER_API __declspec(dllexport)
#else
	#define HEXTER_API __declspec(dllimport)
#endif

#include <limits.h>
#include <stdint.h>

#ifdef _WIN32
	#include <windows.h>
#endif
#include "Globals.h"

HEXTER_API int printFile(char* _file_name, size_t _start, size_t _length);
HEXTER_API int printProcess(uint32_t _pid, size_t _start, size_t _length, int _lpm, int _lpx, int _lph, int _lpt);
#ifdef _WIN32
HEXTER_API void runHexter(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);
#endif

#endif
