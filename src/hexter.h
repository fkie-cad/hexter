#ifndef HEXTER_SRC_HEXTER_H
#define HEXTER_SRC_HEXTER_H

#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__APPLE__)
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


#define PROCESS_LIST_MEMORY             (0x01)
#define PROCESS_LIST_MODULES            (0x02)
#define PROCESS_LIST_THREADS            (0x04)
#define PROCESS_LIST_HEAPS              (0x08)
#define PROCESS_LIST_HEAP_BLOCKS        (0x10)
#define PROCESS_LIST_RUNNING_PROCESSES  (0x20)


#ifdef __cplusplus
extern "C"{
#endif
    
/**
 * Print file memory
 * 
 * @param _file_name char* the file name
 * @param _start size_t start of memory to print
 * @param _length size_t block length of memory to print
 * @return int success state
 */
HEXTER_API int hexter_printFile(
    const char* _file_name, 
    size_t _start, 
    size_t _length
);

/**
 * Print process memory
 * 
 * @param _pid uint32_t process id, 0 for self
 * @param _start size_t start of process memory to print
 * @param _length size_t block length of memory to print
 * @param flags uint32_t PROCESS_LIST_XXX flags. Prevents printing.
 * @return int success state
 */
HEXTER_API int hexter_printProcess(
    uint32_t _pid, 
    size_t _start, 
    size_t _length, 
    uint32_t flags
);

#ifdef _WIN32
// rundll32 call method
HEXTER_API void runHexter(
    HWND hwnd, 
    HINSTANCE hinst, 
    LPSTR lpszCmdLine, 
    int nCmdShow
);
#endif

#ifdef __cplusplus
}
#endif

#endif
