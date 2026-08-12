#ifndef HEXTER_SRC_UTILS_PROCESS_HANDLER_LINUX_H
#define HEXTER_SRC_UTILS_PROCESS_HANDLER_LINUX_H

#include <stdint.h>

#include "Globals.h"

uint64_t getSizeOfProcess(uint32_t pid);
Bool listProcessModules(uint32_t pid);
Bool listProcessThreads(uint64_t pid);
Bool listProcessMemory(uint32_t pid);

Bool listProcessHeaps(uint32_t pid, int type);
uint8_t makeStartHitAccessableMemory(uint32_t pid, uint64_t* start);
int writeProcessMemory(uint32_t pid, uint8_t* _payload, uint32_t _payload_ln, uint64_t start);
Bool printProcessRegions(uint32_t pid, uint64_t start, uint32_t skip_bytes, FIND_CFG *find_cfg);
Bool listRunningProcesses();

#endif
