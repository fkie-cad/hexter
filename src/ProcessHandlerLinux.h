#ifndef HEXTER_SRC_UTILS_PROCESS_HANDLER_LINUX_H
#define HEXTER_SRC_UTILS_PROCESS_HANDLER_LINUXH

#include <stdint.h>

#include "Globals.h"

uint64_t getSizeOfProcess(uint32_t pid);
bool listProcessModules(uint32_t pid);
bool listProcessThreads(uint64_t pid);
bool listProcessMemory(uint32_t pid);

bool listProcessHeaps(uint32_t pid, int type);
uint8_t makeStartAndLengthHitAModule(uint32_t pid, uint64_t* start);
uint8_t makeStartAndLengthHitAccessableMemory(uint32_t pid, uint64_t* start);
int writeProcessMemory(uint32_t pid, unsigned char* _payload, uint32_t _payload_ln, uint64_t start);
bool printProcessRegions(uint32_t pid, uint64_t start, uint8_t skip_bytes, unsigned char* needle, uint32_t needle_ln);

#endif
