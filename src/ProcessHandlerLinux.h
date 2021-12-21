#ifndef HEXTER_SRC_UTILS_PROCESS_HANDLER_LINUX_H
#define HEXTER_SRC_UTILS_PROCESS_HANDLER_LINUXH

#include <stdint.h>

#include "Globals.h"

uint64_t getSizeOfProcess(uint32_t pid);
Bool listProcessModules(uint32_t pid);
Bool listProcessThreads(uint64_t pid);
Bool listProcessMemory(uint32_t pid);

Bool listProcessHeaps(uint32_t pid, int type);
uint8_t makeStartHitAccessableMemory(uint32_t pid, uint64_t* start);
int writeProcessMemory(uint32_t pid, uint8_t* _payload, uint32_t _payload_ln, uint64_t start);
Bool printProcessRegions(uint32_t pid, uint64_t start, uint8_t skip_bytes, uint8_t* needle, uint32_t needle_ln);
Bool listRunningProcesses();

#endif
