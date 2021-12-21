#ifndef HEXTER_SRC_PROCESS_HANDLER_WIN_H
#define HEXTER_SRC_PROCESS_HANDLER_WIN_H

#include <stdint.h>

#include <windows.h>
#include "Globals.h"

size_t getSizeOfProcess(uint32_t pid);
BOOL listProcessModules(uint32_t pid);
BOOL listProcessThreads(size_t pid);
Bool listProcessMemory(uint32_t pid);

Bool listProcessHeaps(uint32_t pid, int type);
uint8_t makeStartHitAccessableMemory(uint32_t pid, size_t* start);
int writeProcessMemory(uint32_t pid, uint8_t* _payload, uint32_t _payload_ln, size_t start);
BOOL printProcessRegions(uint32_t pid, size_t start, uint8_t skip_bytes, uint8_t* needle, uint32_t needle_ln);
//BOOL printProcessModules(uint32_t pid, size_t start, uint8_t skip_bytes, uint8_t* needle, uint32_t needle_ln);
BOOL stackTrace(uint32_t pid);
Bool listRunningProcesses();

#endif
